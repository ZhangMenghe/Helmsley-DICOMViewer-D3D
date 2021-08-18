#ifndef TEXTURE_BASED_VOLUME_RENDERER_H
#define TEXTURE_BASED_VOLUME_RENDERER_H

#include <D3DPipeline/Texture.h>
#include <Renderers/baseDicomRenderer.h>
#include <Common/ConstantAndStruct.h>
struct texbasedVertexBuffer {
	dvr::MVPConstantBuffer uMVP;
	alignas(16) float u_volume_thickness;
};
class textureBasedVolumeRenderer :public baseDicomRenderer {
public:
	textureBasedVolumeRenderer(ID3D11Device* device);

	bool Draw(ID3D11DeviceContext* context, Texture* tex, DirectX::XMMATRIX modelMat, bool is_front = true);
	void setDimension(ID3D11Device* device, glm::vec3 vol_dimension, glm::vec3 vol_dim_scale);
	void setCuttingPlane(float percent);
	void setCuttingPlaneDelta(int delta);
	void setRenderingParameters(float* values);

protected:
	void create_vertex_shader(ID3D11Device* device, const std::vector<byte>& fileData);
	void create_fragment_shader(ID3D11Device* device, const std::vector<byte>& fileData);

private:
	const int MAX_DIMENSIONS = 1000;
	int dimension_draw;float dimension_draw_inv;
	float* m_vertices_front, * m_vertices_back;
	unsigned short* m_indices = nullptr;

	//parameters
	float dense_factor = 1.0f;
	float vol_thickness_factor = 1.0f;
	int cut_id = 0;

	//flags
	bool m_instance_data_dirty = false;

	//bool m_baked_dirty = true;
	//D3D11_SUBRESOURCE_DATA m_instance_data_front, m_instance_data_back;
	//InstanceType* zInfos_front, * zInfos_back;

	//buffer
	//winrt::com_ptr<ID3D11Buffer> m_instanceBuffer_front = nullptr, m_instanceBuffer_back = nullptr;

	//int m_vertexCount;
	//int m_instanceCount;

	texbasedVertexBuffer m_vertex_const_buff_data;
	dvr::cutPlaneConstantBuffer m_pix_const_buff_data;

	void initialize_vertices_and_indices(ID3D11Device* device);
	void on_update_dimension_draw();
	void update_instance_data(ID3D11DeviceContext* context);
};
#endif