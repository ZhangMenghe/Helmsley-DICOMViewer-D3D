#ifndef SCREEN_QUAD_RENDERER_H
#define SCREEN_QUAD_RENDERER_H
#include "pch.h"
#include <Renderers/quadRenderer.h>

class screenQuadRenderer:public quadRenderer {
public:
	static screenQuadRenderer* instance();
	screenQuadRenderer(ID3D11Device* device);
	bool InitializeQuadTex(ID3D11Device* device, ID3D11DeviceContext* context, float width, float height);
	void Draw(ID3D11DeviceContext* context);
	void SetToDrawTarget(ID3D11DeviceContext* context, ID3D11DepthStencilView* pDepthStencilView);
private:
	static screenQuadRenderer* myPtr_;
	dvr::ModelViewProjectionConstantBuffer m_constantBufferData;
	ID3D11BlendState* d3dBlendState;

};
#endif 
