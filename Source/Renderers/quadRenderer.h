#ifndef QUAD_RENDERER_H
#define QUAD_RENDERER_H
#include <D3DPipeline/Texture.h>
#include <Renderers/baseRenderer.h>
class quadRenderer:public baseRenderer{
public:
	quadRenderer(ID3D11Device* device);
	quadRenderer(ID3D11Device* device, DirectX::XMFLOAT4 color);
	quadRenderer(ID3D11Device* device, DirectX::XMFLOAT4 color, const float* vdata);

	quadRenderer(ID3D11Device* device, const wchar_t* vname, const wchar_t* pname);
	quadRenderer(ID3D11Device* device, const wchar_t* vname, const wchar_t* pname, const float* vdata);
	quadRenderer(ID3D11Device* device, const wchar_t* vname, const wchar_t* pname, 
		const float* vdata, const unsigned short* idata, 
		UINT vertice_num, UINT idx_num,
		dvr::INPUT_LAYOUT_IDS layout_id = dvr::INPUT_POS_TEX_2D);
	
	bool Draw(ID3D11DeviceContext* context, DirectX::XMMATRIX);
	void setTexture(Texture* tex) { texture = tex; }
protected:
	void create_vertex_shader(ID3D11Device* device, const std::vector<byte>& fileData);
	void create_fragment_shader(ID3D11Device* device, const std::vector<byte>& fileData);
private:
	dvr::ModelViewProjectionConstantBuffer m_constantBufferData;
	bool m_as_render_target;
	dvr::INPUT_LAYOUT_IDS m_input_layout_id;
};
#endif 
