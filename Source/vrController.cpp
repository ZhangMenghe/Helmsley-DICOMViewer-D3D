﻿#include "pch.h"
#include "vrController.h"
#include <Common/Manager.h>
#include <Common/DirectXHelper.h>
#include <Utils/MathUtils.h>
#include <Utils/TypeConvertUtils.h>
using namespace DirectX;
using namespace Windows::Foundation;

vrController* vrController::myPtr_ = nullptr;

vrController* vrController::instance() {
	return myPtr_;
}

// Loads vertex and pixel shaders from files and instantiates the cube geometry.
vrController::vrController(const std::shared_ptr<DX::DeviceResources>& deviceResources) :
	m_tracking(false),
	m_deviceResources(deviceResources){
	myPtr_ = this;

	auto device = m_deviceResources->GetD3DDevice();
	screen_quad = new screenQuadRenderer(device);
	raycast_renderer = new raycastVolumeRenderer(device);
	texvrRenderer_ = new textureBasedVolumeRenderer(device);
	cutter_ = new cuttingController(device);
	meshRenderer_ = new organMeshRenderer(device);
	Manager::camera = new Camera;

	CreateDeviceDependentResources();
	CreateWindowSizeDependentResources();
	onReset();
}
void vrController::onReset() {
	SpaceMat_ = glm::mat4();
	PosVec3_.z = -1.0f;
	Mouse_old = { .0f, .0f };
	rStates_.clear();
	cst_name = "";
	addStatus("default_status");
	setMVPStatus("default_status");

	volume_model_dirty = true;
}

void vrController::onReset(glm::vec3 pv, glm::vec3 sv, glm::mat4 rm, Camera* cam) {
	Mouse_old = { .0f, .0f };
	rStates_.clear();
	cst_name = "";

	glm::mat4 mm = glm::translate(glm::mat4(1.0), pv)
		* rm
		* glm::scale(glm::mat4(1.0), sv);
	addStatus("template", mm, rm, sv, pv, cam);
	setMVPStatus("template");

	volume_model_dirty = false;
}

void vrController::assembleTexture(int update_target, UINT ph, UINT pw, UINT pd, float sh, float sw, float sd, UCHAR* data, int channel_num) {
	if (update_target == 0 || update_target == 2) {
		vol_dimension_ = { ph, pw, pd };
		m_cmpdata.u_tex_size = { ph,pw,pd, (UINT)0 };

		if (sh <= 0 || sw <= 0 || sd <= 0) {
			if (pd > 200) vol_dim_scale_ = { 1.0f, 1.0f, 0.5f };
			else if (pd > 100) vol_dim_scale_ = { 1.0f, 1.0f, pd / 300.f };
			else vol_dim_scale_ = { 1.0f, 1.0f, pd / 200.f };
		}
		else if (abs(sh - sw) < 1) {
			vol_dim_scale_ = { 1.0f, 1.0f, sd / sh };
		}
		else {
			vol_dim_scale_ = { 1.0f, 1.0f, sd / fmax(sw, sh) };
			if (sw > sh) {
				ScaleVec3_.y *= sh / sw;
			}
			else {
				ScaleVec3_.x *= sw / sh;
			}
			volume_model_dirty = true;
		}
		vol_dim_scale_mat_ = glm::scale(glm::mat4(1.0f), vol_dim_scale_);
		texvrRenderer_->setDimension(m_deviceResources->GetD3DDevice(), vol_dimension_, vol_dim_scale_);
		cutter_->setDimension(pd, vol_dim_scale_.z);
	}

	if (tex_volume != nullptr) { delete tex_volume; tex_volume = nullptr; }

	tex_volume = new Texture;
	D3D11_TEXTURE3D_DESC texDesc{
		ph,pw,pd,
		1,
		DXGI_FORMAT_R32_UINT,
		D3D11_USAGE_DEFAULT,
		D3D11_BIND_SHADER_RESOURCE,
		0,
		0
	};
	if (!tex_volume->Initialize(m_deviceResources->GetD3DDevice(), m_deviceResources->GetD3DDeviceContext(), texDesc, data)) { delete tex_volume; tex_volume = nullptr; }
	tex_volume->GenerateMipMap(m_deviceResources->GetD3DDeviceContext());

	if (tex_baked != nullptr) { delete tex_baked; tex_baked = nullptr; }
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

	//uncomment this to use 3d texture
	/*tex_volume = new Texture;
	D3D11_TEXTURE3D_DESC texDesc{
		400,400,400,
		1,
		DXGI_FORMAT_R8G8B8A8_UNORM,
		D3D11_USAGE_DEFAULT,
		D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE,
		0,
		D3D11_RESOURCE_MISC_SHARED
	};

	auto imageSize = texDesc.Width * texDesc.Height * texDesc.Depth * 4;
	unsigned char* m_targaData = new unsigned char[imageSize];
	for (auto i = 0; i < imageSize; i += 4) {
		m_targaData[i] = 0xff;// (unsigned char)255;
		m_targaData[i + 1] = 0;
		m_targaData[i + 2] = 0;
		m_targaData[i + 3] = (unsigned char)10;
	}
	if (!tex_volume->Initialize(m_deviceResources->GetD3DDevice(), m_deviceResources->GetD3DDeviceContext(), texDesc, m_targaData)) { delete tex_volume; tex_volume = nullptr; }
	*/
	/*if (tex_baked != nullptr) { delete tex_baked; tex_baked = nullptr; }
	tex_baked = new Texture;
	D3D11_TEXTURE2D_DESC texDesc;
	texDesc.Width = screen_width;
	texDesc.Height = screen_height;
	texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	texDesc.MipLevels = 1;
	texDesc.ArraySize = 1;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	texDesc.CPUAccessFlags = 0;
	texDesc.MiscFlags = D3D11_RESOURCE_MISC_SHARED;

	auto imageSize = texDesc.Width * texDesc.Height * 4;
	unsigned char* m_targaData = new unsigned char[imageSize];
	for (int i = 0; i < imageSize; i += 4) {
		m_targaData[i] = (unsigned char)255;
		m_targaData[i + 1] = 0;
		m_targaData[i + 2] = 0;
		m_targaData[i + 3] = (unsigned char)255;
	}
	if (!tex_baked->Initialize(m_deviceResources->GetD3DDevice(),
		m_deviceResources->GetD3DDeviceContext(),
		texDesc, m_targaData)) {
		delete tex_baked;
		tex_baked = nullptr;
	}

	*/
	if (m_comp_tex_d3d != nullptr) { delete m_comp_tex_d3d; m_comp_tex_d3d = nullptr; }
	if (m_textureUAV != nullptr) { delete m_textureUAV; m_textureUAV = nullptr; }

	////Basic random texture which is shader resource and UAV
	//D3D11_TEXTURE2D_DESC texDesc2 = {};
	//texDesc2.Width = screen_width;
	//texDesc2.Height = screen_height;
	//texDesc2.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	//texDesc2.Usage = D3D11_USAGE_DEFAULT;
	//texDesc2.MipLevels = 1;
	//texDesc2.ArraySize = 1;
	//texDesc2.SampleDesc.Count = 1;
	//texDesc2.SampleDesc.Quality = 0;
	//texDesc2.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
	//texDesc2.CPUAccessFlags = 0;
	//texDesc2.MiscFlags = D3D11_RESOURCE_MISC_SHARED;

	D3D11_TEXTURE3D_DESC texDesc{
		vol_dimension_.x,vol_dimension_.y,vol_dimension_.z,
		1,
		DXGI_FORMAT_R8G8B8A8_UNORM,
		D3D11_USAGE_DEFAULT,
		D3D11_BIND_UNORDERED_ACCESS,
		0,
		0
	};

	DX::ThrowIfFailed(
		m_deviceResources->GetD3DDevice()->CreateTexture3D(&texDesc, nullptr, &m_comp_tex_d3d)
	);

	D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE3D;
	uavDesc.Format = texDesc.Format;
	uavDesc.Texture3D.MipSlice = 0;
	uavDesc.Texture3D.FirstWSlice = 0;
	uavDesc.Texture3D.WSize = vol_dimension_.z;
	DX::ThrowIfFailed(
		m_deviceResources->GetD3DDevice()->CreateUnorderedAccessView(m_comp_tex_d3d, &uavDesc, &m_textureUAV)
	);

	volume_model_dirty = true;
}

// Initializes view parameters when the window size changes.
void vrController::CreateWindowSizeDependentResources(){
	Size outputSize = m_deviceResources->GetOutputSize();
	if (outputSize.Width == .0) return;
	//screen_width = outputSize.Width; screen_height = outputSize.Height;
	//screen_quad->setQuadSize(m_deviceResources->GetD3DDevice(), m_deviceResources->GetD3DDeviceContext(), outputSize.Width, outputSize.Height);
	DX::ThrowIfFailed(screen_quad->InitializeQuadTex(m_deviceResources->GetD3DDevice(), m_deviceResources->GetD3DDeviceContext(), outputSize.Width, outputSize.Height));
}

// Called once per frame, rotates the cube and calculates the model and view matrices.
void vrController::Update(DX::StepTimer const& timer) {
	if (!m_tracking){
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
void vrController::Rotate(float radians){
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

void vrController::StopTracking(){
	m_tracking = false;
}

// Renders one frame using the vertex and pixel shaders.
void vrController::Render() {
	if (!tex_volume) return;
	if (!pre_draw_) { render_scene(); return; }

	auto context = m_deviceResources->GetD3DDeviceContext();
	//TODO:WTF..
	if (frame_num<10 || isDirty()) {
		screen_quad->SetToDrawTarget(context, m_deviceResources->GetDepthStencilView());
		render_scene();
		frame_num++;
	}
	m_deviceResources->SetBackBufferRenderTarget();
	screen_quad->Draw(context);	
	m_deviceResources->ClearCurrentDepthBuffer();
}
void vrController::getGraphPoints(float values[], float*& points) {
	DirectX::XMFLOAT2 lb, lm, lt, rb, rm, rt;
	float half_top = values[dvr::TUNE_WIDTHTOP] / 2.0f;
	float half_bottom = std::fmax(values[dvr::TUNE_WIDTHBOTTOM] / 2.0f, half_top);

	float lb_x = values[dvr::TUNE_CENTER] - half_bottom;
	float rb_x = values[dvr::TUNE_CENTER] + half_bottom;

	lb = DirectX::XMFLOAT2(lb_x, .0f);
	rb = DirectX::XMFLOAT2(rb_x, .0f);

	lt = DirectX::XMFLOAT2(values[dvr::TUNE_CENTER] - half_top, values[dvr::TUNE_OVERALL]);
	rt = DirectX::XMFLOAT2(values[dvr::TUNE_CENTER] + half_top, values[dvr::TUNE_OVERALL]);

	float mid_y = values[dvr::TUNE_LOWEST] * values[dvr::TUNE_OVERALL];
	lm = DirectX::XMFLOAT2(lb_x, mid_y);
	rm = DirectX::XMFLOAT2(rb_x, mid_y);

	//if (points) delete points;
	points = new float[12]{
			lb.x, lb.y, lm.x, lm.y, lt.x, lt.y,
			rb.x, rb.y, rm.x, rm.y, rt.x, rt.y
	};
}
void vrController::precompute() {
	if (!Manager::baked_dirty_) return;
	if (!bakeShader_) CreateDeviceDependentResources();

	auto context = m_deviceResources->GetD3DDeviceContext();
	//run compute shader
	context->CSSetShader(bakeShader_, nullptr, 0);
	ID3D11ShaderResourceView* texview = tex_volume->GetTextureView();
	context->CSSetShaderResources(0, 1, &texview);
	context->CSSetUnorderedAccessViews(0, 1, &m_textureUAV, nullptr);


	if (m_compute_constbuff != nullptr) {
		// Prepare the constant buffer to send it to the graphics device.
		m_deviceResources->GetD3DDeviceContext()->UpdateSubresource(
			m_compute_constbuff,
			0,
			nullptr,
			&m_cmpdata,
			0,
			0
		);
		context->CSSetConstantBuffers(0, 1, &m_compute_constbuff);
	}

	context->Dispatch((vol_dimension_.x+7) / 8, (vol_dimension_.y+7) / 8, (vol_dimension_.z + 7) / 8);
	context->CopyResource(tex_baked->GetTexture3D(), m_comp_tex_d3d);
	//unbind UAV
	ID3D11UnorderedAccessView* nullUAV[] = { NULL };
	context->CSSetUnorderedAccessViews(0, 1, nullUAV, 0);
	// Disable Compute Shader
	context->CSSetShader(nullptr, nullptr, 0);
	Manager::baked_dirty_ = false;
}

void vrController::render_scene(){
	precompute();
	auto context = m_deviceResources->GetD3DDeviceContext();

	if (volume_model_dirty) { updateVolumeModelMat(); volume_model_dirty = false; }

	//auto model_mat = vol_dim_scale_mat_ * ModelMat_ * SpaceMat_;
	
	auto model_mat = SpaceMat_ * ModelMat_ * vol_dim_scale_mat_;
	meshRenderer_->Draw(m_deviceResources->GetD3DDeviceContext(), tex_volume, mat42xmmatrix(model_mat));
	cutter_->Update(model_mat);
	cutter_->Draw(m_deviceResources->GetD3DDeviceContext());
	
	m_deviceResources->ClearCurrentDepthBuffer();

	if(isRayCasting())
		raycast_renderer->Draw(context, tex_baked, mat42xmmatrix(model_mat));
	else {
		auto dir = Manager::camera->getViewDirection();
		DirectX::XMFLOAT4X4 m_rot_mat;
		XMStoreFloat4x4(&m_rot_mat, mat42xmmatrix(RotateMat_));
		float front_test = m_rot_mat._33 * dir.z;
		texvrRenderer_->Draw(context, tex_baked, mat42xmmatrix(ModelMat_), front_test < 0);
	}
	m_deviceResources->ClearCurrentDepthBuffer();
	for (auto line : line_renderers_)
		//if ((mask_bits_ >> (line.first + 1)) & 1)
		line.second->Draw(context, mat42xmmatrix(model_mat));
}

void vrController::CreateDeviceDependentResources(){
	if (bakeShader_) return;
	auto loadCSTask = DX::ReadDataAsync(L"VolumeCompute.cso");
	// After the vertex shader file is loaded, create the shader and input layout.
	auto createCSTask = loadCSTask.then([this](const std::vector<byte>& fileData) {
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateComputeShader(
				&fileData[0],
				fileData.size(),
				nullptr,
				&bakeShader_
			)
		);

		CD3D11_BUFFER_DESC constantBufferDesc(sizeof(XMFLOAT4X4) *12, D3D11_BIND_CONSTANT_BUFFER);
		winrt::check_hresult(
			m_deviceResources->GetD3DDevice()->CreateBuffer(
				&constantBufferDesc,
				nullptr,
				&m_compute_constbuff
			)
		);

		//m_cmpdata.u_tex_size = { vol_dimension_.x, vol_dimension_.y, vol_dimension_.z, (UINT)0 };

		float opa_values[5] = {
			1.0f,
			.0f,
			2.0f,
			0.0f,
			1.0f
		};
		float* points;
		getGraphPoints(opa_values, points);

		for (int i = 0; i < 3; i++)m_cmpdata.u_opacity[i] = { points[4 * i], points[4 * i + 1] , points[4 * i + 2] , points[4 * i + 3] };
		m_cmpdata.u_widget_num = 1;
		m_cmpdata.u_visible_bits = 1;
		//contrast
		m_cmpdata.u_contrast_low = .0f;
		m_cmpdata.u_contrast_high = .2f;
		m_cmpdata.u_brightness = 0.5f;
		//mask
		m_cmpdata.u_maskbits = 0xffff - 1;//mask_bits_;
		m_cmpdata.u_organ_num = 7;//mask_num
		m_cmpdata.u_mask_color = 1;
		//others
		m_cmpdata.u_flipy = 0;
		m_cmpdata.u_show_organ = 1;
		m_cmpdata.u_color_scheme = 2;

	});
}
void vrController::onSingleTouchDown(float x, float y) {
	Mouse_old = { x, y };
	m_IsPressed = true;
}

void vrController::onSingle3DTouchDown(float x, float y, float z, int side) {
	//char debug[256];
	//sprintf(debug, "Touch %d: %f,%f,%f\n", side, x, y, z);
	//OutputDebugStringA(debug);
	if(side == 0) {
		Mouse3D_old_left = { x, y, z };
		m_IsPressed_left = true;
		if(m_IsPressed_right) {
		  // record distance for scaling
			glm::vec3 delta = { x = Mouse3D_old_right.x , y - Mouse3D_old_right.y, z - Mouse3D_old_right.z };
			distance_old = glm::length(delta);
			vector_old = delta;

		}
	}else {
		Mouse3D_old_right = { x, y, z };
		m_IsPressed_right = true;
		if (m_IsPressed_left) {
			// record distance for scaling
			glm::vec3 delta = { x - Mouse3D_old_left.x , y - Mouse3D_old_left.y, z - Mouse3D_old_left.z };
			distance_old = glm::length(delta);
			vector_old = delta;
		}
	}

}

void vrController::onTouchMove(float x, float y) {
	if (!m_IsPressed || !tex_volume) return;

	//if (!Manager::param_bool[dvr::CHECK_CUTTING] && Manager::param_bool[dvr::CHECK_FREEZE_VOLUME]) return;

	//if (raycastRenderer_)isRayCasting() ? raycastRenderer_->dirtyPrecompute() : texvrRenderer_->dirtyPrecompute();
	float xoffset = x - Mouse_old.x, yoffset = Mouse_old.y - y;
	Mouse_old = { x, y };
	xoffset *= dvr::MOUSE_ROTATE_SENSITIVITY;
	yoffset *= -dvr::MOUSE_ROTATE_SENSITIVITY;

	/*if (Manager::param_bool[dvr::CHECK_FREEZE_VOLUME]) {
		cutter_->onRotate(xoffset, yoffset);
		return;
	}*/
	RotateMat_ = mouseRotateMat(RotateMat_, xoffset, yoffset);
	volume_model_dirty = true;
}

void vrController::on3DTouchMove(float x, float y, float z, int side) {

	if (m_IsPressed_left) {
	  if(side == 0) {
			//char debug[256];
			//sprintf(debug, "Touch %d: %f,%f,%f\n", side, x, y, z);
			//OutputDebugStringA(debug);
	  }
	}
	if (m_IsPressed_right) {
	  if(side == 1) {
			//char debug[256];
			//sprintf(debug, "Touch %d: %f,%f,%f\n", side, x, y, z);
			//OutputDebugStringA(debug);
	  }
	}

  if(!(m_IsPressed_left && m_IsPressed_right)) {
		// not both hand: translate
    if(side == 0) {
      if(m_IsPressed_left) {
				float dx = x - Mouse3D_old_left.x;
				float dy = y - Mouse3D_old_left.y;
				float dz = z - Mouse3D_old_left.z;

				PosVec3_.x += dx * sens;
				PosVec3_.y += dy * sens;
				PosVec3_.z += dz * sens;

				Mouse3D_old_left = { x, y, z };
				volume_model_dirty = true;
      }
    }else {
			if (m_IsPressed_right) {
				float dx = x - Mouse3D_old_right.x;
				float dy = y - Mouse3D_old_right.y;
				float dz = z - Mouse3D_old_right.z;

				PosVec3_.x += dx * sens;
				PosVec3_.y += dy * sens;
				PosVec3_.z += dz * sens;

				Mouse3D_old_right = { x, y, z };
				volume_model_dirty = true;
			}
    }
  }else {
    // both hand: scale
		glm::vec3 delta;
		glm::vec3 midpoint;
		if (side == 0) {
			delta.x = x - Mouse3D_old_right.x;
			delta.y = y - Mouse3D_old_right.y;
			delta.z = z - Mouse3D_old_right.z;

			float distance = glm::length(delta);

			// Scale
			float ratio = distance / distance_old;

			// Midpoint
			midpoint = glm::vec3{ (x + Mouse3D_old_right.x) / 2.0f, (y + Mouse3D_old_right.y) / 2.0f, (z + Mouse3D_old_right.z) / 2.0f };

			if (ratio > 0.8f && ratio < 1.2f) {
				uniScale *= ratio;
			}

			distance_old = distance;
			Mouse3D_old_left = { x, y, z };
			volume_model_dirty = true;
		}
		else {
			delta.x = Mouse3D_old_left.x - x;
			delta.y = Mouse3D_old_left.y - y;
			delta.z = Mouse3D_old_left.z - z;

			float distance = glm::length(delta);

			// Scale
			float ratio = distance / distance_old;

			// Midpoint
			midpoint = glm::vec3{ (x + Mouse3D_old_left.x) / 2.0f, (y + Mouse3D_old_left.y) / 2.0f, (z + Mouse3D_old_left.z) / 2.0f };

			if (ratio > 0.8f && ratio < 1.2f) {
				uniScale *= ratio;
			}

			distance_old = distance;
			Mouse3D_old_right = { x, y, z };
			volume_model_dirty = true;
		}

		// move
		glm::vec3 mid_delta = glm::vec3{ midpoint.x - Mouse3D_old_mid.x, midpoint.y - Mouse3D_old_mid.y, midpoint.z - Mouse3D_old_mid.z };
		if (glm::length(mid_delta) < 0.01f) {
			PosVec3_.x += mid_delta.x;
			PosVec3_.y += mid_delta.y;
			PosVec3_.z += mid_delta.z;
		}

		Mouse3D_old_mid = { midpoint.x , midpoint.y , midpoint.z };

		// rotate
		glm::vec3 curr = glm::normalize(glm::vec3{ delta.x, 0, delta.z });
		glm::vec3 old = glm::normalize(glm::vec3{ vector_old.x, 0, vector_old.z });
		glm::vec3 axis = glm::normalize(glm::cross(old, curr));
		float angle = std::acos(glm::dot(old, curr));


		if (angle < 0.5f && angle > 0.01f) {
			glm::mat4 rot = glm::toMat4(glm::angleAxis(angle, axis));

			//char debug[256];
			//sprintf(debug, "Touch %f\n", angle);
			//OutputDebugStringA(debug);

			RotateMat_ *= rot;
		}
		vector_old = glm::normalize(delta);
  }

}

void vrController::onTouchReleased(){
	m_IsPressed = false;
}

void vrController::on3DTouchReleased(int side) {
	//char debug[256];
	if (side == 0) {
		//sprintf(debug, "Release %d: %f,%f,%f\n", side, Mouse3D_old_left.x, Mouse3D_old_left.y, Mouse3D_old_left.z);
	}else {
		//sprintf(debug, "Release %d: %f,%f,%f\n", side, Mouse3D_old_right.x, Mouse3D_old_right.y, Mouse3D_old_right.z);
	}
	//OutputDebugStringA(debug);
	if (side == 0) {
		m_IsPressed_left = false;
	}
	else {
		m_IsPressed_right = false;
	}
}

void vrController::onScale(float sx, float sy) {

}

void vrController::onScale(float scale) {
  
}

void vrController::onPan(float x, float y) {

}
void vrController::updateVolumeModelMat() {
	ModelMat_ = glm::translate(glm::mat4(1.0), PosVec3_)
		* RotateMat_
		* glm::scale(glm::mat4(1.0), ScaleVec3_)
    * glm::scale(glm::mat4(1.0), glm::vec3(uniScale, uniScale, uniScale));
}

bool vrController::addStatus(std::string name, glm::mat4 mm, glm::mat4 rm, glm::vec3 sv, glm::vec3 pv, Camera* cam) {
	auto it = rStates_.find(name);
	if (it != rStates_.end()) return false;

	rStates_[name] = reservedStatus(mm, rm, sv, pv, cam);
	if (Manager::screen_w != 0) rStates_[name].vcam->setProjMat(Manager::screen_w, Manager::screen_h);
	return true;
}
bool vrController::addStatus(std::string name, bool use_current_status) {
	auto it = rStates_.find(name);
	if (it != rStates_.end()) return false;

	if (use_current_status) {
		if (volume_model_dirty) {
			updateVolumeModelMat();
			volume_model_dirty = false;
		}
		rStates_[name] = reservedStatus(ModelMat_, RotateMat_, ScaleVec3_, PosVec3_, new Camera(name.c_str()));
	}
	else {
	  rStates_[name] = reservedStatus();
		rStates_[name].pos_vec = PosVec3_;
	}

	if (Manager::screen_w != 0){
		rStates_[name].vcam->setViewMat(Manager::camera->getViewMat());
		//rStates_[name].vcam->setProjMat(Manager::screen_w, Manager::screen_h);
	}
	return true;
}
void vrController::setMVPStatus(std::string name) {
	if (name == cst_name) return;
	auto rstate_ = rStates_[name];
	ModelMat_ = rstate_.model_mat; RotateMat_ = rstate_.rot_mat; ScaleVec3_ = rstate_.scale_vec; PosVec3_ = rstate_.pos_vec; Manager::camera = rstate_.vcam;
	volume_model_dirty = false;
	cst_name = name;
}
void vrController::setupCenterLine(int id, float* data) {
	int oid = 0;
	while (id /= 2)oid++;
	if (line_renderers_.count(oid))
		line_renderers_[oid]->updateVertices(m_deviceResources->GetD3DDevice(), 4000, data);
	else
		line_renderers_[oid] = new lineRenderer(m_deviceResources->GetD3DDevice(), oid, 4000, data);
	//cutter_->setupCenterLine((dvr::ORGAN_IDS)oid, data);
}

//void vrController::setSpaces(XrSpace * space, XrSpace * app_space) {
//  this->space = space;
//	this->app_space = app_space;
//}

void vrController::ReleaseDeviceDependentResources(){
	raycast_renderer->Clear();
	//screen_quad->Clear();
	//texture
	if (tex_volume) delete tex_volume;
	if (tex_baked) delete tex_baked;
	rStates_.clear();
}

bool vrController::isDirty() {
	if (!tex_volume) return false;
	if (volume_model_dirty || Manager::baked_dirty_) {
		//meshRenderer_->dirtyPrecompute();
		return true;
	}
	/*if (Manager::IsCuttingNeedUpdate() && cutter_->isPrecomputeDirty()) {
		meshRenderer_->dirtyPrecompute();
		if (isRayCasting())raycastRenderer_->dirtyPrecompute();
		else texvrRenderer_->dirtyPrecompute();
		return true;
	}
	if (Manager::param_bool[dvr::CHECK_VOLUME_ON]) {
		if (isRayCasting()) return raycastRenderer_->isPrecomputeDirty();
		return texvrRenderer_->isPrecomputeDirty();
	}*/
	return false;
}
