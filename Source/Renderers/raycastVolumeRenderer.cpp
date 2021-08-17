#include "pch.h"
#include "raycastVolumeRenderer.h"
#include <D3DPipeline/Primitive.h>
#include <Common/Manager.h>
#include <vrController.h>
using namespace DirectX;

raycastVolumeRenderer::raycastVolumeRenderer(ID3D11Device* device)
	:baseDicomRenderer(device,
		L"raycastVertexShader.cso", L"raycastPixelShader.cso",
		cube_vertices_pos_w_tex, cube_indices,
		48, 36
	)
{
	this->initialize();
	m_sample_steps = Manager::indiv_rendering_params[dvr::RAYCASTING];
}

void raycastVolumeRenderer::create_vertex_shader(ID3D11Device* device, const std::vector<byte>& fileData) {
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
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
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
	m_vertex_stride = sizeof(dvr::VertexPositionColor);
	m_vertex_offset = 0;
}
void raycastVolumeRenderer::create_fragment_shader(ID3D11Device* device, const std::vector<byte>& fileData) {
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

	CD3D11_BUFFER_DESC constantBufferDesc(sizeof(raycastVertexBuffer), D3D11_BIND_CONSTANT_BUFFER);
	winrt::check_hresult(
		device->CreateBuffer(
			&constantBufferDesc,
			nullptr,
			m_constantBuffer.put()
		)
	);

	CD3D11_BUFFER_DESC pixconstantBufferDesc(sizeof(raycastPixConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);
	winrt::check_hresult(
		device->CreateBuffer(
			&pixconstantBufferDesc,
			nullptr,
			m_pixConstantBuffer.put()
		)
	);
}

bool raycastVolumeRenderer::Draw(ID3D11DeviceContext* context, Texture* tex, DirectX::XMMATRIX modelMat, bool is_front) {
	if (!m_loadingComplete) return false;
	//context->RSSetState(m_render_state);
	if (m_constantBuffer != nullptr) {
		DirectX::XMStoreFloat4x4(&m_const_buff_data.uMVP.mm,
			DirectX::XMMatrixMultiply(Manager::camera->getVPMat(), DirectX::XMMatrixTranspose(modelMat)));

		auto inv_mat = DirectX::XMMatrixInverse(nullptr, modelMat);
		DirectX::XMVECTOR veye = DirectX::XMLoadFloat3(&Manager::camera->getCameraPosition());
		DirectX::XMStoreFloat4(&m_const_buff_data.uCamPosInObjSpace, //Manager::camera->getCameraPosition()
			DirectX::XMVector4Transform(veye,
				inv_mat));

		// Prepare the constant buffer to send it to the graphics device.
		context->UpdateSubresource(
			m_constantBuffer.get(),
			0,
			nullptr,
			&m_const_buff_data,
			0,
			0
		);
	}
	//update pixel shader const buffer data
	if (m_pixConstantBuffer != nullptr) {
		m_pix_const_buff_data.u_cut = Manager::IsCuttingEnabled();
		m_pix_const_buff_data.u_sample_param = 1.0f / m_sample_steps;
		m_pix_const_buff_data.u_cutplane_realsample = Manager::param_bool[dvr::CUT_PLANE_REAL_SAMPLE];
		if (m_pix_const_buff_data.u_cut) 
			vrController::instance()->getCuttingPlane(
				m_pix_const_buff_data.u_plane.pp, 
				m_pix_const_buff_data.u_plane.pn);
		context->UpdateSubresource(
			m_pixConstantBuffer.get(),
			0,
			nullptr,
			&m_pix_const_buff_data,
			0,
			0
		);
	}
	if (tex != nullptr) {
		auto texview = tex->GetTextureView();
		context->PSSetShaderResources(0, 1, &texview);
	}
	baseRenderer::Draw(context);
	return true;
}
