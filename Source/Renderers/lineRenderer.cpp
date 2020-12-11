#include "pch.h"
#include "lineRenderer.h"
#include <Common/DirectXHelper.h>
#include <Common/Manager.h>
using namespace DirectX;
lineRenderer::lineRenderer(ID3D11Device* device, int uid)
:baseRenderer(device, L"Naive3DVertexShader.cso", L"NaiveColorPixelShader.cso"),
m_uid(uid){
	D3D11_BLEND_DESC omDesc;
	ZeroMemory(&omDesc, sizeof(D3D11_BLEND_DESC));
	omDesc.RenderTarget[0].BlendEnable = TRUE;
	omDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	omDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
	omDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	omDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_SRC_ALPHA;// D3D11_BLEND_ONE;// D3D11_BLEND_ONE;
	omDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;// D3D11_BLEND_ZERO;
	omDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	omDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	omDesc.AlphaToCoverageEnable = false;
	device->CreateBlendState(&omDesc, &d3dBlendState);
}
lineRenderer::lineRenderer(ID3D11Device* device, int uid, int point_num, const float* data)
	: lineRenderer(device, uid) {
	updateVertices(device, point_num, data);
}

void lineRenderer::updateVertices(ID3D11Device* device, int point_num, const float* data) {
	m_vertice_count = point_num;
	D3D11_SUBRESOURCE_DATA vertexBufferData = { 0 };
	vertexBufferData.pSysMem = data;
	vertexBufferData.SysMemPitch = 0;
	vertexBufferData.SysMemSlicePitch = 0;
	CD3D11_BUFFER_DESC vertexBufferDesc(m_vertice_count * sizeof(dvr::VertexPos3d), D3D11_BIND_VERTEX_BUFFER);
	winrt::check_hresult(
		device->CreateBuffer(
			&vertexBufferDesc,
			&vertexBufferData,
			m_vertexBuffer.put()
		)
	);
}

// Renders one frame using the vertex and pixel shaders.
void lineRenderer::	Draw(ID3D11DeviceContext* context, DirectX::XMMATRIX modelMat){
	if (!m_loadingComplete) return; 
	if (m_constantBuffer != nullptr) {
		XMStoreFloat4x4(&m_constantBufferData.uViewProjMat, Manager::camera->getVPMat());
		XMStoreFloat4x4(&m_constantBufferData.model, XMMatrixTranspose(modelMat));

		// Prepare the constant buffer to send it to the graphics device.
		context->UpdateSubresource(
			m_constantBuffer.get(),
			0,
			nullptr,
			&m_constantBufferData,
			0,
			0
		);
	}
	context->OMSetBlendState(d3dBlendState, 0, 0xffffffff);

	baseRenderer::Draw(context, D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP);
	context->OMSetBlendState(nullptr, 0, 0xffffffff);

}
void lineRenderer::create_vertex_shader(ID3D11Device* device, const std::vector<byte>& fileData) {
	// After the vertex shader file is loaded, create the shader and input layout.
	DX::ThrowIfFailed(
		device->CreateVertexShader(
			&fileData[0],
			fileData.size(),
			nullptr,
			m_vertexShader.put()
		)
	);

	static const D3D11_INPUT_ELEMENT_DESC vertexDesc[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	DX::ThrowIfFailed(
		device->CreateInputLayout(
			vertexDesc,
			ARRAYSIZE(vertexDesc),
			&fileData[0],
			fileData.size(),
			m_inputLayout.put()
		)
	);
	m_vertex_stride = sizeof(dvr::VertexPos3d);
	m_vertex_offset = 0;
}
void lineRenderer::create_fragment_shader(ID3D11Device* device, const std::vector<byte>& fileData) {
	// After the pixel shader file is loaded, create the shader and constant buffer.
	DX::ThrowIfFailed(
		device->CreatePixelShader(
			&fileData[0],
			fileData.size(),
			nullptr,
			m_pixelShader.put()
		)
	);

	CD3D11_BUFFER_DESC constantBufferDesc(sizeof(dvr::ModelViewProjectionConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);
	DX::ThrowIfFailed(
		device->CreateBuffer(
			&constantBufferDesc,
			nullptr,
			m_constantBuffer.put()
		)
	);

	CD3D11_BUFFER_DESC pixconstBufferDesc(sizeof(dvr::ColorConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);
	dvr::ColorConstantBuffer tdata;
	if(m_uid == 4) tdata.u_color = { 1.0f, .0f, .0f, 1.0f };
	else tdata.u_color = { 1.0f, 1.0f, .0f, 1.0f };
	D3D11_SUBRESOURCE_DATA color_resource;
	color_resource.pSysMem = &tdata;
	createPixelConstantBuffer(device, pixconstBufferDesc, &color_resource);
}