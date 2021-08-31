#include "pch.h"
#include "vrController.h"
#include <Common/Manager.h>
#include <Common/DirectXHelper.h>
#include <Utils/MathUtils.h>
#include <Utils/TypeConvertUtils.h>
#include <Renderers/textureBasedVolumeRenderer.h>
#include <Renderers/ViewAlignedSlicingRenderer.h>
#include <Renderers/raycastVolumeRenderer.h>
using namespace dvr;
using namespace DirectX;
//using namespace winrt::Windows::Foundation;

vrController *vrController::myPtr_ = nullptr;

vrController *vrController::instance()
{
	return myPtr_;
}

// Loads vertex and pixel shaders from files and instantiates the cube geometry.
vrController::vrController(const std::shared_ptr<DX::DeviceResources> &deviceResources, const std::shared_ptr<Manager> &manager)
						    : m_tracking(false)
							, m_deviceResources(deviceResources)
							, m_manager(manager){
	myPtr_ = this;
	auto device = deviceResources->GetD3DDevice();

	vRenderer_.reserve(dvr::RENDER_METHOD_END);
	vRenderer_.emplace_back(new textureBasedVolumeRenderer(device));
	vRenderer_.emplace_back(new viewAlignedSlicingRenderer(device));
	vRenderer_.emplace_back(new raycastVolumeRenderer(device));

	screen_quad = new screenQuadRenderer(device);
	cutter_ = new cuttingController(device);
	data_board_ = new dataBoard(device);
	meshRenderer_ = new organMeshRenderer(device);
	Manager::camera = new Camera;

	CreateDeviceDependentResources();
	CreateWindowSizeDependentResources();
	onReset();
}
void vrController::onReset()
{
	SpaceMat_ = glm::mat4(1.0f);

	Mouse_old = {.0f, .0f};
	rStates_.clear();
	cst_name = "";
	addStatus("default_status");
	setMVPStatus("default_status");

	volume_model_dirty = true; volume_rotate_dirty = true;
	if (cutter_)
		cutter_->onReset(m_deviceResources->GetD3DDevice());
}

void vrController::onReset(glm::vec3 pv, glm::vec3 sv, glm::mat4 rm, Camera *cam)
{
	Mouse_old = {.0f, .0f};
	rStates_.clear();
	cst_name = "";

	glm::mat4 mm = glm::translate(glm::mat4(1.0), pv) * rm * glm::scale(glm::mat4(1.0), sv);

	if (cam == nullptr)
		addStatus("template", mm, rm, sv, pv, Manager::camera);
	else
		addStatus("template", mm, rm, sv, pv, cam);

	setMVPStatus("template");
	if (cutter_)
		cutter_->onReset(m_deviceResources->GetD3DDevice());
	volume_model_dirty = false; volume_rotate_dirty = true;
}

void vrController::InitOXRScene(){
	uniScale = 0.5f;
	PosVec3_.z = -1.0f;
	volume_model_dirty = true; volume_rotate_dirty = true;
}

void vrController::assembleTexture(int update_target, UINT ph, UINT pw, UINT pd, float sh, float sw, float sd, UCHAR *data, int channel_num)
{
	if (update_target == 0 || update_target == 2)
	{
		vol_dimension_ = {ph, pw, pd};
		if (sh <= 0 || sw <= 0 || sd <= 0)
		{
			if (pd > 200)
				vol_dim_scale_ = {1.0f, 1.0f, 0.5f};
			else if (pd > 100)
				vol_dim_scale_ = {1.0f, 1.0f, pd / 300.f};
			else
				vol_dim_scale_ = {1.0f, 1.0f, pd / 200.f};
		}
		else if (abs(sh - sw) < 1)
		{
			vol_dim_scale_ = {1.0f, 1.0f, sd / sh};
		}
		else
		{
			vol_dim_scale_ = {1.0f, 1.0f, sd / fmax(sw, sh)};
			if (sw > sh)
			{
				ScaleVec3_.y *= sh / sw;
			}
			else
			{
				ScaleVec3_.x *= sw / sh;
			}
			volume_model_dirty = true;
		}
		vol_dim_scale_mat_ = glm::scale(glm::mat4(1.0f), vol_dim_scale_);
		m_manager->setDimension(vol_dimension_);
		for (auto render : vRenderer_)render->setDimension(m_deviceResources->GetD3DDevice(), vol_dimension_, vol_dim_scale_);
		cutter_->setDimension(pd, vol_dim_scale_.z);
	}

	if (tex_volume != nullptr)
	{
		delete tex_volume;
		tex_volume = nullptr;
	}

	tex_volume = new Texture;
	D3D11_TEXTURE3D_DESC texDesc{
		ph,pw,pd,
		1,
		DXGI_FORMAT_R32_UINT,
		D3D11_USAGE_DEFAULT,
		D3D11_BIND_RENDER_TARGET|D3D11_BIND_SHADER_RESOURCE,
		0,
		D3D11_RESOURCE_MISC_GENERATE_MIPS
	};

	if (!tex_volume->Initialize(m_deviceResources->GetD3DDevice(), m_deviceResources->GetD3DDeviceContext(), texDesc, data)) { delete tex_volume; tex_volume = nullptr; }
	tex_volume->GenerateMipMap(m_deviceResources->GetD3DDeviceContext());

	if (tex_baked != nullptr)
	{
		delete tex_baked;
		tex_baked = nullptr;
	}
	tex_baked = new Texture;
	texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	tex_baked->Initialize(m_deviceResources->GetD3DDevice(), texDesc);
	// Generate mipmaps for this texture.
	tex_baked->GenerateMipMap(m_deviceResources->GetD3DDeviceContext());
	init_texture();

	Manager::baked_dirty_ = true;

	meshRenderer_->Setup(m_deviceResources->GetD3DDevice(), ph, pw, pd);
}

void vrController::init_texture() {
	if (m_comp_tex_d3d != nullptr) { 
		m_comp_tex_d3d->Release();
		//delete m_comp_tex_d3d; 
		m_comp_tex_d3d = nullptr; 
	}
	if (m_textureUAV != nullptr) { 
		//delete m_textureUAV;
		m_textureUAV->Release();
		m_textureUAV = nullptr; 
	}

	D3D11_TEXTURE3D_DESC texDesc{
			vol_dimension_.x, vol_dimension_.y, vol_dimension_.z,
			1,
			DXGI_FORMAT_R8G8B8A8_UNORM,
			D3D11_USAGE_DEFAULT,
			D3D11_BIND_UNORDERED_ACCESS,
			0,
			0};

	DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateTexture3D(&texDesc, nullptr, &m_comp_tex_d3d));

	D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE3D;
	uavDesc.Format = texDesc.Format;
	uavDesc.Texture3D.MipSlice = 0;
	uavDesc.Texture3D.FirstWSlice = 0;
	uavDesc.Texture3D.WSize = vol_dimension_.z;
	DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateUnorderedAccessView(m_comp_tex_d3d, &uavDesc, &m_textureUAV));

	volume_model_dirty = true;
}

// Initializes view parameters when the window size changes.
void vrController::CreateWindowSizeDependentResources()
{
	winrt::Windows::Foundation::Size outputSize = m_deviceResources->GetOutputSize();
	if (outputSize.Width == .0)
		return;
	DX::ThrowIfFailed(screen_quad->InitializeQuadTex(m_deviceResources->GetD3DDevice(), m_deviceResources->GetD3DDeviceContext(), outputSize.Width, outputSize.Height));
}

// Called once per frame, rotates the cube and calculates the model and view matrices.
void vrController::Update(DX::StepTimer const &timer)
{
	if (!m_tracking)
	{
		// Convert degrees to radians, then convert seconds to rotation angle
		float radiansPerSecond = XMConvertToRadians(m_degreesPerSecond);
		double totalRotation = timer.GetTotalSeconds() * radiansPerSecond;
		float radians = static_cast<float>(fmod(totalRotation, XM_2PI));

		Rotate(radians);
	}
}

//void vrController::Update(XrTime time) {
//XrSpaceLocation origin{ XR_TYPE_SPACE_LOCATION };
//xrLocateSpace(*space, *app_space, time, &origin);
//SpaceMat_ = xr::math::LoadXrPose(origin.pose);
//}

// Rotate the 3D cube model a set amount of radians.
void vrController::Rotate(float radians)
{
	// Prepare to pass the updated model matrix to the shader
	XMStoreFloat4x4(&m_all_buff_Data.model, XMMatrixTranspose(XMMatrixRotationY(radians)));
}

void vrController::StartTracking()
{
	m_tracking = true;
}

// When tracking, the 3D cube can be rotated around its Y axis by tracking pointer position relative to the output screen width.
void vrController::TrackingUpdate(float positionX)
{
	if (m_tracking)
	{
		float radians = XM_2PI * 2.0f * positionX / m_deviceResources->GetOutputSize().Width;
		Rotate(radians);
	}
}

void vrController::StopTracking()
{
	m_tracking = false;
}

// Renders one frame using the vertex and pixel shaders.
void vrController::Render(int view_id)
{
	if (tex_volume == nullptr || tex_baked == nullptr) return;
	if (!pre_draw_) { render_scene(view_id); return; }

	//auto context = m_deviceResources->GetD3DDeviceContext();
	//if (true) //(true) // TODO: debug
	//{
		/*screen_quad->SetToDrawTarget(context, m_deviceResources.get());
		render_scene(view_id);
		m_deviceResources->removeCurrentTargetViews();*/
	//}
	//m_deviceResources->SetBackBufferRenderTarget();
	//screen_quad->Draw(context);
	render_scene(view_id);
	//m_deviceResources->ClearCurrentDepthBuffer();
	//render_scene(view_id);
}

void vrController::precompute()
{
	if (!Manager::baked_dirty_)
		return;
	if (!bakeShader_)
		CreateDeviceDependentResources();

	auto context = m_deviceResources->GetD3DDeviceContext();
	//run compute shader
	context->CSSetShader(bakeShader_, nullptr, 0);
	ID3D11ShaderResourceView *texview = tex_volume->GetTextureView();
	context->CSSetShaderResources(0, 1, &texview);
	context->CSSetUnorderedAccessViews(0, 1, &m_textureUAV, nullptr);

	if (m_compute_created)	{
		volumeSetupConstBuffer vol_setup;
		m_manager->getVolumeSetupConstData(vol_setup);
		// Prepare the constant buffer to send it to the graphics device.
		context->UpdateSubresource(
				m_compute_constbuff,
				0,
				nullptr,
				&vol_setup,
				0,
				0);
		context->CSSetConstantBuffers(0, 1, &m_compute_constbuff);
	}

	context->Dispatch((vol_dimension_.x + 7) / 8, (vol_dimension_.y + 7) / 8, (vol_dimension_.z + 7) / 8);
	context->CopyResource(tex_baked->GetTexture3D(), m_comp_tex_d3d);
	//unbind UAV
	ID3D11UnorderedAccessView *nullUAV[] = {NULL};
	context->CSSetUnorderedAccessViews(0, 1, nullUAV, 0);
	// Disable Compute Shader
	context->CSSetShader(nullptr, nullptr, 0);

	data_board_->Update(m_deviceResources->GetD3DDevice(), context);
	Manager::baked_dirty_ = false;
}

void vrController::render_scene(int view_id){
	if (volume_model_dirty)
	{
		updateVolumeModelMat();
		volume_model_dirty = false;
	}

	auto context = m_deviceResources->GetD3DDeviceContext();
	//front or back
	DirectX::XMFLOAT4X4 m_rot_mat;
	XMStoreFloat4x4(&m_rot_mat, mat42xmmatrix(RotateMat_));
	auto model_mat_tex = m_use_space_mat?SpaceMat_ *glm::scale(glm::mat4(1.0f), glm::vec3(0.2f)) : ModelMat_;
	auto model_mat = model_mat_tex * vol_dim_scale_mat_;

	bool is_front = (model_mat[2][2] * Manager::camera->getViewDirection().z) < 0;
	context->RSSetState(is_front ? m_render_state_front : m_render_state_back);

	if (Manager::IsCuttingNeedUpdate()) cutter_->Update(model_mat);

	bool render_complete = true;
	
	////  CUTTING PLANE  //////
	if (Manager::param_bool[dvr::CHECK_CUTTING] && Manager::param_bool[dvr::CHECK_SHOW_CPLANE]){
		render_complete &= cutter_->Draw(m_deviceResources->GetD3DDeviceContext());
		m_deviceResources->ClearCurrentDepthBuffer();
	}

	precompute();

	//////   VOLUME   //////
	if (m_manager->isDrawVolume()){
		switch (m_rmethod_id) {
		case dvr::TEXTURE_BASED:
			render_complete &= vRenderer_[m_rmethod_id]->Draw(context, tex_baked, mat42xmmatrix(model_mat_tex), is_front);
			break;
		case dvr::VIEW_ALIGN_SLICING:
			if (volume_rotate_dirty || vRenderer_[m_rmethod_id]->isVerticesDirty()) {
				vRenderer_[m_rmethod_id]->updateVertices(context, model_mat_tex);
			}
			volume_rotate_dirty = false;
		case dvr::RAYCASTING:
			render_complete &= vRenderer_[m_rmethod_id]->Draw(context, tex_baked, mat42xmmatrix(model_mat));
			break;
		default:
			break;
		}
		m_deviceResources->ClearCurrentDepthBuffer();
	}

	/// MESH  ////
	if (m_manager->isDrawMesh()) {
		render_complete &= meshRenderer_->Draw(m_deviceResources->GetD3DDeviceContext(), tex_volume, mat42xmmatrix(model_mat));
		m_deviceResources->ClearCurrentDepthBuffer();
	}

	/// CENTER LINE/////
	if (m_manager->isDrawCenterLine())
	{
		auto mask_bits_ = m_manager->getMaskBits();
		for (auto line : line_renderers_)
			if ((mask_bits_ >> (line.first + 1)) & 1)
				render_complete &= line.second->Draw(context, mat42xmmatrix(model_mat));
		m_deviceResources->ClearCurrentDepthBuffer();
	}

	/// CENTERLINE TRAVERSAL PLANE////
	if (Manager::param_bool[dvr::CHECK_CENTER_LINE_TRAVEL] && Manager::param_bool[dvr::CHECK_SHOW_CPLANE])
	{
		render_complete &= cutter_->Draw(m_deviceResources->GetD3DDeviceContext(), is_front);
		m_deviceResources->ClearCurrentDepthBuffer();
	}

	/// OVERLAY DATA BOARD/////
	if (Manager::param_bool[dvr::CHECK_OVERLAY])
	{
		render_complete &= data_board_->Draw(m_deviceResources->GetD3DDeviceContext(),
											DirectX::XMMatrixScaling(1.0, 0.3, 0.1) * DirectX::XMMatrixTranslation(0.8f, 0.8f, .0f),
											is_front);
		m_deviceResources->ClearCurrentDepthBuffer();
	}
	Manager::baked_dirty_ = false;
	m_scene_dirty = !render_complete;
	context->RSSetState(m_render_state_front);
}

void vrController::CreateDeviceDependentResources()
{
	if (bakeShader_)
		return;
	auto loadCSTask = DX::ReadDataAsync(L"VolumeCompute.cso");
	// After the vertex shader file is loaded, create the shader and input layout.
	auto createCSTask = loadCSTask.then([this](const std::vector<byte> &fileData) {
		DX::ThrowIfFailed(
				m_deviceResources->GetD3DDevice()->CreateComputeShader(
						&fileData[0],
						fileData.size(),
						nullptr,
						&bakeShader_));

		CD3D11_BUFFER_DESC constantBufferDesc(sizeof(XMFLOAT4X4) * 12, D3D11_BIND_CONSTANT_BUFFER);
		winrt::check_hresult(
				m_deviceResources->GetD3DDevice()->CreateBuffer(
						&constantBufferDesc,
						nullptr,
						&m_compute_constbuff));
		m_compute_created = true;
	});
	D3D11_RASTERIZER_DESC rasterDesc;

	rasterDesc.AntialiasedLineEnable = false;
	rasterDesc.CullMode = D3D11_CULL_BACK;
	rasterDesc.DepthBias = 0;
	rasterDesc.DepthBiasClamp = 0.0f;
	rasterDesc.DepthClipEnable = false;
	rasterDesc.FillMode = D3D11_FILL_SOLID;
	rasterDesc.FrontCounterClockwise = false;
	rasterDesc.MultisampleEnable = false;
	rasterDesc.ScissorEnable = false;
	rasterDesc.SlopeScaledDepthBias = 0.0f;
	winrt::check_hresult(
			m_deviceResources->GetD3DDevice()->CreateRasterizerState(&rasterDesc, &m_render_state_front));
	rasterDesc.FrontCounterClockwise = true;
	winrt::check_hresult(
			m_deviceResources->GetD3DDevice()->CreateRasterizerState(&rasterDesc, &m_render_state_back));
}
void vrController::onSingleTouchDown(float x, float y)
{
	Mouse_old = {x, y};
	m_IsPressed = true;
}

void vrController::onSingle3DTouchDown(float x, float y, float z, int side)
{
	//char debug[256];
	//sprintf(debug, "Touch %d: %f,%f,%f\n", side, x, y, z);
	//OutputDebugStringA(debug);
	if (side == 0)
	{
		Mouse3D_old_left = {x, y, z};
		m_IsPressed_left = true;
		if (m_IsPressed_right)
		{
			// record distance for scaling
			glm::vec3 delta = {x = Mouse3D_old_right.x, y - Mouse3D_old_right.y, z - Mouse3D_old_right.z};
			distance_old = glm::length(delta);
			vector_old = delta;
		}
	}
	else
	{
		Mouse3D_old_right = {x, y, z};
		m_IsPressed_right = true;
		if (m_IsPressed_left)
		{
			// record distance for scaling
			glm::vec3 delta = {x - Mouse3D_old_left.x, y - Mouse3D_old_left.y, z - Mouse3D_old_left.z};
			distance_old = glm::length(delta);
			vector_old = delta;
		}
	}
}

void vrController::onTouchMove(float x, float y)
{
	if (!m_IsPressed || !tex_volume)
		return;

	if (!Manager::param_bool[dvr::CHECK_CUTTING] && Manager::param_bool[dvr::CHECK_FREEZE_VOLUME])
		return;

	float xoffset = x - Mouse_old.x, yoffset = Mouse_old.y - y;
	Mouse_old = {x, y};
	xoffset *= dvr::MOUSE_ROTATE_SENSITIVITY;
	yoffset *= -dvr::MOUSE_ROTATE_SENSITIVITY;

	if (Manager::param_bool[dvr::CHECK_FREEZE_VOLUME])
	{
		cutter_->onRotate(xoffset, yoffset);
		m_scene_dirty = true;
		return;
	}
	RotateMat_ = mouseRotateMat(RotateMat_, xoffset, yoffset);
	volume_model_dirty = true; volume_rotate_dirty = true;
}

void vrController::on3DTouchMove(float x, float y, float z, glm::mat4 rot, int side){
	if (dvr::USE_GESTURE_CUTTING &&side == 0){
		// Update cutting plane
		glm::vec3 normal = glm::vec3(-1, 0, 0);
		auto inv_model_mat = glm::inverse(SpaceMat_ * ModelMat_ * vol_dim_scale_mat_);
		normal = glm::mat3(inv_model_mat) * glm::mat3(rot) * normal;

		glm::vec3 pos = glm::vec3(inv_model_mat * glm::vec4(x, y, z, 1)); //glm::vec3(x, y, z);//glm::vec3(inv_model_mat * glm::vec4(x, y, z, 1));

		setCuttingPlane(pos, normal);
	}

	if (m_IsPressed_left)
	{
		if (side == 0)
		{
			//char debug[256];
			//sprintf(debug, "Touch %d: %f,%f,%f\n", side, x, y, z);
			//OutputDebugStringA(debug);
		}
	}
	if (m_IsPressed_right)
	{
		if (side == 1)
		{
			//char debug[256];
			//sprintf(debug, "Touch %d: %f,%f,%f\n", side, x, y, z);
			//OutputDebugStringA(debug);
		}
	}

	if (!(m_IsPressed_left && m_IsPressed_right))
	{
		// not both hand: translate
		if (side == 0)
		{
			if (m_IsPressed_left)
			{
				float dx = x - Mouse3D_old_left.x;
				float dy = y - Mouse3D_old_left.y;
				float dz = z - Mouse3D_old_left.z;

				PosVec3_.x += dx * sens;
				PosVec3_.y += dy * sens;
				PosVec3_.z += dz * sens;

				Mouse3D_old_left = {x, y, z};
				volume_model_dirty = true;
			}
		}
		else
		{
			if (m_IsPressed_right)
			{
				float dx = x - Mouse3D_old_right.x;
				float dy = y - Mouse3D_old_right.y;
				float dz = z - Mouse3D_old_right.z;

				PosVec3_.x += dx * sens;
				PosVec3_.y += dy * sens;
				PosVec3_.z += dz * sens;

				Mouse3D_old_right = {x, y, z};
				volume_model_dirty = true;
			}
		}
	}
	else
	{
		// both hand: scale
		glm::vec3 delta;
		glm::vec3 midpoint;
		if (side == 0)
		{
			delta.x = x - Mouse3D_old_right.x;
			delta.y = y - Mouse3D_old_right.y;
			delta.z = z - Mouse3D_old_right.z;

			float distance = glm::length(delta);

			// Scale
			float ratio = distance / distance_old;

			// Midpoint
			midpoint = glm::vec3{(x + Mouse3D_old_right.x) / 2.0f, (y + Mouse3D_old_right.y) / 2.0f, (z + Mouse3D_old_right.z) / 2.0f};

			if (ratio > 0.8f && ratio < 1.2f)
			{
				uniScale *= ratio;
			}

			distance_old = distance;
			Mouse3D_old_left = {x, y, z};
			volume_model_dirty = true;
		}
		else
		{
			delta.x = Mouse3D_old_left.x - x;
			delta.y = Mouse3D_old_left.y - y;
			delta.z = Mouse3D_old_left.z - z;

			float distance = glm::length(delta);

			// Scale
			float ratio = distance / distance_old;

			// Midpoint
			midpoint = glm::vec3{(x + Mouse3D_old_left.x) / 2.0f, (y + Mouse3D_old_left.y) / 2.0f, (z + Mouse3D_old_left.z) / 2.0f};

			if (ratio > 0.8f && ratio < 1.2f)
			{
				uniScale *= ratio;
			}

			distance_old = distance;
			Mouse3D_old_right = {x, y, z};
			volume_model_dirty = true; volume_rotate_dirty = true;
		}

		// move
		glm::vec3 mid_delta = glm::vec3{midpoint.x - Mouse3D_old_mid.x, midpoint.y - Mouse3D_old_mid.y, midpoint.z - Mouse3D_old_mid.z};
		if (glm::length(mid_delta) < 0.01f)
		{
			PosVec3_.x += mid_delta.x;
			PosVec3_.y += mid_delta.y;
			PosVec3_.z += mid_delta.z;
		}

		Mouse3D_old_mid = {midpoint.x, midpoint.y, midpoint.z};

		// rotate
		glm::vec3 curr = glm::normalize(glm::vec3{delta.x, 0, delta.z});
		glm::vec3 old = glm::normalize(glm::vec3{vector_old.x, 0, vector_old.z});
		glm::vec3 axis = glm::normalize(glm::cross(old, curr));
		float angle = std::acos(glm::dot(old, curr));

		if (angle < 0.5f && angle > 0.01f)
		{
			glm::mat4 rot = glm::toMat4(glm::angleAxis(angle, axis));

			//char debug[256];
			//sprintf(debug, "Touch %f\n", angle);
			//OutputDebugStringA(debug);

			RotateMat_ *= rot;
		}
		vector_old = glm::normalize(delta);
	}
}

void vrController::onTouchReleased()
{
	m_IsPressed = false;
}

void vrController::on3DTouchReleased(int side)
{
	//char debug[256];
	if (side == 0)
	{
		//sprintf(debug, "Release %d: %f,%f,%f\n", side, Mouse3D_old_left.x, Mouse3D_old_left.y, Mouse3D_old_left.z);
	}
	else
	{
		//sprintf(debug, "Release %d: %f,%f,%f\n", side, Mouse3D_old_right.x, Mouse3D_old_right.y, Mouse3D_old_right.z);
	}
	//OutputDebugStringA(debug);
	if (side == 0)
	{
		m_IsPressed_left = false;
	}
	else
	{
		m_IsPressed_right = false;
	}
}

void vrController::onScale(float sx, float sy)
{
	if (!tex_volume)
		return;
	//unified scaling
	if (sx > 1.0f)
		sx = 1.0f + (sx - 1.0f) * MOUSE_SCALE_SENSITIVITY;
	else
		sx = 1.0f - (1.0f - sx) * MOUSE_SCALE_SENSITIVITY;

	if (Manager::param_bool[dvr::CHECK_FREEZE_VOLUME])
	{
		cutter_->onScale(sx);
	}
	else
	{
		ScaleVec3_ = ScaleVec3_ * sx;
		volume_model_dirty = true;
	}
}

void vrController::onScale(float scale)
{
}

void vrController::onPan(float x, float y)
{
	if (!tex_volume || Manager::param_bool[dvr::CHECK_FREEZE_VOLUME])
		return;

	float offx = x / Manager::screen_w * MOUSE_PAN_SENSITIVITY, offy = -y / Manager::screen_h * MOUSE_PAN_SENSITIVITY;
	PosVec3_.x += offx * ScaleVec3_.x;
	PosVec3_.y += offy * ScaleVec3_.y;
	volume_model_dirty = true;
}
void vrController::updateVolumeModelMat()
{
	ModelMat_ = 
		glm::translate(glm::mat4(1.0), PosVec3_) 
		* RotateMat_ 
		* glm::scale(glm::mat4(1.0), ScaleVec3_) 
		* glm::scale(glm::mat4(1.0), glm::vec3(uniScale));
}

bool vrController::addStatus(std::string name, glm::mat4 mm, glm::mat4 rm, glm::vec3 sv, glm::vec3 pv, Camera *cam)
{
	auto it = rStates_.find(name);
	if (it != rStates_.end())
		return false;

	rStates_[name] = reservedStatus(mm, rm, sv, pv, cam);
	if (Manager::screen_w != 0)
		rStates_[name].vcam->setProjMat(Manager::screen_w, Manager::screen_h);
	return true;
}
bool vrController::addStatus(std::string name, bool use_current_status)
{
	auto it = rStates_.find(name);
	if (it != rStates_.end())
		return false;

	if (use_current_status)
	{
		if (volume_model_dirty)
		{
			updateVolumeModelMat();
			volume_model_dirty = false;
		}
		rStates_[name] = reservedStatus(ModelMat_, RotateMat_, ScaleVec3_, PosVec3_, new Camera(name.c_str()));
	}
	else
	{
		rStates_[name] = reservedStatus();
	}

	if (Manager::screen_w != 0)
	{
		rStates_[name].vcam->setViewMat(Manager::camera->getViewMat());
		//rStates_[name].vcam->setProjMat(Manager::screen_w, Manager::screen_h);
	}
	return true;
}
void vrController::setMVPStatus(std::string name)
{
	if (name == cst_name)
		return;
	auto rstate_ = rStates_[name];
	ModelMat_ = rstate_.model_mat;
	RotateMat_ = rstate_.rot_mat;
	ScaleVec3_ = rstate_.scale_vec;
	PosVec3_ = rstate_.pos_vec;
	Manager::camera = rstate_.vcam;
	volume_model_dirty = false;
	cst_name = name;
}
void vrController::setupCenterLine(int id, float *data)
{
	int oid = 0;
	while (id /= 2)
		oid++;
	if (line_renderers_.count(oid))
		line_renderers_[oid]->updateVertices(m_deviceResources->GetD3DDevice(), 4000, data);
	else
		line_renderers_[oid] = new lineRenderer(m_deviceResources->GetD3DDevice(), oid, 4000, data);
	cutter_->setupCenterLine((dvr::ORGAN_IDS)oid, data);
}
void vrController::setCuttingPlane(float value)
{
	/*if (Manager::param_bool[dvr::CHECK_CUTTING] && !isRayCasting()){
		cutter_->setCutPlane(value);
		vRenderer_[0]->setCuttingPlane(value);
		vRenderer_[1]->setCuttingPlane(value);
		m_scene_dirty = true;
	}else*/
	if (Manager::param_bool[dvr::CHECK_CENTER_LINE_TRAVEL]){
		if (!cutter_->IsCenterLineAvailable())
			return;
		cutter_->setCenterLinePos((int)(value * 4000.0f));
		if (Manager::param_bool[dvr::CHECK_TRAVERSAL_VIEW])
			AlignModelMatToTraversalPlane();
		m_scene_dirty = true;
	}
}
void vrController::setCuttingPlane(int id, int delta)
{
	if (Manager::param_bool[dvr::CHECK_CUTTING]){
		cutter_->setCuttingPlaneDelta(delta);
		m_scene_dirty = true;
	}
	else if (Manager::param_bool[dvr::CHECK_CENTER_LINE_TRAVEL])
	{
		if (!cutter_->IsCenterLineAvailable())
			return;
		cutter_->setCenterLinePos(id, delta);
		if (Manager::param_bool[dvr::CHECK_TRAVERSAL_VIEW])
			AlignModelMatToTraversalPlane();
		m_scene_dirty = true;
	}
}
void vrController::setCuttingPlane(glm::vec3 pp, glm::vec3 pn)
{
	cutter_->setCutPlane(pp, pn);
	m_scene_dirty = true;
}
void vrController::switchCuttingPlane(dvr::PARAM_CUT_ID cut_plane_id)
{
	cutter_->SwitchCuttingPlane(cut_plane_id);
	if (Manager::param_bool[dvr::CHECK_TRAVERSAL_VIEW])
		AlignModelMatToTraversalPlane();
	m_scene_dirty = true;
}
void vrController::ReleaseDeviceDependentResources()
{
	for (auto render : vRenderer_)delete render;
	delete tex_volume;
	delete tex_baked;
	rStates_.clear();
}

bool vrController::isDirty(){
	if (!tex_volume) return false;
	return (Manager::baked_dirty_ || volume_model_dirty || m_scene_dirty);
}
void vrController::AlignModelMatToTraversalPlane()
{
	glm::vec3 pp, pn;
	cutter_->getCurrentTraversalInfo(pp, pn);
	pn = glm::normalize(pn);

	RotateMat_ = glm::toMat4(glm::rotation(pn, glm::vec3(.0, .0, -1.0f)));

	volume_model_dirty = true;
}