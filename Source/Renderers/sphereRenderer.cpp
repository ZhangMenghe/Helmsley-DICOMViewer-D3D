#include "pch.h"
#include "sphereRenderer.h"
#include "Common/DirectXHelper.h"
#include <Common/Manager.h>
#include <D3DPipeline/Primitive.h>
using namespace DirectX;

sphereRenderer::sphereRenderer(ID3D11Device* device, int stackCount, int sectorCount, DirectX::XMFLOAT4 color)
:baseRenderer(device, L"NaiveNormVertexShader.cso", L"mesh3DPixelShader.cso", nullptr, nullptr, 0, 0){
	m_color = color;
	m_vertice_count = (stackCount + 1) * (sectorCount + 1) * 3;
	float sectorStep = 2 * glm::pi<float>() / sectorCount;
	float stackStep = glm::pi<float>() / stackCount;
	float sectorAngle, stackAngle;
	float radius = 1.0f;
	float x, y, z, xy; int id = 0;
	for (int i = 0; i <= stackCount; ++i){
		stackAngle = glm::pi<float>() / 2 - i * stackStep;        // starting from pi/2 to -pi/2
		xy = radius * cosf(stackAngle);             // r * cos(u)
		z = radius * sinf(stackAngle);              // r * sin(u)

		// add (sectorCount+1) vertices per stack
		// the first and last vertices have same position and normal, but different tex coords
		for (int j = 0; j <= sectorCount; ++j)
		{
			sectorAngle = j * sectorStep;           // starting from 0 to 2pi

			// vertex position (x, y, z)
			x = xy * cosf(sectorAngle);             // r * cos(u) * cos(v)
			y = xy * sinf(sectorAngle);             // r * cos(u) * sin(v)
			m_vertices.push_back(x); m_vertices.push_back(y); m_vertices.push_back(z);
			//vertices[3*id] = x; vertices[3 * id+1] = y; vertices[3 * id+2] = z;
			id += 1;
		}
	}
	m_vertice_count = m_vertices.size();
	vdata = m_vertices.data();

	int k1, k2;
	for (int i = 0; i < stackCount; ++i){
		k1 = i * (sectorCount + 1);     // beginning of current stack
		k2 = k1 + sectorCount + 1;      // beginning of next stack

		for (int j = 0; j < sectorCount; ++j, ++k1, ++k2){
			// 2 triangles per sector excluding first and last stacks
			// k1 => k2 => k1+1
			if (i != 0){
				m_indices.push_back(k1);
				m_indices.push_back(k1 + 1);
				m_indices.push_back(k2);

			}

			// k1+1 => k2 => k2+1
			if (i != (stackCount - 1)){
				m_indices.push_back(k1 + 1);
				m_indices.push_back(k2 + 1);
				m_indices.push_back(k2);
			}

			//indices for lines
			m_line_indices.push_back(k1); m_line_indices.push_back(k2);
			if(i!=0){ m_line_indices.push_back(k1); m_line_indices.push_back(k1+1);}
		}
	}

	m_index_count = m_indices.size();
	idata = m_indices.data();

	initialize();
	m_line_renderer = std::make_unique<lineRenderer>(
		device, DirectX::XMFLOAT4({.0f, .0f, .0f, 1.0f}),
		m_vertices.data(), m_line_indices.data(), 
		m_vertice_count, m_index_count);
}


void sphereRenderer::create_vertex_shader(ID3D11Device* device, const std::vector<byte>& fileData) {
	// After the vertex shader file is loaded, create the shader and input layout.
	DX::ThrowIfFailed(
		device->CreateVertexShader(
			&fileData[0],
			fileData.size(),
			nullptr,
			m_vertexShader.put()
		)
	);

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
	m_vertex_offset = 0;

	CD3D11_BUFFER_DESC constantBufferDesc(sizeof(dvr::ModelViewProjectionConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);
	DX::ThrowIfFailed(
		device->CreateBuffer(
			&constantBufferDesc,
			nullptr,
			m_constantBuffer.put()
		)
	);
}
void sphereRenderer::create_fragment_shader(ID3D11Device* device, const std::vector<byte>& fileData) {
	// After the pixel shader file is loaded, create the shader and constant buffer.
	DX::ThrowIfFailed(
		device->CreatePixelShader(
			&fileData[0],
			fileData.size(),
			nullptr,
			m_pixelShader.put()
		)
	);

	CD3D11_BUFFER_DESC pixconstBufferDesc(sizeof(dvr::ColorConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);
	dvr::ColorConstantBuffer tdata;
	tdata.u_color = m_color;
	D3D11_SUBRESOURCE_DATA color_resource;
	color_resource.pSysMem = &tdata;
	createPixelConstantBuffer(device, pixconstBufferDesc, &color_resource);
}
// Renders one frame using the vertex and pixel shaders.
bool sphereRenderer::Draw(ID3D11DeviceContext* context, DirectX::XMMATRIX modelMat) {
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

	m_line_renderer->Draw(context, modelMat);
	return true;
}