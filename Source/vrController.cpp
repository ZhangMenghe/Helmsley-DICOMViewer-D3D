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
						    //: m_tracking(false)
							: m_deviceResources(deviceResources)
							, m_manager(manager){
	myPtr_ = this;
	auto device = deviceResources->GetD3DDevice();

	m_vRenderers.reserve(RENDER_METHOD_END);
	m_vRenderers.emplace_back(std::make_unique<textureBasedVolumeRenderer>(device));
	m_vRenderers.emplace_back(std::make_unique<viewAlignedSlicingRenderer>(device));
	m_vRenderers.emplace_back(std::make_unique<raycastVolumeRenderer>(device));

	m_screen_quad = std::make_unique<screenQuadRenderer>(device);
	m_cutter = std::make_unique<cuttingController>(device);
	m_data_board = std::make_unique<dataBoard>(device);
	m_meshRenderer = std::make_unique<organMeshRenderer>(device);
	Manager::camera = new Camera;

	//onReset();
}

void vrController::onReset(){
	SpaceMat_ = glm::mat4(1.0f);
	Mouse_old = {.0f, .0f};

	volume_model_dirty = true; volume_rotate_dirty = true;
	if (m_cutter.get()) m_cutter->onReset(m_deviceResources->GetD3DDevice());
}

//reset with template
void vrController::onReset(glm::vec3 pv, glm::vec3 sv, glm::mat4 rm, Camera *cam, const std::string& state_name){
	onReset();
	Manager::camera->Reset(cam);

	if (state_name.empty()) {
		PosVec3_ = pv; RotateMat_ = rm; ScaleVec3_ = sv;
		if (Manager::screen_w != 0)Manager::camera->setProjMat(Manager::screen_w, Manager::screen_h);
	}else {
		m_manager->addMVPStatus(state_name, rm, sv, pv, true);
		m_manager->getCurrentMVPStatus(RotateMat_, ScaleVec3_, PosVec3_);
	}
}

void vrController::assembleTexture(int update_target, UINT ph, UINT pw, UINT pd, float sh, float sw, float sd, UCHAR *data, int channel_num){
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

		for (auto render = m_vRenderers.begin(); render != m_vRenderers.end(); render++)
			render->get()->setDimension(m_deviceResources->GetD3DDevice(), vol_dimension_, vol_dim_scale_);
		m_cutter->setDimension(pd, vol_dim_scale_.z);
	}

	tex_volume = std::make_unique<Texture>();
	D3D11_TEXTURE3D_DESC texSrcDesc{
		ph,pw,pd,
		1,
		DXGI_FORMAT_R32_UINT,
		D3D11_USAGE_DEFAULT,
		D3D11_BIND_SHADER_RESOURCE,
		0,
		0
	};

	if (!tex_volume->Initialize(m_deviceResources->GetD3DDevice(), m_deviceResources->GetD3DDeviceContext(), texSrcDesc, data)) { 
		tex_volume.reset(); tex_volume = nullptr; 
		m_volume_valid = false; return;
	}

	tex_baked = std::make_unique<Texture>();
	D3D11_TEXTURE3D_DESC texBakedDesc{
		ph,pw,pd,
		1,
		DXGI_FORMAT_R8G8B8A8_UNORM,
		D3D11_USAGE_DEFAULT,
		D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE,
		0,
		D3D11_RESOURCE_MISC_GENERATE_MIPS
	};

	tex_baked->Initialize(m_deviceResources->GetD3DDevice(), texBakedDesc);
	// Generate mipmaps for this texture.
	tex_baked->GenerateMipMap(m_deviceResources->GetD3DDeviceContext());

	D3D11_TEXTURE3D_DESC texDesc{
			vol_dimension_.x, vol_dimension_.y, vol_dimension_.z,
			1,
			DXGI_FORMAT_R8G8B8A8_UNORM,
			D3D11_USAGE_DEFAULT,
			D3D11_BIND_UNORDERED_ACCESS,
			0,
			0 };
	ID3D11Texture3D* tmp_tex_3d;
	DX::ThrowIfFailed(
		m_deviceResources->GetD3DDevice()->CreateTexture3D(&texDesc, nullptr, &tmp_tex_3d));
	m_comp_tex_d3d = std::unique_ptr<ID3D11Texture3D>(tmp_tex_3d);

	D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE3D;
	uavDesc.Format = texDesc.Format;
	uavDesc.Texture3D.MipSlice = 0;
	uavDesc.Texture3D.FirstWSlice = 0;
	uavDesc.Texture3D.WSize = vol_dimension_.z;
	
	ID3D11UnorderedAccessView* tmp_uav;
	DX::ThrowIfFailed(
		m_deviceResources->GetD3DDevice()->CreateUnorderedAccessView(m_comp_tex_d3d.get(), &uavDesc, &tmp_uav));
	m_textureUAV.reset(tmp_uav);

	Manager::baked_dirty_ = true;
	m_volume_valid = true;
	if (!m_comp_shader_setup) setup_compute_shader();
	

	m_meshRenderer->Setup(m_deviceResources->GetD3DDevice(), ph, pw, pd);
}

// Initializes view parameters when the window size changes.
void vrController::onViewChanged(float width, float height){
	if (width == .0)
		return;
	DX::ThrowIfFailed(m_screen_quad->InitializeQuadTex(m_deviceResources->GetD3DDevice(), m_deviceResources->GetD3DDeviceContext(), width, height));
}

void vrController::precompute()
{
	if (!Manager::baked_dirty_)
		return;
	auto context = m_deviceResources->GetD3DDeviceContext();
	//run compute shader
	context->CSSetShader(m_bakeShader.get(), nullptr, 0);
	ID3D11ShaderResourceView *texview = tex_volume->GetTextureView();
	context->CSSetShaderResources(0, 1, &texview);
	ID3D11UnorderedAccessView* tmp_uav = m_textureUAV.release();
	context->CSSetUnorderedAccessViews(0, 1, &tmp_uav, nullptr);
	m_textureUAV.reset(tmp_uav);

	if (m_comp_shader_setup)	{
		volumeSetupConstBuffer vol_setup;
		m_manager->getVolumeSetupConstData(vol_setup);
		// Prepare the constant buffer to send it to the graphics device.
		context->UpdateSubresource(
				m_compute_constbuff.get(),
				0,
				nullptr,
				&vol_setup,
				0,
				0);
		ID3D11Buffer* tmp_buff = m_compute_constbuff.release();
		context->CSSetConstantBuffers(0, 1, &tmp_buff);
		m_compute_constbuff.reset(tmp_buff);
	}

	context->Dispatch((vol_dimension_.x + 7) / 8, (vol_dimension_.y + 7) / 8, (vol_dimension_.z + 7) / 8);
	context->CopyResource(tex_baked->GetTexture3D(), m_comp_tex_d3d.get());
	//unbind UAV
	ID3D11UnorderedAccessView *nullUAV[] = {NULL};
	context->CSSetUnorderedAccessViews(0, 1, nullUAV, 0);
	// Disable Compute Shader
	context->CSSetShader(nullptr, nullptr, 0);

	m_data_board->Update(m_deviceResources->GetD3DDevice(), context);
	Manager::baked_dirty_ = false;
}
void vrController::Render(int view_id)
{
	if (tex_volume == nullptr || tex_baked == nullptr) return;
	if (Manager::mvp_dirty_) { m_manager->getCurrentMVPStatus(RotateMat_, ScaleVec3_, PosVec3_); volume_model_dirty = true; }
	if (volume_model_dirty)
	{
		update_volume_model_mat();
		volume_model_dirty = false;
	}

	auto context = m_deviceResources->GetD3DDeviceContext();
	//front or back
	DirectX::XMFLOAT4X4 m_rot_mat;
	XMStoreFloat4x4(&m_rot_mat, mat42xmmatrix(RotateMat_));
	auto model_mat_tex = SpaceMat_ * ModelMat_;
	auto model_mat = model_mat_tex * vol_dim_scale_mat_;

	bool is_front = (model_mat[2][2] * Manager::camera->getViewDirection().z) < 0;
	context->RSSetState(is_front ? m_render_state_front : m_render_state_back);

	if (Manager::IsCuttingNeedUpdate()) m_cutter->Update(model_mat);

	bool render_complete = true;
	
	////  CUTTING PLANE  //////
	if (Manager::param_bool[CHECK_CUTTING] && Manager::param_bool[CHECK_SHOW_CPLANE]){
		render_complete &= m_cutter->Draw(m_deviceResources->GetD3DDeviceContext());
		m_deviceResources->ClearCurrentDepthBuffer();
	}

	precompute();

	//////   VOLUME   //////
	if (m_manager->isDrawVolume()){
		switch (m_rmethod_id) {
		case TEXTURE_BASED:
			render_complete &= m_vRenderers[m_rmethod_id]->Draw(context, tex_baked.get(), mat42xmmatrix(model_mat_tex), is_front);
			break;
		case VIEW_ALIGN_SLICING:
			if (volume_rotate_dirty || m_vRenderers[m_rmethod_id]->isVerticesDirty()) {
				m_vRenderers[m_rmethod_id]->updateVertices(context, model_mat_tex);
			}
			volume_rotate_dirty = false;
		case RAYCASTING:
			render_complete &= m_vRenderers[m_rmethod_id]->Draw(context, tex_baked.get(), mat42xmmatrix(model_mat));
			break;
		default:
			break;
		}
		m_deviceResources->ClearCurrentDepthBuffer();
	}

	/// MESH  ////
	if (m_manager->isDrawMesh()) {
		render_complete &= m_meshRenderer->Draw(m_deviceResources->GetD3DDeviceContext(), tex_volume.get(), mat42xmmatrix(model_mat));
		m_deviceResources->ClearCurrentDepthBuffer();
	}

	/// CENTER LINE/////
	if (m_manager->isDrawCenterLine())
	{
		auto mask_bits_ = m_manager->getMaskBits();
		for (auto line = m_line_renderers.begin(); line != m_line_renderers.end(); line++)
			if ((mask_bits_ >> (line->first + 1)) & 1)
				render_complete &= line->second->Draw(context, mat42xmmatrix(model_mat));
		m_deviceResources->ClearCurrentDepthBuffer();
	}

	/// CENTERLINE TRAVERSAL PLANE////
	if (Manager::param_bool[CHECK_CENTER_LINE_TRAVEL] && Manager::param_bool[CHECK_SHOW_CPLANE])
	{
		render_complete &= m_cutter->Draw(m_deviceResources->GetD3DDeviceContext(), is_front);
		m_deviceResources->ClearCurrentDepthBuffer();
	}

	/// OVERLAY DATA BOARD/////
	if (Manager::param_bool[CHECK_OVERLAY])
	{
		render_complete &= m_data_board->Draw(m_deviceResources->GetD3DDeviceContext(),
											DirectX::XMMatrixScaling(1.0, 0.3, 0.1) 
											* DirectX::XMMatrixTranslation(0.8f, 0.8f, DEFAULT_VIEW_Z),
											is_front);
		m_deviceResources->ClearCurrentDepthBuffer();
	}
	Manager::baked_dirty_ = false;
	//m_scene_dirty = !render_complete;
	context->RSSetState(m_render_state_front);
}

void vrController::setup_compute_shader(){
	auto loadCSTask = DX::ReadDataAsync(L"VolumeCompute.cso");
	// After the vertex shader file is loaded, create the shader and input layout.
	auto createCSTask = loadCSTask.then([this](const std::vector<byte> &fileData) {
		ID3D11ComputeShader* tmp_bake_shader;
		DX::ThrowIfFailed(
				m_deviceResources->GetD3DDevice()->CreateComputeShader(
						&fileData[0],
						fileData.size(),
						nullptr,
						&tmp_bake_shader));
		m_bakeShader = std::unique_ptr<ID3D11ComputeShader>(tmp_bake_shader);

		ID3D11Buffer* tmp_buff;
		CD3D11_BUFFER_DESC constantBufferDesc(sizeof(XMFLOAT4X4) * 12, D3D11_BIND_CONSTANT_BUFFER);
		winrt::check_hresult(
				m_deviceResources->GetD3DDevice()->CreateBuffer(
						&constantBufferDesc,
						nullptr,
						&tmp_buff));
		m_compute_constbuff = std::unique_ptr<ID3D11Buffer>(tmp_buff);
		m_comp_shader_setup = true;
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

void vrController::onSingle3DTouchDown(float x, float y, float z, int side){
	//char debug[256];
	//sprintf(debug, "====Touch %d: %f,%f,%f\n", side, x, y, z);
	//OutputDebugStringA(debug);

	m_IsPressed3D[side] = true;
	m_Mouse3D_old[side] = { x, y, z };

	// record distance for scaling
	if (m_IsPressed3D[1 - side]) {
		glm::vec3 delta = m_Mouse3D_old[side] - m_Mouse3D_old[1 - side];
		distance_old = glm::length(delta);
		vector_old = delta;
	}
}

void vrController::onTouchMove(float x, float y)
{
	if (!m_IsPressed || !tex_volume)
		return;

	if (!Manager::param_bool[CHECK_CUTTING] && Manager::param_bool[CHECK_FREEZE_VOLUME])
		return;

	float xoffset = x - Mouse_old.x, yoffset = Mouse_old.y - y;
	Mouse_old = {x, y};
	xoffset *= MOUSE_ROTATE_SENSITIVITY;
	yoffset *= -MOUSE_ROTATE_SENSITIVITY;

	if (Manager::param_bool[CHECK_FREEZE_VOLUME])
	{
		m_cutter->onRotate(xoffset, yoffset);
		//m_scene_dirty = true;
		return;
	}
	RotateMat_ = mouseRotateMat(RotateMat_, xoffset, yoffset);
	volume_model_dirty = true; volume_rotate_dirty = true;
}
void vrController::on3DTouchMove(float x, float y, float z, glm::mat4 rot, int side, std::vector<dvr::OXR_POSE_TYPE>& types){
	if (USE_GESTURE_CUTTING && side == 0){
		// Update cutting plane
		glm::vec3 normal = glm::vec3(-1, 0, 0);
		auto inv_model_mat = glm::inverse(ModelMat_ * vol_dim_scale_mat_);
		normal = glm::mat3(inv_model_mat) * glm::mat3(rot) * normal;

		glm::vec3 pos = glm::vec3(inv_model_mat * glm::vec4(x, y, z, 1)); //glm::vec3(x, y, z);//glm::vec3(inv_model_mat * glm::vec4(x, y, z, 1));

		setCuttingPlane(pos, normal);
	}
	glm::vec3 npos = { x, y, z };
	//Rotate
	if (m_IsPressed3D[OXR_INPUT_LEFT] && m_IsPressed3D[OXR_INPUT_RIGHT]) {
		glm::vec3 delta = npos - m_Mouse3D_old[1 - side];
		if (side == OXR_INPUT_RIGHT) delta = -delta;
		float distance = glm::length(delta);
		// Scale
		float ratio = distance / distance_old;
		// Midpoint
		glm::vec3 midpoint = (npos + m_Mouse3D_old[1 - side]) * 0.5f;
		if (ratio > 0.8f && ratio < 1.2f) ScaleVec3_ *= ratio;
		types.push_back(dvr::POSE_SCALE);

		m_Mouse3D_old[side] = npos;
		distance_old = distance;
		volume_model_dirty = true; volume_rotate_dirty = true;
		types.push_back(dvr::POSE_ROTATE);

		// move
		glm::vec3 mid_delta = midpoint - m_Mouse3D_old[OXR_INPUT_MID];
		if (glm::length(mid_delta) < 0.01f) PosVec3_ += mid_delta;
		m_Mouse3D_old[OXR_INPUT_MID] = midpoint;

		// rotate
		glm::vec3 curr = glm::normalize(glm::vec3{ delta.x, 0, delta.z });
		glm::vec3 old = glm::normalize(glm::vec3{ vector_old.x, 0, vector_old.z });
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
	//MOVE
	else if((m_IsPressed3D[OXR_INPUT_LEFT] && side == OXR_INPUT_LEFT)
		||(m_IsPressed3D[OXR_INPUT_RIGHT] && side == OXR_INPUT_RIGHT)){

		PosVec3_ += (npos - m_Mouse3D_old[side]) * MOUSE_3D_SENSITIVITY;
		m_Mouse3D_old[side] = npos;
		volume_model_dirty = true;
		types.push_back(dvr::POSE_TRANSLATE);
	}
}

void vrController::onTouchReleased()
{
	m_IsPressed = false;
}

void vrController::on3DTouchReleased(int side)
{
	m_IsPressed3D[side] = false;
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

	if (Manager::param_bool[CHECK_FREEZE_VOLUME])
	{
		m_cutter->onScale(sx);
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
	if (!tex_volume || Manager::param_bool[CHECK_FREEZE_VOLUME])
		return;

	float offx = x / Manager::screen_w * MOUSE_PAN_SENSITIVITY, offy = -y / Manager::screen_h * MOUSE_PAN_SENSITIVITY;
	PosVec3_.x += offx * ScaleVec3_.x;
	PosVec3_.y += offy * ScaleVec3_.y;
	volume_model_dirty = true;
}

void vrController::update_volume_model_mat()
{
	ModelMat_ =
		glm::translate(glm::mat4(1.0), PosVec3_)
		* RotateMat_
		* glm::scale(glm::mat4(1.0), ScaleVec3_);
}

void vrController::setupCenterLine(int id, float *data)
{
	int oid = 0;
	while (id /= 2)
		oid++;
	if (m_line_renderers.count(oid))
		m_line_renderers[oid]->updateVertices(m_deviceResources->GetD3DDevice(), 4000, data);
	else
		m_line_renderers[oid] = std::make_unique<lineRenderer>(m_deviceResources->GetD3DDevice(), oid, 4000, data);
	m_cutter->setupCenterLine((ORGAN_IDS)oid, data);
}

void vrController::setCuttingPlane(float value)
{
	/*if (Manager::param_bool[CHECK_CUTTING] && !isRayCasting()){
		cutter_->setCutPlane(value);
		vRenderer_[0]->setCuttingPlane(value);
		vRenderer_[1]->setCuttingPlane(value);
		m_scene_dirty = true;
	}else*/
	if (Manager::param_bool[CHECK_CENTER_LINE_TRAVEL]){
		if (!m_cutter->IsCenterLineAvailable())
			return;
		m_cutter->setCenterLinePos((int)(value * 4000.0f));
		if (Manager::param_bool[CHECK_TRAVERSAL_VIEW])
			align_volume_to_traversal_plane();
		//m_scene_dirty = true;
	}
}

void vrController::setCuttingPlane(int id, int delta)
{
	if (Manager::param_bool[CHECK_CUTTING]){
		m_cutter->setCuttingPlaneDelta(delta);
		//m_scene_dirty = true;
	}
	else if (Manager::param_bool[CHECK_CENTER_LINE_TRAVEL])
	{
		if (!m_cutter->IsCenterLineAvailable())
			return;
		m_cutter->setCenterLinePos(id, delta);
		if (Manager::param_bool[CHECK_TRAVERSAL_VIEW])
			align_volume_to_traversal_plane();
		//m_scene_dirty = true;
	}
}

void vrController::setCuttingPlane(glm::vec3 pp, glm::vec3 pn)
{
	m_cutter->setCutPlane(pp, pn);
	//m_scene_dirty = true;
}

void vrController::switchCuttingPlane(PARAM_CUT_ID cut_plane_id)
{
	m_cutter->SwitchCuttingPlane(cut_plane_id);
	if (Manager::param_bool[CHECK_TRAVERSAL_VIEW])
		align_volume_to_traversal_plane();
	//m_scene_dirty = true;
}

void vrController::align_volume_to_traversal_plane()
{
	glm::vec3 pp, pn;
	m_cutter->getCurrentTraversalInfo(pp, pn);
	pn = glm::normalize(pn);

	RotateMat_ = glm::toMat4(glm::rotation(pn, glm::vec3(.0, .0, -1.0f)));

	volume_model_dirty = true;
}