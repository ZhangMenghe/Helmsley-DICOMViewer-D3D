#ifndef QUAD_RENDERER_H
#define QUAD_RENDERER_H
#include "ShaderStructures.h"
#include <D3DPipeline/Texture.h>
class quadRenderer{
public:
	quadRenderer(ID3D11Device* device);
	bool setQuadSize(ID3D11Device* device, ID3D11DeviceContext* context, float width, float height);

	void Draw(ID3D11DeviceContext* context);
	void Clear(){}
	ID3D11RenderTargetView*  GetRenderTargetView() { return texture->GetRenderTargetView(); }
private:
	ID3D11Device* m_device_ref;

	// Direct3D resources for cube geometry.
	ID3D11InputLayout*	m_inputLayout;
	ID3D11Buffer*		m_vertexBuffer;
	ID3D11Buffer*		m_indexBuffer;
	ID3D11VertexShader*	m_vertexShader;
	ID3D11PixelShader*	m_pixelShader;
	ID3D11Buffer*		m_constantBuffer;

	//texture
	ID3D11SamplerState* m_sampleState;
	Texture* texture;
	bool	m_loadingComplete;

	ModelViewProjectionConstantBuffer	m_constantBufferData;

	const float m_clear_color[4] = {
		1.f,1.f,1.f,1.f
	};
};
#endif 
