#include "pch.h"
#include "quadRenderer.h"
#include "Common/DirectXHelper.h"
#include <D3DPipeline/Primitive.h>

using namespace DirectX;
quadRenderer::quadRenderer(ID3D11Device* device)
:m_device_ref(device),
m_loadingComplete(false){
	// Load shaders asynchronously.
	auto loadVSTask = DX::ReadDataAsync(L"QuadVertexShader.cso");
	auto loadPSTask = DX::ReadDataAsync(L"QuadPixelShader.cso");

	// After the vertex shader file is loaded, create the shader and input layout.
	auto createVSTask = loadVSTask.then([this](const std::vector<byte>& fileData) {
		DX::ThrowIfFailed(
			m_device_ref->CreateVertexShader(
				&fileData[0],
				fileData.size(),
				nullptr,
				&m_vertexShader
			)
		);

		static const D3D11_INPUT_ELEMENT_DESC vertexDesc[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};

		DX::ThrowIfFailed(
			m_device_ref->CreateInputLayout(
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
			m_device_ref->CreatePixelShader(
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
		DX::ThrowIfFailed(m_device_ref->CreateSamplerState(&samplerDesc, &m_sampleState));

		CD3D11_BUFFER_DESC constantBufferDesc(sizeof(ModelViewProjectionConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);
		DX::ThrowIfFailed(
			m_device_ref->CreateBuffer(
				&constantBufferDesc,
				nullptr,
				&m_constantBuffer
			)
		);
	});


	// Once both shaders are loaded, create the mesh.
	auto createQuadTask = (createPSTask && createVSTask).then([this]() {
		D3D11_SUBRESOURCE_DATA vertexBufferData = { 0 };
		vertexBufferData.pSysMem = quad_vertices_pos_w_tex;
		vertexBufferData.SysMemPitch = 0;
		vertexBufferData.SysMemSlicePitch = 0;
		CD3D11_BUFFER_DESC vertexBufferDesc(sizeof(quad_vertices_pos_w_tex), D3D11_BIND_VERTEX_BUFFER);
		DX::ThrowIfFailed(
			m_device_ref->CreateBuffer(
				&vertexBufferDesc,
				&vertexBufferData,
				&m_vertexBuffer
			)
		);

		D3D11_SUBRESOURCE_DATA indexBufferData = { 0 };
		indexBufferData.pSysMem = quad_indices;
		indexBufferData.SysMemPitch = 0;
		indexBufferData.SysMemSlicePitch = 0;
		CD3D11_BUFFER_DESC indexBufferDesc(sizeof(quad_indices), D3D11_BIND_INDEX_BUFFER);
		DX::ThrowIfFailed(
			m_device_ref->CreateBuffer(
				&indexBufferDesc,
				&indexBufferData,
				&m_indexBuffer
			)
		);
	});

	createQuadTask.then([this]() {
		m_loadingComplete = true;
	});
}
bool quadRenderer::setQuadSize(ID3D11Device* device, ID3D11DeviceContext* context, float width, float height){
	texture = new Texture;
	D3D11_TEXTURE2D_DESC texDesc;
	texDesc.Width = width;
	texDesc.Height = height;
	texDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	texDesc.MipLevels = 1;
	texDesc.ArraySize = 1;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	texDesc.CPUAccessFlags = 0;
	texDesc.MiscFlags = D3D11_RESOURCE_MISC_SHARED;


	D3D11_RENDER_TARGET_VIEW_DESC view_desc{
		texDesc.Format,
		D3D11_RTV_DIMENSION_TEXTURE2D,
	};
	view_desc.Texture2D.MipSlice = 0;
	if (!texture->Initialize(device, texDesc, view_desc)) { delete texture; texture = nullptr; return false; }
	
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	context->Map(texture->GetTexture2D(), 0, D3D11_MAP_READ, 0, &mappedResource);



	//
	float aspectRatio = width / height;
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
	XMMATRIX perspectiveMatrix = XMMatrixPerspectiveFovRH(
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
		&m_constantBufferData.projection,
		XMMatrixTranspose(perspectiveMatrix * orientationMatrix)
	);

	// Eye is at (0,0.7,1.5), looking at point (0,-0.1,0) with the up-vector along the y-axis.
	static const XMVECTORF32 eye = { 0.0f, 0.7f, 1.5f, 0.0f };
	static const XMVECTORF32 at = { 0.0f, -0.1f, 0.0f, 0.0f };
	static const XMVECTORF32 up = { 0.0f, 1.0f, 0.0f, 0.0f };

	XMStoreFloat4x4(&m_constantBufferData.view, XMMatrixTranspose(XMMatrixLookAtRH(eye, at, up)));

	return true;
}

// Renders one frame using the vertex and pixel shaders.
void quadRenderer::Draw(ID3D11DeviceContext* context) {
	// Loading is asynchronous. Only draw geometry after it's loaded.
	if (!m_loadingComplete)
	{
		return;
	}
	if (texture != nullptr) {
		ID3D11ShaderResourceView* texview = texture->GetTextureView();
		context->PSSetShaderResources(0, 1, &texview);
	}
	// Prepare the constant buffer to send it to the graphics device.
	context->UpdateSubresource(
		m_constantBuffer,
		0,
		NULL,
		&m_constantBufferData,
		0,
		0
		);

	// Each vertex is one instance of the VertexPositionColor struct.
	UINT stride = sizeof(VertexPosTex2d);
	UINT offset = 0;
	context->IASetVertexBuffers(
		0,
		1,
		&m_vertexBuffer,
		&stride,
		&offset
		);

	context->IASetIndexBuffer(
		m_indexBuffer,
		DXGI_FORMAT_R16_UINT, // Each index is one 16-bit unsigned integer (short).
		0
		);

	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	context->IASetInputLayout(m_inputLayout);

	// Attach our vertex shader.
	context->VSSetShader(
		m_vertexShader,
		nullptr,
		0
		);

	// Send the constant buffer to the graphics device.
	context->VSSetConstantBuffers(
		0,
		1,
		&m_constantBuffer
	);

	// Attach our pixel shader.
	context->PSSetShader(
		m_pixelShader,
		nullptr,
		0
		);
	//texture sampler
	context->PSSetSamplers(0, 1, &m_sampleState);

	// Draw the objects.
	context->DrawIndexed(
		6,
		0,
		0
		);
}