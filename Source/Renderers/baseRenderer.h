#ifndef BASE_RENDERER_H
#define BASE_RENDERER_H
#include "pch.h"
#include <Content/ShaderStructures.h>
#include <D3DPipeline/Texture.h>
class baseRenderer {
public:
	baseRenderer(ID3D11Device* device,
		const wchar_t* vname, const wchar_t* pname,
		const float* vdata, const unsigned short* idata);
	void updateMatrix(ModelViewProjectionConstantBuffer buff_data) {
		m_constantBufferData.projection = buff_data.projection;
		m_constantBufferData.view = buff_data.view;
	}
	virtual void Draw(ID3D11DeviceContext* context);
	virtual void Clear() {};
	ID3D11RenderTargetView* GetRenderTargetView() { return texture->GetRenderTargetView(); }
protected:
	//buffers
	winrt::com_ptr<ID3D11InputLayout> m_inputLayout;
	winrt::com_ptr<ID3D11Buffer> m_vertexBuffer = nullptr;
	winrt::com_ptr<ID3D11Buffer> m_indexBuffer = nullptr;
	winrt::com_ptr<ID3D11VertexShader> m_vertexShader = nullptr;
	winrt::com_ptr<ID3D11PixelShader> m_pixelShader = nullptr;
	winrt::com_ptr<ID3D11ComputeShader> m_computeShader = nullptr;
	winrt::com_ptr<ID3D11Buffer> m_constantBuffer = nullptr;

	//texture
	ID3D11SamplerState* m_sampleState = nullptr;
	Texture* texture = nullptr;

	ModelViewProjectionConstantBuffer	m_constantBufferData;

	const float m_clear_color[4] = {
		1.f,1.f,1.f,1.f
	};
	bool m_loadingComplete;
	UINT m_vertex_stride = 0, m_vertex_offset = 0, m_index_count;


	virtual void create_vertex_shader(ID3D11Device* device, const std::vector<byte>& fileData) = 0;
	virtual void create_fragment_shader(ID3D11Device* device, const std::vector<byte>& fileData) = 0;

};
#endif