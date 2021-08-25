#ifndef VIEW_ALIGNED_SLICING_RENDERER_H
#define VIEW_ALIGNED_SLICING_RENDERER_H

#include <D3DPipeline/Texture.h>
#include <Renderers/baseDicomRenderer.h>
#include <Common/ConstantAndStruct.h>

struct texSlicingPixConstantBuffer {
	alignas(16)bool u_front;
	alignas(16)bool u_cut;
	alignas(16)float u_dist;
	alignas(16)DirectX::XMFLOAT4 u_cut_point;
	alignas(16)DirectX::XMFLOAT4 u_cut_normal;
	alignas(16)DirectX::XMFLOAT4 u_plane_normal;
};

class viewAlignedSlicingRenderer :public baseDicomRenderer {
public:
	viewAlignedSlicingRenderer(ID3D11Device* device);

	bool Draw(ID3D11DeviceContext* context, Texture* tex, DirectX::XMMATRIX modelMat, bool is_front);
	void setDimension(ID3D11Device* device, glm::vec3 vol_dimension, glm::vec3 vol_dim_scale);
	void setCuttingPlane(float percent);
	void setCuttingPlaneDelta(int delta);
	void setRenderingParameters(float* values){}

	void updateVertices(ID3D11DeviceContext* context, glm::mat4 model_mat);
protected:
	void create_vertex_shader(ID3D11Device* device, const std::vector<byte>& fileData);
	void create_fragment_shader(ID3D11Device* device, const std::vector<byte>& fileData);
private:
	const float DENSE_FACTOR = 6.f;
	int dimensions; float dimension_inv;
	float vol_thickness_factor = 1.0f;
	const static int MAX_DIMENSIONS = 30;
	int m_indice_num[MAX_DIMENSIONS];
	bool b_init_successful = false;
	int cut_id = 0;
	bool m_data_dirty = false;
	bool m_baked_dirty = true;
	bool m_right_order;
	glm::vec3 m_last_vec3 = glm::vec3(1e6);
	int m_slice_num;
	const unsigned short m_indices_data[12] = {
			0,1,2,0,2,3,0,3,4,0,4,5
	};

	glm::vec3 pn_local;
	float slice_spacing;

	//D3D11_SUBRESOURCE_DATA m_instance_data_front, m_instance_data_back;
	//InstanceType* zInfos_front, *zInfos_back;
	std::vector<baseRenderer*> renderers;

	//buffer
	winrt::com_ptr<ID3D11Buffer> m_instanceBuffer_front = nullptr, m_instanceBuffer_back = nullptr;

	int m_vertexCount;
	int m_instanceCount;

	dvr::ModelViewProjectionConstantBuffer m_const_buff_data;
	texSlicingPixConstantBuffer m_const_buff_data_pix;
};
#endif