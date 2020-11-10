﻿#include "pch.h"
#include "baseRenderer.h"
#include <Common/DirectXHelper.h>
#include <D3DPipeline/Primitive.h>

using namespace DirectX;
baseRenderer::baseRenderer(ID3D11Device* device,
	const wchar_t* vname, const wchar_t* pname,
	const float* vdata, const unsigned short* idata,
	UINT vertice_num, UINT idx_num)
	:m_loadingComplete(false),
	m_vertice_count(vertice_num),
	m_index_count(idx_num){
	// Load shaders asynchronously.
	auto loadVSTask = DX::ReadDataAsync(vname);
	auto loadPSTask = DX::ReadDataAsync(pname);

	// After the vertex shader file is loaded, create the shader and input layout.
	auto createVSTask = loadVSTask.then([this, device](const std::vector<byte>& fileData) {
		create_vertex_shader(device, fileData);
	});


	// After the pixel shader file is loaded, create the shader and constant buffer.
	auto createPSTask = loadPSTask.then([this, device](const std::vector<byte>& fileData) {
		create_fragment_shader(device, fileData);
	});


	// Once both shaders are loaded, create the mesh.
	auto createModelTask = (createPSTask && createVSTask).then([this, device, vdata, idata]() {
		D3D11_SUBRESOURCE_DATA vertexBufferData = { 0 };
		vertexBufferData.pSysMem = vdata;
		vertexBufferData.SysMemPitch = 0;
		vertexBufferData.SysMemSlicePitch = 0;
		CD3D11_BUFFER_DESC vertexBufferDesc(m_vertice_count * sizeof(float), D3D11_BIND_VERTEX_BUFFER);
		winrt::check_hresult(
			device->CreateBuffer(
				&vertexBufferDesc,
				&vertexBufferData,
				m_vertexBuffer.put()
			)
		);

		D3D11_SUBRESOURCE_DATA indexBufferData = { 0 };
		indexBufferData.pSysMem = idata;
		indexBufferData.SysMemPitch = 0;
		indexBufferData.SysMemSlicePitch = 0;
		CD3D11_BUFFER_DESC indexBufferDesc(m_index_count * sizeof(unsigned short), D3D11_BIND_INDEX_BUFFER);
		winrt::check_hresult(
			device->CreateBuffer(
				&indexBufferDesc,
				&indexBufferData,
				m_indexBuffer.put()
			)
		);
	});

	createModelTask.then([this]() {
		m_loadingComplete = true;
	});
}


// Renders one frame using the vertex and pixel shaders.
void baseRenderer::Draw(ID3D11DeviceContext* context) {
	ID3D11Buffer* tmp_vertex_buff{ m_vertexBuffer.get() };
	context->IASetVertexBuffers(
		0,
		1,
		&tmp_vertex_buff,
		&m_vertex_stride,
		&m_vertex_offset
		);

	context->IASetIndexBuffer(
		m_indexBuffer.get(),
		DXGI_FORMAT_R16_UINT, // Each index is one 16-bit unsigned integer (short).
		0);

	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	if (m_inputLayout != nullptr) context->IASetInputLayout(m_inputLayout.get());

	// Attach our vertex shader.
	if (m_vertexShader != nullptr) context->VSSetShader(m_vertexShader.get(), nullptr, 0);

	// Attach our pixel shader.
	if(m_pixelShader!=nullptr) context->PSSetShader(m_pixelShader.get(),nullptr,0);
	
	// Send the constant buffer to the graphics device.
	if (m_constantBuffer != nullptr) {
		ID3D11Buffer* constantBufferNeverChanges{ m_constantBuffer.get() };
		context->VSSetConstantBuffers(0, 1, &constantBufferNeverChanges);
	}

	//texture sampler
	if(m_sampleState!=nullptr) context->PSSetSamplers(0, 1, &m_sampleState);

	// Draw the objects.
	context->DrawIndexed(m_index_count,0,0);
}