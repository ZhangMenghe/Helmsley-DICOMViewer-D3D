#ifndef QUAD_RENDERER_H
#define QUAD_RENDERER_H
#include <D3DPipeline/Texture.h>
#include <Renderers/baseRenderer.h>
class quadRenderer:public baseRenderer{
public:
	quadRenderer(ID3D11Device* device, bool as_render_target = false);
	quadRenderer(ID3D11Device* device, const wchar_t* vname, const wchar_t* pname);
	quadRenderer(ID3D11Device* device, const wchar_t* vname, const wchar_t* pname, const float* vdata);
	bool setQuadSize(ID3D11Device* device, ID3D11DeviceContext* context, float width, float height);
	void Draw(ID3D11DeviceContext* context, DirectX::XMMATRIX);
	void setTexture(Texture* tex) { texture = tex; }
protected:
	void create_vertex_shader(ID3D11Device* device, const std::vector<byte>& fileData);
	void create_fragment_shader(ID3D11Device* device, const std::vector<byte>& fileData);
private:
	dvr::ModelViewProjectionConstantBuffer m_constantBufferData;
	bool m_as_render_target;
};
#endif 
