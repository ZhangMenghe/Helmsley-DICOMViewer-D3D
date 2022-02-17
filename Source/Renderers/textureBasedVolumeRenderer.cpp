#include "pch.h"
#include "textureBasedVolumeRenderer.h"
#include <D3DPipeline/Primitive.h>
#include <Common/Manager.h>
#include <vrController.h>
using namespace DirectX;

textureBasedVolumeRenderer::textureBasedVolumeRenderer(ID3D11Device* device)
	:baseDicomRenderer(device,
		L"texbasedVertexShader.cso", L"texbasedPixelShader.cso",
		nullptr, nullptr, 0, 0
	),
	cut_id(0)
{
	initialize();
	initialize_vertices_and_indices(device);

	float tmp[] = { Manager::indiv_rendering_params[0] };
	setRenderingParameters(tmp);
}
void textureBasedVolumeRenderer::initialize_vertices_and_indices(ID3D11Device* device) {
	m_vertice_count = 12 * MAX_DIMENSIONS;
	m_vertices_front = new float[m_vertice_count]; m_vertices_back = new float[m_vertice_count];
	for (int i = 0, idj = 0; i < MAX_DIMENSIONS; i++, idj += 12) {
		for (int j = 0; j < 12; j++) {
			m_vertices_front[idj + j] = quad_vertices_3d[j]; m_vertices_back[idj + j] = quad_vertices_3d[j];
		}
	}
	createDynamicVertexBuffer(device, m_vertice_count);

	m_index_count = 6 * MAX_DIMENSIONS;
	m_indices = new unsigned short[m_index_count];
	for (int i = 0, idk = 0; i < MAX_DIMENSIONS; i++, idk += 6) {
		for (int k = 0; k < 6; k++)m_indices[idk + k] = quad_indices[k] + 4 * i;
	}
	baseDicomRenderer::initialize_indices(device, m_indices);
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
	winrt::check_hresult(
		device->CreateInputLayout(
			dvr::g_vinput_pos_3d_desc,
			ARRAYSIZE(dvr::g_vinput_pos_3d_desc),
			&fileData[0],
			fileData.size(),
			m_inputLayout.put()
		)
	);
	m_vertex_stride = sizeof(dvr::VertexPos3d);
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

	CD3D11_BUFFER_DESC constantBufferDesc(sizeof(texbasedVertexBuffer), D3D11_BIND_CONSTANT_BUFFER);
	winrt::check_hresult(
		device->CreateBuffer(
			&constantBufferDesc,
			nullptr,
			m_constantBuffer.put()
		)
	);

	CD3D11_BUFFER_DESC pixconstantBufferDesc(sizeof(dvr::cutPlaneConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);
	winrt::check_hresult(
		device->CreateBuffer(
			&pixconstantBufferDesc,
			nullptr,
			m_pixConstantBuffer.put()
		)
	);

	D3D11_BLEND_DESC blendDesc;
	ZeroMemory(&blendDesc, sizeof(blendDesc));

	D3D11_RENDER_TARGET_BLEND_DESC rtbd;
	ZeroMemory(&rtbd, sizeof(rtbd));

	rtbd.BlendEnable = TRUE;
	rtbd.SrcBlend = D3D11_BLEND_SRC_ALPHA;//D3D11_BLEND_ONE;
	rtbd.DestBlend = D3D11_BLEND_INV_SRC_ALPHA;//D3D11_BLEND_INV_SRC_ALPHA;
	rtbd.BlendOp = D3D11_BLEND_OP_ADD;
	rtbd.SrcBlendAlpha = D3D11_BLEND_ONE;//D3D11_BLEND_ONE;
	rtbd.DestBlendAlpha = D3D11_BLEND_ZERO;//D3D11_BLEND_ZERO;
	rtbd.BlendOpAlpha = D3D11_BLEND_OP_ADD;
	rtbd.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	blendDesc.AlphaToCoverageEnable = false;
	blendDesc.RenderTarget[0] = rtbd;

	device->CreateBlendState(&blendDesc, &m_d3dBlendState);
}
void textureBasedVolumeRenderer::update_instance_data(ID3D11DeviceContext* context){
	if (dimension_draw == 0) return;
	float mappedZVal = -0.5f;
	for (int i = 0, id = 0; i < dimension_draw; i++, id += 12) {
		m_vertices_front[id + 2] = mappedZVal; m_vertices_front[id + 5] = mappedZVal; m_vertices_front[id + 8] = mappedZVal; m_vertices_front[id + 11] = mappedZVal;
		mappedZVal += dimension_draw_inv;
	}
	mappedZVal = 0.5f;
	for (int i = 0, id = 0; i < dimension_draw; i++, id += 12) {
		m_vertices_back[id + 2] = m_vertices_back[id + 5] = m_vertices_back[id + 8] = m_vertices_back[id + 11] = mappedZVal;
		mappedZVal -= dimension_draw_inv;
	}
}
bool textureBasedVolumeRenderer::Draw(ID3D11DeviceContext* context, Texture* tex, DirectX::XMMATRIX modelMat, bool is_front){
	if (!m_loadingComplete) return false;
	if(!is_front)context->RSSetState(vrController::instance()->m_render_state_back);
	if (m_instance_data_dirty) {
		update_instance_data(context);
		m_instance_data_dirty = false;
	}
	updateVertexBuffer(context, is_front ? m_vertices_front : m_vertices_back);

	if (m_constantBuffer != nullptr) {
		DirectX::XMStoreFloat4x4(&m_vertex_const_buff_data.uMVP.mm,
			DirectX::XMMatrixMultiply(Manager::camera->getVPMat(), 
				DirectX::XMMatrixTranspose(modelMat)));
		m_vertex_const_buff_data.u_volume_thickness = vol_thickness_factor;

		// Prepare the constant buffer to send it to the graphics device.
		context->UpdateSubresource(
			m_constantBuffer.get(),
			0,
			nullptr,
			&m_vertex_const_buff_data,
			0,
			0
		);
	}
	if (m_pixConstantBuffer != nullptr) {
		if (Manager::IsCuttingEnabled()) {
			vrController::instance()->getCuttingPlane(
				m_pix_const_buff_data.pp,
				m_pix_const_buff_data.pn);
		}else {
			m_pix_const_buff_data.pn = XMFLOAT4(-1.0f , -1.0f , -1.0f, -1.0f);
		}
		context->UpdateSubresource(
			m_pixConstantBuffer.get(),
			0,
			nullptr,
			&m_pix_const_buff_data,
			0,
			0
		);
	}
	context->OMSetBlendState(m_d3dBlendState, 0, 0xffffffff);

	if (tex != nullptr) {
		auto texview = tex->GetTextureView();
		context->PSSetShaderResources(0, 1, &texview);
	}

	baseRenderer::Draw(context, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	//setback states
	context->OMSetBlendState(nullptr, 0, 0xffffffff);
	if (!is_front) context->RSSetState(vrController::instance()->m_render_state_front);
	return true;
}
void textureBasedVolumeRenderer::setDimension(ID3D11Device* device, glm::vec3 vol_dimension, glm::vec3 vol_dim_scale) {
	baseDicomRenderer::setDimension(device, vol_dimension, vol_dim_scale);

	//dimensions = int(vol_dimension.z * DENSE_FACTOR); dimension_inv = 1.0f / dimensions;
	vol_thickness_factor = vol_dim_scale.z;// *2.0f;
	//initialize_mesh_others(device);
	on_update_dimension_draw();
}
void textureBasedVolumeRenderer::setCuttingPlane(float percent) {
	cut_id = int(dimension_draw * percent);
	//baked_dirty_ = true;
}
void textureBasedVolumeRenderer::setCuttingPlaneDelta(int delta) {
	cut_id = ((int)fmax(0, cut_id + delta)) % dimension_draw;
	//baked_dirty_ = true;
}
void textureBasedVolumeRenderer::setRenderingParameters(float* values) {
	if (values[0] == dense_factor) return;
	dense_factor = values[0];
	on_update_dimension_draw();
}
void textureBasedVolumeRenderer::on_update_dimension_draw() {
	dimension_draw = fmin(float(m_dimensions_origin) * dense_factor, MAX_DIMENSIONS);
	dimension_draw_inv = 1.0f / float(dimension_draw);
	m_instance_data_dirty = true;
}