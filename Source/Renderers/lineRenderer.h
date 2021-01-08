#ifndef LINE_RENDERER_H
#define LINE_RENDERER_H

#include <Renderers/baseRenderer.h>
class lineRenderer:public baseRenderer{
public:
	lineRenderer(ID3D11Device* device, int uid);
	lineRenderer(ID3D11Device* device, int uid, int point_num, const float* data);
	void updateVertices(ID3D11Device* device, int point_num, const float* data);
	bool Draw(ID3D11DeviceContext* context, DirectX::XMMATRIX);
protected:
	void create_vertex_shader(ID3D11Device* device, const std::vector<byte>& fileData);
	void create_fragment_shader(ID3D11Device* device, const std::vector<byte>& fileData);
private:
	int m_uid;
	const int MAX_POINT_NUM = 4000;
	dvr::ModelViewProjectionConstantBuffer m_constantBufferData;
	ID3D11BlendState* d3dBlendState;

};
#endif 
