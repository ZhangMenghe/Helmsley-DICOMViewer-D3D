#include "pch.h"
#include "vrController.h"
#include <Common/Manager.h>
#include <Utils/MathUtils.h>
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

	screen_quad = new quadRenderer(m_deviceResources->GetD3DDevice());
	raycast_renderer = new raycastVolumeRenderer(m_deviceResources->GetD3DDevice());

	Manager::camera = new Camera;
	CreateDeviceDependentResources();
	CreateWindowSizeDependentResources();
	onReset();
}
void vrController::onReset() {
	Mouse_old = { .0f, .0f };
	rStates_.clear();
	cst_name = "";
	addStatus("default_status");
	setMVPStatus("default_status");
}

void vrController::onReset(DirectX::XMFLOAT3 pv, DirectX::XMFLOAT3 sv, DirectX::XMFLOAT4X4 rm, Camera* cam) {
	Mouse_old = { .0f, .0f };
	rStates_.clear();
	cst_name = "";
	DirectX::XMMATRIX mrot = DirectX::XMLoadFloat4x4(&rm);

	XMMATRIX mmodel =
		DirectX::XMMatrixScaling(sv.x, sv.y, sv.z)
		* mrot
		* DirectX::XMMatrixTranslation(pv.x, pv.y, pv.z);

	addStatus("template", mmodel, mrot, sv, pv, cam);
	setMVPStatus("template");

	volume_model_dirty = false;
}

void vrController::assembleTexture(int update_target, int ph, int pw, int pd, float sh, float sw, float sd, UCHAR* data, int channel_num) {
	if (update_target == 0 || update_target == 2) {
		vol_dimension_ = { ph, pw, pd };
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
		vol_dim_scale_mat_ = DirectX::XMMatrixScaling(vol_dim_scale_.x, vol_dim_scale_.y, vol_dim_scale_.z);
		//texvrRenderer_->setDimension(vol_dimension_, vol_dim_scale_);
		//cutter_->setDimension(pd, vol_dim_scale_.z);
	}
	
	if (tex_volume != nullptr) { delete tex_volume; tex_volume = nullptr; }

	tex_volume = new Texture;
	D3D11_TEXTURE3D_DESC texDesc{
		ph,pw,pd,
		1,
		DXGI_FORMAT_R8G8B8A8_UNORM,
		D3D11_USAGE_DEFAULT,
		D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE,
		0,
		D3D11_RESOURCE_MISC_SHARED
	};
	if (!tex_volume->Initialize(m_deviceResources->GetD3DDevice(), m_deviceResources->GetD3DDeviceContext(), texDesc, data)) { delete tex_volume; tex_volume = nullptr; }

	tex_baked = new Texture;
	tex_baked->Initialize(m_deviceResources->GetD3DDevice(), texDesc);
	Manager::baked_dirty_ = true;
}

void vrController::init_texture() {
	//uncomment this to use 3d texture
	tex_volume = new Texture;
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
	
	/*if (tex2d_srv_from_uav != nullptr) { delete tex2d_srv_from_uav; tex2d_srv_from_uav = nullptr; }
	tex2d_srv_from_uav = new Texture;
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
	if (!tex2d_srv_from_uav->Initialize(m_deviceResources->GetD3DDevice(),
		m_deviceResources->GetD3DDeviceContext(),
		texDesc, m_targaData)) {
		delete tex2d_srv_from_uav;
		tex2d_srv_from_uav = nullptr;
	}


	if (m_comp_tex_d3d != nullptr) { delete m_comp_tex_d3d; m_comp_tex_d3d = nullptr; }
	if (m_textureUAV != nullptr) { delete m_textureUAV; m_textureUAV = nullptr; }

	//Basic random texture which is shader resource and UAV
	D3D11_TEXTURE2D_DESC texDesc2 = {};
	texDesc2.Width = screen_width;
	texDesc2.Height = screen_height;
	texDesc2.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	texDesc2.Usage = D3D11_USAGE_DEFAULT;
	texDesc2.MipLevels = 1;
	texDesc2.ArraySize = 1;
	texDesc2.SampleDesc.Count = 1;
	texDesc2.SampleDesc.Quality = 0;
	texDesc2.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
	texDesc2.CPUAccessFlags = 0;
	texDesc2.MiscFlags = D3D11_RESOURCE_MISC_SHARED;

	DX::ThrowIfFailed(
		m_deviceResources->GetD3DDevice()->CreateTexture2D(&texDesc2, nullptr, &m_comp_tex_d3d)
	);

	D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
	uavDesc.Format = texDesc2.Format;
	uavDesc.Texture2D.MipSlice = 0;
	DX::ThrowIfFailed(
		m_deviceResources->GetD3DDevice()->CreateUnorderedAccessView(m_comp_tex_d3d, &uavDesc, &m_textureUAV)
	);
	*/
}

// Initializes view parameters when the window size changes.
void vrController::CreateWindowSizeDependentResources(){
	Size outputSize = m_deviceResources->GetOutputSize();
	if (outputSize.Width == .0) return;
	screen_width = outputSize.Width; screen_height = outputSize.Height;
	screen_quad->setQuadSize(m_deviceResources->GetD3DDevice(), m_deviceResources->GetD3DDeviceContext(), outputSize.Width, outputSize.Height);
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
	/*
	// Loading is asynchronous. Only draw geometry after it's loaded.
	if (!m_loadingComplete)
	{
		return;
	}
	if (!m_render_to_texture) { render_scene(); return; }
	
	auto context = m_deviceResources->GetD3DDeviceContext();
	auto tview = screen_quad->GetRenderTargetView();
	
	context->OMSetRenderTargets(1, &tview, m_deviceResources->GetDepthStencilView());
	// Clear the render to texture.
	context->ClearRenderTargetView(tview, m_clear_color);
	context->ClearDepthStencilView(m_deviceResources->GetDepthStencilView(), D3D11_CLEAR_DEPTH, 1.0f, 0);
	render_scene();
	m_deviceResources->SetBackBufferRenderTarget();
	context->ClearRenderTargetView(m_deviceResources->GetBackBufferRenderTargetView(), m_clear_color);
	
	//draw to screen
	*/
	//screen_quad->Draw(m_deviceResources->GetD3DDeviceContext());
	render_scene();
}
void vrController::render_scene(){
	if (volume_model_dirty) { updateVolumeModelMat(); volume_model_dirty = false; }

	auto model_mat = ModelMat_ * vol_dim_scale_mat_;

	raycast_renderer->Draw(m_deviceResources->GetD3DDeviceContext(), tex_volume, model_mat);
	/*
	/*auto context = m_deviceResources->GetD3DDeviceContext();

	// Set shader texture resource in the pixel shader.
	if (texture != nullptr) {
		ID3D11ShaderResourceView* texview = texture->GetTextureView();
		context->PSSetShaderResources(0, 1, &texview);
	}

	//compute shader stuff
	context->CSSetShader(m_computeShader, nullptr, 0);
	context->CSSetUnorderedAccessViews(0, 1, &m_textureUAV, nullptr);
	context->Dispatch(screen_width / 8, screen_height / 8, 1);
	context->CopyResource(tex2d_srv_from_uav->GetTexture2D(), m_comp_tex_d3d);
	//unbind UAV
	ID3D11UnorderedAccessView* nullUAV[] = { NULL };
	context->CSSetUnorderedAccessViews(0, 1, nullUAV, 0);
	// Disable Compute Shader
	context->CSSetShader(nullptr, nullptr, 0);

	if (tex2d_srv_from_uav != nullptr) {
		ID3D11ShaderResourceView* texview = tex2d_srv_from_uav->GetTextureView();
		context->PSSetShaderResources(0, 1, &texview);
	}


	// Prepare the constant buffer to send it to the graphics device.
	context->UpdateSubresource1(
		m_constantBuffer.Get(),
		0,
		NULL,
		&m_constantBufferData,
		0,
		0,
		0
		);

	// Each vertex is one instance of the VertexPositionColor struct.
	UINT stride = sizeof(VertexPositionColor);
	UINT offset = 0;
	context->IASetVertexBuffers(
		0,
		1,
		m_vertexBuffer.GetAddressOf(),
		&stride,
		&offset
		);

	context->IASetIndexBuffer(
		m_indexBuffer.Get(),
		DXGI_FORMAT_R16_UINT, // Each index is one 16-bit unsigned integer (short).
		0
		);

	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	context->IASetInputLayout(m_inputLayout.Get());

	// Attach our vertex shader.
	context->VSSetShader(
		m_vertexShader.Get(),
		nullptr,
		0
		);

	// Send the constant buffer to the graphics device.
	context->VSSetConstantBuffers1(
		0,
		1,
		m_constantBuffer.GetAddressOf(),
		nullptr,
		nullptr
	);

	// Attach our pixel shader.
	context->PSSetShader(
		m_pixelShader.Get(),
		nullptr,
		0
		);
	//texture sampler
	context->PSSetSamplers(0, 1, &m_sampleState);

	// Draw the objects.
	context->DrawIndexed(
		36,
		0,
		0
		);
		*/
}

void vrController::CreateDeviceDependentResources()
{
	/*
	// Load shaders asynchronously.
	auto loadVSTask = DX::ReadDataAsync(L"SampleVertexShader.cso");
	auto loadPSTask = DX::ReadDataAsync(L"SamplePixelShader.cso");
	auto loadCSTask = DX::ReadDataAsync(L"NaiveComputeShader.cso");

	// After the vertex shader file is loaded, create the shader and input layout.
	auto createVSTask = loadVSTask.then([this](const std::vector<byte>& fileData) {
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateVertexShader(
				&fileData[0],
				fileData.size(),
				nullptr,
				&m_vertexShader
				)
			);

		static const D3D11_INPUT_ELEMENT_DESC vertexDesc [] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};

		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateInputLayout(
				vertexDesc,
				ARRAYSIZE(vertexDesc),
				&fileData[0],
				fileData.size(),
				&m_inputLayout
				)
			);
	});

	// After the pixel shader file is loaded, create the shader and constant buffer.
	auto createPSTask = loadPSTask.then([this](const std::vector<byte>& fileData) {
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreatePixelShader(
				&fileData[0],
				fileData.size(),
				nullptr,
				&m_pixelShader
				)
			);
		//texture
		D3D11_SAMPLER_DESC samplerDesc;

		// Create a texture sampler state description.
		samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.MipLODBias = 0.0f;
		samplerDesc.MaxAnisotropy = 1;
		samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
		samplerDesc.BorderColor[0] = 0;
		samplerDesc.BorderColor[1] = 0;
		samplerDesc.BorderColor[2] = 0;
		samplerDesc.BorderColor[3] = 0;
		samplerDesc.MinLOD = 0;
		samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

		// Create the texture sampler state.
		DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateSamplerState(&samplerDesc, &m_sampleState));

		CD3D11_BUFFER_DESC constantBufferDesc(sizeof(ModelViewProjectionConstantBuffer) , D3D11_BIND_CONSTANT_BUFFER);
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateBuffer(
				&constantBufferDesc,
				nullptr,
				&m_constantBuffer
				)
			);
	});

	// After the vertex shader file is loaded, create the shader and input layout.
	auto createCSTask = loadCSTask.then([this](const std::vector<byte>& fileData) {
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateComputeShader(
				&fileData[0],
				fileData.size(),
				nullptr,
				&m_computeShader
			)
		);

	});

	// Once both shaders are loaded, create the mesh.
	auto createCubeTask = (createPSTask && createVSTask && createCSTask).then([this] () {
		D3D11_SUBRESOURCE_DATA vertexBufferData = {0};
		vertexBufferData.pSysMem = cube_vertices_pos_w_tex;
		vertexBufferData.SysMemPitch = 0;
		vertexBufferData.SysMemSlicePitch = 0;
		CD3D11_BUFFER_DESC vertexBufferDesc(sizeof(cube_vertices_pos_w_tex), D3D11_BIND_VERTEX_BUFFER);
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateBuffer(
				&vertexBufferDesc,
				&vertexBufferData,
				&m_vertexBuffer
				)
			);

		D3D11_SUBRESOURCE_DATA indexBufferData = {0};
		indexBufferData.pSysMem = cube_indices;
		indexBufferData.SysMemPitch = 0;
		indexBufferData.SysMemSlicePitch = 0;
		CD3D11_BUFFER_DESC indexBufferDesc(sizeof(cube_indices), D3D11_BIND_INDEX_BUFFER);
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateBuffer(
				&indexBufferDesc,
				&indexBufferData,
				&m_indexBuffer
				)
			);
	});

	// Once the cube is loaded, the object is ready to be rendered.
	createCubeTask.then([this] () {
		m_loadingComplete = true;
	});
	*/
}
void vrController::onSingleTouchDown(float x, float y) {
	Mouse_old = { x, y };
	m_IsPressed = true;
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
void vrController::onTouchReleased(){
	m_IsPressed = false;
}
void vrController::onScale(float sx, float sy) {

}
void vrController::onPan(float x, float y) {

}
void vrController::updateVolumeModelMat() {
	ModelMat_ =
		DirectX::XMMatrixScaling(ScaleVec3_.x, ScaleVec3_.y, ScaleVec3_.z)
		* RotateMat_
		* DirectX::XMMatrixTranslation(PosVec3_.x, PosVec3_.y, PosVec3_.z);
}

bool vrController::addStatus(std::string name, DirectX::XMMATRIX mm, DirectX::XMMATRIX rm, DirectX::XMFLOAT3 sv, DirectX::XMFLOAT3 pv, Camera* cam) {
	auto it = rStates_.find(name);
	if (it != rStates_.end()) return false;

	rStates_[name] = new reservedStatus(mm, rm, sv, pv, cam);
	if (Manager::screen_w != 0) rStates_[name]->vcam->setProjMat(Manager::screen_w, Manager::screen_h);
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
		rStates_[name] = new reservedStatus(ModelMat_, RotateMat_, ScaleVec3_, PosVec3_, new Camera(name.c_str()));
	}
	else rStates_[name] = new reservedStatus();
	if (Manager::screen_w != 0)rStates_[name]->vcam->setProjMat(Manager::screen_w, Manager::screen_h);
	return true;
}
void vrController::setMVPStatus(std::string status_name) {
	if (status_name == cst_name) return;
	auto rstate_ = rStates_[status_name];
	ModelMat_ = DirectX::XMLoadFloat4x4(&rstate_->model_mat);
	RotateMat_ = DirectX::XMLoadFloat4x4(&rstate_->rot_mat);
	ScaleVec3_ = rstate_->scale_vec; PosVec3_ = rstate_->pos_vec; 
	Manager::camera = rstate_->vcam;
	volume_model_dirty = false;
	cst_name = status_name;
}
void vrController::ReleaseDeviceDependentResources(){
	raycast_renderer->Clear();
	screen_quad->Clear();
	//texture
	if (tex_volume) delete tex_volume;
	if (tex_baked) delete tex_baked;
	rStates_.clear();
}