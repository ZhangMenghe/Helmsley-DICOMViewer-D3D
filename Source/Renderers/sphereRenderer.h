#ifndef SPHERE_RENDERER_H
#define SPHERE_RENDERER_H

#include <D3DPipeline/Texture.h>
#include <Renderers/baseRenderer.h>
#include <Renderers/lineRenderer.h>

class sphereRenderer:public baseRenderer{
public:
	sphereRenderer(ID3D11Device* device, int stackCount, int sectorCount, DirectX::XMFLOAT4 color);
	bool Draw(ID3D11DeviceContext* context, DirectX::XMMATRIX);
protected:
	virtual void create_vertex_shader(ID3D11Device* device, const std::vector<byte>& fileData);
	virtual void create_fragment_shader(ID3D11Device* device, const std::vector<byte>& fileData);
private:
	dvr::ModelViewProjectionConstantBuffer m_constantBufferData;
	DirectX::XMFLOAT4 m_color;
	std::vector<float>m_vertices;
	std::vector<unsigned short> m_indices, m_line_indices;

	std::unique_ptr<lineRenderer> m_line_renderer;
};
#endif 
