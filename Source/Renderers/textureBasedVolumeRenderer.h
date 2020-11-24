#ifndef TEXTURE_BASED_VOLUME_RENDERER_H
#define TEXTURE_BASED_VOLUME_RENDERER_H

#include <D3DPipeline/Texture.h>
#include <Renderers/baseRenderer.h>
#include <Common/ConstantAndStruct.h>

struct InstanceType{
	DirectX::XMFLOAT2 zinfo;
};
struct texPixConstantBuffer {
	bool u_front;
};
class textureBasedVolumeRenderer:public baseRenderer {
public:
	textureBasedVolumeRenderer(ID3D11Device* device);

	void Draw(ID3D11DeviceContext* context, Texture* tex, DirectX::XMMATRIX modelMat, bool is_front, bool pre_draw = true);
	void setDimension(ID3D11Device* device, DirectX::XMUINT3 vol_dimension, DirectX::XMFLOAT3 vol_dim_scale);

protected:
	void create_vertex_shader(ID3D11Device* device, const std::vector<byte>& fileData);
	void create_fragment_shader(ID3D11Device* device, const std::vector<byte>& fileData);
	void initialize_mesh_others(ID3D11Device* device);

private:
	const float DENSE_FACTOR = 1.0f;
	int dimensions; float dimension_inv;
	float vol_thickness_factor = 1.0f;
	const int MAX_DIMENSIONS = 1000;
	bool b_init_successful = false;
	int cut_id = 0;
	bool m_data_dirty = false;
	bool m_baked_dirty = true;

	//D3D11_SUBRESOURCE_DATA m_instance_data_front, m_instance_data_back;
	InstanceType* zInfos_front, *zInfos_back;

	//buffer
	winrt::com_ptr<ID3D11Buffer> m_instanceBuffer_front = nullptr, m_instanceBuffer_back = nullptr;

	int m_vertexCount;
	int m_instanceCount;

	dvr::ModelViewProjectionConstantBuffer m_const_buff_data;
	texPixConstantBuffer m_const_buff_data_pix;

	DirectX::XMMATRIX projMat, viewMat;
	ID3D11BlendState* d3dBlendState;
	ID3D11RasterizerState* m_render_state_front, * m_render_state_back;

	void draw_scene(ID3D11DeviceContext* context, Texture* tex, DirectX::XMMATRIX modelMat, bool is_front);
	void draw_baked(ID3D11DeviceContext* context, Texture* tex, DirectX::XMMATRIX modelMat, bool is_front);
};
#endif