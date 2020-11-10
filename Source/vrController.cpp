﻿#include "pch.h"
#include "vrController.h"
#include <Common/Manager.h>
#include <iostream>
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
}
void vrController::assembleTexture(int update_target, int ph, int pw, int pd, float sh, float sw, float sd, UCHAR* data, int channel_num) {
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
	
	//init_texture();
	/*float aspectRatio = outputSize.Width / outputSize.Height;
	float fovAngleY = 70.0f * XM_PI / 180.0f;

	// This is a simple example of change that can be made when the app is in
	// portrait or snapped view.
	if (aspectRatio < 1.0f)
	{
		fovAngleY *= 2.0f;
	}

	// Note that the OrientationTransform3D matrix is post-multiplied here
	// in order to correctly orient the scene to match the display orientation.
	// This post-multiplication step is required for any draw calls that are
	// made to the swap chain render target. For draw calls to other targets,
	// this transform should not be applied.

	// This sample makes use of a right-handed coordinate system using row-major matrices.
	XMMATRIX perspectiveMatrix = DirectX::XMMatrixPerspectiveFovRH(
		fovAngleY,
		aspectRatio,
		0.01f,
		100.0f
		);
	
	const XMFLOAT4X4 orientation(
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	);

	//XMFLOAT4X4 orientation = m_deviceResources->GetOrientationTransform3D();

	XMMATRIX orientationMatrix = XMLoadFloat4x4(&orientation);

	XMStoreFloat4x4(
		&m_all_buff_Data.projection,
		DirectX::XMMatrixTranspose(perspectiveMatrix * orientationMatrix)
		);

	// Eye is at (0,0.7,1.5), looking at point (0,-0.1,0) with the up-vector along the y-axis.
	static const XMVECTORF32 eye = { 0.0f, 0.f, 1.5f, 0.0f };
	static const XMVECTORF32 at = { 0.0f, 0.f, .0f, 0.0f };//{ 0.0f, -0.1f, 0.0f, 0.0f };
	static const XMVECTORF32 up = { 0.0f, 1.0f, 0.0f, 0.0f };
	auto model_mat = DirectX::XMMatrixIdentity();
	XMStoreFloat4x4(&m_all_buff_Data.model, model_mat);
	XMStoreFloat4x4(&m_all_buff_Data.view, DirectX::XMMatrixTranspose(DirectX::XMMatrixLookAtRH(eye, at, up)));
	XMStoreFloat4(&m_all_buff_Data.uCamPosInObjSpace, DirectX::XMVector4Transform(eye, DirectX::XMMatrixInverse(nullptr, model_mat)));
	screen_quad->setQuadSize(m_deviceResources->GetD3DDevice(), m_deviceResources->GetD3DDeviceContext(), outputSize.Width, outputSize.Height);
	screen_quad->updateMatrix(m_all_buff_Data);
	raycast_renderer->updateMatrix(m_all_buff_Data);*/
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

void vrController::StopTracking()
{
	m_tracking = false;
}

// Renders one frame using the vertex and pixel shaders.
void vrController::Render() {
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
	raycast_renderer->Draw(m_deviceResources->GetD3DDeviceContext(), tex_volume);
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
}
void vrController::onTouchMove(float x, float y) {

}
void vrController::onTouchReleased(){}
void vrController::onScale(float sx, float sy) {

}
void vrController::onPan(float x, float y) {

}

void vrController::ReleaseDeviceDependentResources(){
	raycast_renderer->Clear();
	screen_quad->Clear();
	//texture
	if (tex_volume) delete tex_volume;
	if (tex_baked) delete tex_baked;
}