#include "pch.h"
#include "organMeshRenderer.h"
#include <Common/DirectXHelper.h>
#include <Common/Manager.h>
#include <D3DPipeline/Primitive.h>
using namespace DirectX;

organMeshRenderer::organMeshRenderer(ID3D11Device* device)
	:baseRenderer(device, L"mesh3DVertexShader.cso", L"mesh3DPixelShader.cso"){
	this->initialize();

	//setup rasterization state
	D3D11_RASTERIZER_DESC desc;
	ZeroMemory(&desc, sizeof(desc));
	desc.FillMode = D3D11_FILL_SOLID;// D3D11_FILL_SOLID;//D3D11_FILL_WIREFRAME;//
	desc.CullMode = D3D11_CULL_FRONT; // D3D11_CULL_NONE // D3D11_CULL_BACK // D3D11_CULL_FRONT
	//desc.CullMode = D3D11_CULL_BACK;
	desc.FrontCounterClockwise = FALSE;
	desc.DepthBias = 0;
	desc.SlopeScaledDepthBias = 0.0f;
	desc.DepthBiasClamp = 0.0f;
	desc.DepthClipEnable = FALSE;
	//desc.DepthClipEnable = FALSE;
	desc.ScissorEnable = FALSE;
	desc.MultisampleEnable = FALSE;
	desc.AntialiasedLineEnable = FALSE;

	//Create
	device->CreateRasterizerState(&desc, &m_RasterizerState);

	//setup compute shader
	auto loadCSTask = DX::ReadDataAsync(L"MarchingCubeGeometryShader.cso");
	// After the vertex shader file is loaded, create the shader and input layout.
	auto createCSTask = loadCSTask.then([this, device](const std::vector<byte>& fileData) {
		DX::ThrowIfFailed(
			device->CreateComputeShader(
				&fileData[0],
				fileData.size(),
				nullptr,
				&m_computeShader
			)
		);

		CD3D11_BUFFER_DESC constantBufferDesc(sizeof(XMFLOAT4X4) * 12, D3D11_BIND_CONSTANT_BUFFER);
		winrt::check_hresult(
			device->CreateBuffer(
				&constantBufferDesc,
				nullptr,
				&m_computeConstBuff
			)
		);
		m_computeConstData.u_organ_num = 7;
		m_computeConstData.u_maskbits =8;
	});

	//setup StructuredBuffer(triangle table & configuration table)

	CD3D11_BUFFER_DESC constantDataDesc(sizeof(int) * 256 * 16, D3D11_BIND_SHADER_RESOURCE);
	//constantDataDesc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;

	D3D11_SUBRESOURCE_DATA tri_data = { 0 };
	tri_data.pSysMem = &triangle_table[0][0];
	tri_data.SysMemPitch = 0;
	tri_data.SysMemSlicePitch = 0;

	winrt::check_hresult(
		device->CreateBuffer(&constantDataDesc, &tri_data, &m_computeInBuff_tri)
	);

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	srvDesc.Format = DXGI_FORMAT_R32_SINT;// DXGI_FORMAT_UNKNOWN;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	
	srvDesc.Buffer.FirstElement = 0;
	//srvDesc.Buffer.Flags = D3D11_BUFFEREX_SRV_FLAG_RAW;
	srvDesc.Buffer.NumElements = 256 * 16;
	
	
	winrt::check_hresult(
		device->CreateShaderResourceView(m_computeInBuff_tri, &srvDesc, &m_computeInSRV_tri)
	);

	//configuration
	constantDataDesc.ByteWidth = sizeof(int) * 256;
	D3D11_SUBRESOURCE_DATA edge_data = { 0 };
	edge_data.pSysMem = edge_table;
	edge_data.SysMemPitch = 0;
	edge_data.SysMemSlicePitch = 0;
	winrt::check_hresult(
		device->CreateBuffer(&constantDataDesc, &edge_data, &m_computeInBuff_config)
	);
	srvDesc.Buffer.NumElements = 256;
	winrt::check_hresult(
		device->CreateShaderResourceView(m_computeInBuff_config, &srvDesc, &m_computeInSRV_config)
	);
}

void organMeshRenderer::Setup(ID3D11Device* device, UINT h, UINT w, UINT d) {
	m_computeConstData.u_volume_size = { h,w,d,(UINT)0};
	m_computeConstData.u_grid_size = { UINT(h* grid_factor), UINT(w * grid_factor), UINT(d * grid_factor), (UINT)0 };
	
	m_vertice_count = m_computeConstData.u_grid_size.x
		* m_computeConstData.u_grid_size.y 
		* m_computeConstData.u_grid_size.z 
		* max_triangles_per_cell 
		* max_vertices_per_triangle;

	//m_vertice_count = 6;

	//setup the RW buffer
	if (m_computeOutBuff != nullptr) { m_computeOutBuff->Release(); m_computeOutBuff = nullptr; }
	if (m_vertexBuffer != nullptr) { m_vertexBuffer = nullptr; }
	if (m_computeUAV != nullptr) { m_computeUAV->Release(); m_computeUAV = nullptr; }

	D3D11_BUFFER_DESC outputDesc;
	outputDesc.Usage = D3D11_USAGE_DEFAULT;
	outputDesc.ByteWidth = sizeof(MarchingCubeOutputVertex) * m_vertice_count;
	outputDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
	outputDesc.CPUAccessFlags = 0;
	outputDesc.StructureByteStride = sizeof(MarchingCubeOutputVertex);
	outputDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;

	winrt::check_hresult(
		(device->CreateBuffer(&outputDesc, 0, &m_computeOutBuff))
		);

	// a system memory version of the buffer to read the results back from
	outputDesc.Usage = D3D11_USAGE_DYNAMIC;
	outputDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	outputDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	outputDesc.MiscFlags = 0;
	winrt::check_hresult(
		(device->CreateBuffer(&outputDesc, 0, m_vertexBuffer.put()))
	);

	//UAV
	D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;
	uavDesc.Format = DXGI_FORMAT_UNKNOWN;
	uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
	uavDesc.Buffer.FirstElement = 0;
	uavDesc.Buffer.Flags = 0;
	uavDesc.Buffer.NumElements = m_vertice_count;
	
	winrt::check_hresult(
		device->CreateUnorderedAccessView(m_computeOutBuff, &uavDesc, &m_computeUAV)
		);
	m_baked_dirty = true;
}

void organMeshRenderer::create_vertex_shader(ID3D11Device* device, const std::vector<byte>& fileData) {
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
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
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
	m_vertex_stride = sizeof(MarchingCubeOutputVertex);
	m_vertex_offset = 0;
}
void organMeshRenderer::create_fragment_shader(ID3D11Device* device, const std::vector<byte>& fileData) {
	winrt::check_hresult(
		device->CreatePixelShader(
			&fileData[0],
			fileData.size(),
			nullptr,
			m_pixelShader.put()
		)
	);
	
	CD3D11_BUFFER_DESC constantBufferDesc(sizeof(dvr::ModelViewProjectionConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);
	winrt::check_hresult(
		device->CreateBuffer(
			&constantBufferDesc,
			nullptr,
			m_constantBuffer.put()
		)
	);

	//CD3D11_BUFFER_DESC pixconstantBufferDesc(sizeof(meshPixelConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);
	//winrt::check_hresult(
	//	device->CreateBuffer(
	//		&pixconstantBufferDesc,
	//		nullptr,
	//		m_pixConstantBuffer.put()
	//	)
	//);
	//D3D11_BLEND_DESC omDesc;
	//ZeroMemory(&omDesc, sizeof(D3D11_BLEND_DESC));
	//omDesc.RenderTarget[0].BlendEnable = TRUE;
	//omDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	//omDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	//omDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	//omDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ZERO;// D3D11_BLEND_ONE;
	//omDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;// D3D11_BLEND_ZERO;
	//omDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	//omDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	//device->CreateBlendState(&omDesc, &d3dBlendState);


	/*D3D11_RASTERIZER_DESC wfdesc;
	ZeroMemory(&wfdesc, sizeof(D3D11_RASTERIZER_DESC));
	wfdesc.CullMode = D3D11_CULL_NONE;
	wfdesc.FrontCounterClockwise = TRUE;
	device->CreateRasterizerState(&wfdesc, &m_render_state);*/
}
bool organMeshRenderer::Draw(ID3D11DeviceContext* context, Texture* tex_vol, DirectX::XMMATRIX modelMat) {
	if (!m_loadingComplete) return false;
	if (m_baked_dirty) {
		context->CSSetShader(m_computeShader, nullptr, 0);
		ID3D11ShaderResourceView* srvs[3] = { 
			tex_vol->GetTextureView(), 
			m_computeInSRV_tri,
			m_computeInSRV_config,
			//m_triTableSRV 
		};
		//ID3D11ShaderResourceView* texview = tex_vol->GetTextureView();
		context->CSSetShaderResources(0, 3, srvs);
		context->CSSetUnorderedAccessViews(0, 1, &m_computeUAV, nullptr);
		
		if (m_computeConstBuff != nullptr) {
			// Prepare the constant buffer to send it to the graphics device.
			context->UpdateSubresource(
				m_computeConstBuff,
				0,
				nullptr,
				&m_computeConstData,
				0,
				0
			);
			context->CSSetConstantBuffers(0, 1, &m_computeConstBuff);
		}
		//run compute shader
		context->Dispatch(
			(m_computeConstData.u_grid_size.x + 7) / 8,// / 8, 
			(m_computeConstData.u_grid_size.y + 7) / 8,// / 8, 
			(m_computeConstData.u_grid_size.z + 7) / 8// / 8
		);
		//debug
		//context->Dispatch((tex_vol->Width() + 7) / 6, (tex_vol->Height() + 7) / 6, (tex_vol->Depth() + 7) / 6);

		context->CopyResource(m_vertexBuffer.get(), m_computeOutBuff);
		//unbind UAV
		ID3D11UnorderedAccessView* nullUAV[] = { NULL };
		context->CSSetUnorderedAccessViews(0, 1, nullUAV, 0);
		// Disable Compute Shader
		context->CSSetShader(nullptr, nullptr, 0);
		context->CSSetConstantBuffers(0, 0, nullptr);
		ID3D11ShaderResourceView* ppSRVNULL[3] = { nullptr, nullptr, nullptr };
		context->CSSetShaderResources(0, 3, ppSRVNULL);
		m_baked_dirty = false;
	}
	if (m_constantBuffer != nullptr) {
		DirectX::XMStoreFloat4x4(&m_const_buff_data.uViewProjMat, Manager::camera->getVPMat());
		//TODO: don't know why no transpose...
		DirectX::XMStoreFloat4x4(&m_const_buff_data.model, DirectX::XMMatrixTranspose(modelMat));
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
	context->RSSetState(m_RasterizerState);
	baseRenderer::Draw(context);
	context->RSSetState(nullptr);
	return true;
}
