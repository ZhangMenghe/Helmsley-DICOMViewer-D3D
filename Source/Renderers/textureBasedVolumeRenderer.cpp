﻿#include "pch.h"
#include "textureBasedVolumeRenderer.h"
#include <D3DPipeline/Primitive.h>
#include <Common/Manager.h>
#include <vrController.h>
using namespace DirectX;

textureBasedVolumeRenderer::textureBasedVolumeRenderer(ID3D11Device* device)
	:baseRenderer(device, 
		L"texbasedVertexShader.cso", L"texbasedPixelShader.cso",
		quad_vertices_pos_w_tex, quad_indices, 16, 6
	)
{

}

void textureBasedVolumeRenderer::create_vertex_shader(ID3D11Device* device, const std::vector<byte>& fileData) {
	winrt::check_hresult(
		device->CreateVertexShader(
			&fileData[0],
			fileData.size(),
			nullptr,
			m_vertexShader.put()
		)
	);
	static const D3D11_INPUT_ELEMENT_DESC vertexDesc[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 1, DXGI_FORMAT_R32G32_FLOAT, 1, 0, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
	};
	winrt::check_hresult(
		device->CreateInputLayout(
			vertexDesc,
			ARRAYSIZE(vertexDesc),
			&fileData[0],
			fileData.size(),
			m_inputLayout.put()
		)
	);
	m_vertex_stride = sizeof(dvr::VertexPosTex2d);
	m_vertex_offset = 0;
}
void textureBasedVolumeRenderer::create_fragment_shader(ID3D11Device* device, const std::vector<byte>& fileData) {
	winrt::check_hresult(
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
	winrt::check_hresult(device->CreateSamplerState(&samplerDesc, &m_sampleState));

	CD3D11_BUFFER_DESC constantBufferDesc(sizeof(dvr::ModelViewProjectionConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);
	winrt::check_hresult(
		device->CreateBuffer(
			&constantBufferDesc,
			nullptr,
			m_constantBuffer.put()
		)
	);

	CD3D11_BUFFER_DESC pixconstantBufferDesc(sizeof(DirectX::XMFLOAT4X4), D3D11_BIND_CONSTANT_BUFFER);
	winrt::check_hresult(
		device->CreateBuffer(
			&pixconstantBufferDesc,
			nullptr,
			m_pixConstantBuffer.put()
		)
	);

	D3D11_BLEND_DESC omDesc;
	ZeroMemory(&omDesc, sizeof(D3D11_BLEND_DESC));
	omDesc.RenderTarget[0].BlendEnable = TRUE;
	omDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	omDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	omDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	omDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;// D3D11_BLEND_ONE;
	omDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;// D3D11_BLEND_ZERO;
	omDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	omDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	omDesc.AlphaToCoverageEnable = false;
	device->CreateBlendState(&omDesc, &d3dBlendState);


	//D3D11_RASTERIZER_DESC wfdesc;
	//ZeroMemory(&wfdesc, sizeof(D3D11_RASTERIZER_DESC));
	//wfdesc.CullMode = D3D11_CULL_BACK;
	//wfdesc.FrontCounterClockwise = true;

	D3D11_RASTERIZER_DESC rasterDesc;

	rasterDesc.AntialiasedLineEnable = false;
	rasterDesc.CullMode = D3D11_CULL_BACK;
	rasterDesc.DepthBias = 0;
	rasterDesc.DepthBiasClamp = 0.0f;
	rasterDesc.DepthClipEnable = true;
	rasterDesc.FillMode = D3D11_FILL_SOLID;
	rasterDesc.FrontCounterClockwise = false;
	rasterDesc.MultisampleEnable = false;
	rasterDesc.ScissorEnable = false;
	rasterDesc.SlopeScaledDepthBias = 0.0f;
	winrt::check_hresult(
		device->CreateRasterizerState(&rasterDesc, &m_render_state_front)
	);
	rasterDesc.FrontCounterClockwise = true;
	winrt::check_hresult(
		device->CreateRasterizerState(&rasterDesc, &m_render_state_back)
	);
}
void textureBasedVolumeRenderer::initialize_mesh_others(ID3D11Device* device){
	//update instance data
	if (dimensions == 0) return;
	//InstanceType* zInfos = new InstanceType[dimensions];
	zInfos_front = new InstanceType[dimensions];
	zInfos_back = new InstanceType[dimensions];
	
	float zTex = .0f;
	float step = 1.0f / dimensions;
	float mappedZVal = -(dimensions - 1) / 2.0f * step;
	float mappedZVal_back = -mappedZVal;
	for (int i = 0; i < dimensions; i++) {
		zInfos_front[i].zinfo.x = mappedZVal * vol_thickness_factor; zInfos_front[i].zinfo.y = zTex;
		zInfos_back[i].zinfo.x = mappedZVal_back * vol_thickness_factor; zInfos_back[i].zinfo.y = zTex;
		mappedZVal += step; mappedZVal_back -= step; zTex += dimension_inv;
	}
	
	//initialize instances
	D3D11_BUFFER_DESC instBuffDesc;
	ZeroMemory(&instBuffDesc, sizeof(instBuffDesc));

	instBuffDesc.Usage = D3D11_USAGE_DYNAMIC;//D3D11_USAGE_DEFAULT;
	instBuffDesc.ByteWidth = sizeof(InstanceType) * dimensions;
	instBuffDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	instBuffDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;//0;
	instBuffDesc.MiscFlags = 0;

	//D3D11_SUBRESOURCE_DATA instData;
	//ZeroMemory(&m_instance_data_front, sizeof(m_instance_data_front));
	//m_instance_data_front.pSysMem = zInfos;
	if(m_instanceBuffer_front == nullptr)
		winrt::check_hresult(device->CreateBuffer(&instBuffDesc,
				nullptr,
				//&m_instance_data_front,
				m_instanceBuffer_front.put()
			)
		);
	if (m_instanceBuffer_back == nullptr)
		winrt::check_hresult(device->CreateBuffer(&instBuffDesc,
			nullptr,
			m_instanceBuffer_back.put()
		)
	);
	//delete[]zInfos;
	m_data_dirty = true;
}

void textureBasedVolumeRenderer::Draw(ID3D11DeviceContext* context, Texture* tex, DirectX::XMMATRIX modelMat, bool is_front) {
	if (!m_loadingComplete) return;
	if (m_constantBuffer != nullptr) {
		DirectX::XMStoreFloat4x4(&m_const_buff_data.uViewProjMat, Manager::camera->getVPMat());
		DirectX::XMStoreFloat4x4(&m_const_buff_data.model, modelMat);

		// Prepare the constant buffer to send it to the graphics device.
		context->UpdateSubresource(
			m_constantBuffer.get(),
			0,
			nullptr,
			&m_const_buff_data,
			0,
			0
		);
		ID3D11Buffer* constantBufferNeverChanges{ m_constantBuffer.get() };
		context->VSSetConstantBuffers(0, 1, &constantBufferNeverChanges);
	}
	if (m_pixConstantBuffer != nullptr) {
		m_const_buff_data_pix.u_front = is_front;
		context->UpdateSubresource(
			m_pixConstantBuffer.get(),
			0,
			nullptr,
			&m_const_buff_data_pix,
			0,
			0
		);
		ID3D11Buffer* constantBuffer_pix{ m_pixConstantBuffer.get() };
		context->PSSetConstantBuffers(0, 1, &constantBuffer_pix);
	}
	if (m_data_dirty) {
		D3D11_MAPPED_SUBRESOURCE resource;
		context->Map(m_instanceBuffer_front.get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &resource);
		memcpy(resource.pData, zInfos_front, dimensions * sizeof(InstanceType));
		context->Unmap(m_instanceBuffer_front.get(), 0);

		D3D11_MAPPED_SUBRESOURCE resource_back;
		context->Map(m_instanceBuffer_back.get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &resource_back);
		memcpy(resource_back.pData, zInfos_back, dimensions * sizeof(InstanceType));
		context->Unmap(m_instanceBuffer_back.get(), 0);

		m_data_dirty = false;
	}
	context->OMSetBlendState(d3dBlendState, 0, 0xffffffff);

	if (tex != nullptr) {
		auto texview = tex->GetTextureView();
		context->PSSetShaderResources(0, 1, &texview);
	}
	//texture sampler
	if (m_sampleState != nullptr) context->PSSetSamplers(0, 1, &m_sampleState);


	//todo: if its front or back
	ID3D11Buffer* vertInstBuffers[2] = { m_vertexBuffer.get(), nullptr };

	if (is_front) {
		vertInstBuffers[1] = m_instanceBuffer_front.get();
		context->RSSetState(m_render_state_front);
	}else {
		vertInstBuffers[1] = m_instanceBuffer_back.get();
		context->RSSetState(m_render_state_back);
	}
	UINT strides[2] = { sizeof(dvr::VertexPosTex2d), sizeof(InstanceType) };
	UINT offsets[2] = { 0, 0 };
	context->IASetVertexBuffers(0, 2, vertInstBuffers, strides, offsets);
	context->IASetIndexBuffer(
		m_indexBuffer.get(),
		DXGI_FORMAT_R16_UINT, // Each index is one 16-bit unsigned integer (short).
		0);
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	if (m_inputLayout != nullptr) context->IASetInputLayout(m_inputLayout.get());
	
	// Attach our vertex shader.
	if (m_vertexShader != nullptr) context->VSSetShader(m_vertexShader.get(), nullptr, 0);

	// Attach our pixel shader.
	if (m_pixelShader != nullptr) context->PSSetShader(m_pixelShader.get(), nullptr, 0);
	context->DrawIndexedInstanced(m_index_count, dimensions, 0, 0, 0);
}
void textureBasedVolumeRenderer::setDimension(ID3D11Device* device, DirectX::XMUINT3 vol_dimension, DirectX::XMFLOAT3 vol_dim_scale) {
	dimensions = int(vol_dimension.z * DENSE_FACTOR); dimension_inv = 1.0f / dimensions;
	vol_thickness_factor = vol_dim_scale.z;
	initialize_mesh_others(device);
}