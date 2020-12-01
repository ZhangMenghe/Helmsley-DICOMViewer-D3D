#ifndef BASE_RENDERER_H
#define BASE_RENDERER_H
#include "pch.h"
#include <Common/ConstantAndStruct.h>
#include <D3DPipeline/Texture.h>
class baseRenderer {
public:
	baseRenderer(ID3D11Device* device,
		const wchar_t* vname, const wchar_t* pname,
		const float* vdata, const unsigned short* idata,
		UINT vertice_num = 0, UINT idx_num = 0);
	virtual void Draw(ID3D11DeviceContext* context);
	virtual void Clear() {
		m_loadingComplete = false;
		m_vertexShader = nullptr;
		m_inputLayout = nullptr;
		m_pixelShader = nullptr;
		m_constantBuffer = nullptr;
		m_vertexBuffer = nullptr;
		m_indexBuffer = nullptr;
	};
	ID3D11RenderTargetView* GetRenderTargetView() { return texture->GetRenderTargetView(); }
	void createPixelConstantBuffer(ID3D11Device* device, CD3D11_BUFFER_DESC pixconstBufferDesc, D3D11_SUBRESOURCE_DATA* data);
protected:
	//buffers
	winrt::com_ptr<ID3D11InputLayout> m_inputLayout;
	winrt::com_ptr<ID3D11Buffer> m_vertexBuffer = nullptr;
	winrt::com_ptr<ID3D11Buffer> m_indexBuffer = nullptr;
	winrt::com_ptr<ID3D11VertexShader> m_vertexShader = nullptr;
	winrt::com_ptr<ID3D11PixelShader> m_pixelShader = nullptr;
	winrt::com_ptr<ID3D11ComputeShader> m_computeShader = nullptr;
	winrt::com_ptr<ID3D11Buffer> m_constantBuffer = nullptr;
	winrt::com_ptr<ID3D11Buffer> m_pixConstantBuffer = nullptr;

	//texture
	ID3D11SamplerState* m_sampleState = nullptr;
	Texture* texture = nullptr;

	const float m_clear_color[4] = {
		1.f,1.f,1.f,1.f
	};
	bool m_loadingComplete;
	UINT m_vertex_stride = 0, m_vertex_offset = 0;
	UINT m_vertice_count, m_index_count;

	virtual void create_vertex_shader(ID3D11Device* device, const std::vector<byte>& fileData) = 0;
	virtual void create_fragment_shader(ID3D11Device* device, const std::vector<byte>& fileData) = 0;
	void initialize_vertices(ID3D11Device* device, const float* vdata);
	void initialize_indices(ID3D11Device* device, const unsigned short* idata);
	void initialize_mesh_others(ID3D11Device* device){}
};
#endif