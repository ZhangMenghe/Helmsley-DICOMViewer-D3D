#ifndef ORGAN_MESH_RENDERER_H
#define ORGAN_MESH_RENDERER_H

#include <D3DPipeline/Texture.h>
#include <Renderers/baseRenderer.h>
struct MarchingCubeOutputVertex {
	DirectX::XMFLOAT3 pos;
	DirectX::XMFLOAT3 norm;
};
struct mcConstantBuffer {
	DirectX::XMUINT4 u_grid_size;
	DirectX::XMUINT4 u_volume_size;
	UINT u_maskbits;
	UINT u_organ_num;
};
class organMeshRenderer :public baseRenderer {
public:
	organMeshRenderer(ID3D11Device* device);
	void Setup(ID3D11Device* device, UINT h, UINT w, UINT d);
	bool Draw(ID3D11DeviceContext* context, Texture* tex_vol, DirectX::XMMATRIX modelMat);
protected:
	void create_vertex_shader(ID3D11Device* device, const std::vector<byte>& fileData);
	void create_fragment_shader(ID3D11Device* device, const std::vector<byte>& fileData);
private:
	const float grid_factor = 0.25f;
	const size_t max_triangles_per_cell = 5;
	const size_t max_vertices_per_triangle = 3;

	ID3D11ComputeShader* m_computeShader = nullptr;

	//for compute shader input 
	ID3D11Buffer* m_computeInBuff_tri, *m_computeInBuff_config;
	ID3D11ShaderResourceView *m_computeInSRV_tri, *m_computeInSRV_config;
	ID3D11Texture2D* m_triTableTex;
	ID3D11ShaderResourceView* m_triTableSRV = nullptr;

	//for compute shader output
	ID3D11Buffer* m_computeOutBuff = nullptr;
	//ID3D11Buffer* m_computeResultBuff = nullptr;
	ID3D11UnorderedAccessView* m_computeUAV = nullptr;

	//for compute shader constant 
	ID3D11Buffer* m_computeConstBuff = nullptr;
	mcConstantBuffer m_computeConstData;

	ID3D11RasterizerState* m_RasterizerState;
	dvr::ModelViewProjectionConstantBuffer m_const_buff_data;

	bool m_baked_dirty = false;
};
#endif