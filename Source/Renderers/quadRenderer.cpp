#include "pch.h"
#include "quadRenderer.h"
#include "Common/DirectXHelper.h"
#include <D3DPipeline/Primitive.h>
#include <Common/Manager.h>
using namespace DirectX;
quadRenderer::quadRenderer(ID3D11Device* device)
:baseRenderer(device, L"QuadVertexShader.cso", L"QuadPixelShader.cso",
	quad_vertices_pos_w_tex, quad_indices, 24, 6),
	m_input_layout_id(dvr::INPUT_POS_TEX_2D){
	this->initialize();
}
quadRenderer::quadRenderer(ID3D11Device* device, DirectX::XMFLOAT4 color, const float* vdata)
:baseRenderer(device, L"Naive3DVertexShader.cso", L"SimpleColorPixelShader.cso", 
	vdata, quad_indices, 12, 6),
	m_input_layout_id(dvr::INPUT_POS_3D){
	this->initialize();

	CD3D11_BUFFER_DESC pixconstBufferDesc(sizeof(dvr::ColorConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);
	dvr::ColorConstantBuffer tdata;
	tdata.u_color = color;
	D3D11_SUBRESOURCE_DATA color_resource;
	color_resource.pSysMem = &tdata;
	createPixelConstantBuffer(device, pixconstBufferDesc, &color_resource);
}
quadRenderer::quadRenderer(ID3D11Device* device, DirectX::XMFLOAT4 color)
:quadRenderer(device, color, quad_vertices_3d){
}

quadRenderer::quadRenderer(ID3D11Device* device, const wchar_t* vname, const wchar_t* pname) 
: baseRenderer(device, vname, pname,
	quad_vertices_pos_w_tex, quad_indices, 24, 6),
	m_input_layout_id(dvr::INPUT_POS_TEX_2D) {
	this->initialize();
}
quadRenderer::quadRenderer(ID3D11Device* device, const wchar_t* vname, const wchar_t* pname, const float* vdata)
	:baseRenderer(device, vname, pname,
		vdata, quad_indices, 24, 6),
	m_input_layout_id(dvr::INPUT_POS_TEX_2D) {
	this->initialize();
}
quadRenderer::quadRenderer(ID3D11Device* device, const wchar_t* vname, const wchar_t* pname, const float* vdata, const unsigned short* idata, UINT vertice_num, UINT idx_num, dvr::INPUT_LAYOUT_IDS layout_id)
	:baseRenderer(device, vname, pname,
		vdata, idata, vertice_num, idx_num),
	m_input_layout_id(layout_id){
	this->initialize();
}
// Renders one frame using the vertex and pixel shaders.
bool quadRenderer::Draw(ID3D11DeviceContext* context, DirectX::XMMATRIX modelMat){
	if (!m_loadingComplete) return false; 
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

	baseRenderer::Draw(context);
	return true;
}
void quadRenderer::create_vertex_shader(ID3D11Device* device, const std::vector<byte>& fileData) {
	// After the vertex shader file is loaded, create the shader and input layout.
	DX::ThrowIfFailed(
		device->CreateVertexShader(
			&fileData[0],
			fileData.size(),
			nullptr,
			m_vertexShader.put()
		)
	);

	if (m_input_layout_id == dvr::INPUT_POS_TEX_2D) {
		DX::ThrowIfFailed(
			device->CreateInputLayout(
				dvr::g_vinput_pos_tex_desc,
				ARRAYSIZE(dvr::g_vinput_pos_tex_desc),
				&fileData[0],
				fileData.size(),
				m_inputLayout.put()
			)
		);
		m_vertex_stride = sizeof(dvr::VertexPosTex2d);
	}
	else {
		DX::ThrowIfFailed(
			device->CreateInputLayout(
				dvr::g_vinput_pos_3d_desc,
				ARRAYSIZE(dvr::g_vinput_pos_3d_desc),
				&fileData[0],
				fileData.size(),
				m_inputLayout.put()
			)
		);
		m_vertex_stride = sizeof(dvr::VertexPos3d);
	}
	m_vertex_offset = 0;
}
void quadRenderer::create_fragment_shader(ID3D11Device* device, const std::vector<byte>& fileData) {
	// After the pixel shader file is loaded, create the shader and constant buffer.
	DX::ThrowIfFailed(
		device->CreatePixelShader(
			&fileData[0],
			fileData.size(),
			nullptr,
			m_pixelShader.put()
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
	DX::ThrowIfFailed(device->CreateSamplerState(&samplerDesc, &m_sampleState));

	CD3D11_BUFFER_DESC constantBufferDesc(sizeof(dvr::ModelViewProjectionConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);
	DX::ThrowIfFailed(
		device->CreateBuffer(
			&constantBufferDesc,
			nullptr,
			m_constantBuffer.put()
		)
	);
}