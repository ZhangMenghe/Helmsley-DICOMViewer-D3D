#ifndef QUAD_RENDERER_H
#define QUAD_RENDERER_H
#include <Utils/ShaderStructures.h>
#include <D3DPipeline/Texture.h>
#include <Renderers/baseRenderer.h>
class quadRenderer:public baseRenderer{
public:
	quadRenderer(ID3D11Device* device);
	bool setQuadSize(ID3D11Device* device, ID3D11DeviceContext* context, float width, float height);
	void Draw(ID3D11DeviceContext* context);
	void updateMatrix(allConstantBuffer buff_data) {
		m_constantBufferData.projection = buff_data.projection;
		m_constantBufferData.view = buff_data.view;
	}
protected:
	void create_vertex_shader(ID3D11Device* device, const std::vector<byte>& fileData);
	void create_fragment_shader(ID3D11Device* device, const std::vector<byte>& fileData);
private:
	ModelViewProjectionConstantBuffer m_constantBufferData;
};
#endif 
