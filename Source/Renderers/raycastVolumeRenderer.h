#ifndef RAYCAST_VOLUME_RENDERER_H
#define RAYCAST_VOLUME_RENDERER_H

#include <D3DPipeline/Texture.h>
#include <Renderers/baseRenderer.h>

struct raycastConstantBuffer
{
	DirectX::XMFLOAT4X4 uModelMat;
	DirectX::XMFLOAT4X4 uViewProjMat;
	DirectX::XMFLOAT4 uCamPosInObjSpace;
};
struct raypixConstantBuffer {
	alignas(16)bool u_cut;
	alignas(16)DirectX::XMFLOAT4 u_pp;
	alignas(16)DirectX::XMFLOAT4 u_pn;
};

class raycastVolumeRenderer:public baseRenderer {
public:
	raycastVolumeRenderer(ID3D11Device* device);

	void Draw(ID3D11DeviceContext* context, Texture* tex, DirectX::XMMATRIX modelMat);
protected:
	void create_vertex_shader(ID3D11Device* device, const std::vector<byte>& fileData);
	void create_fragment_shader(ID3D11Device* device, const std::vector<byte>& fileData);
private:
	winrt::com_ptr<ID3D11Buffer> m_planeConstantBuffer = nullptr;
	raycastConstantBuffer m_const_buff_data;
	raypixConstantBuffer m_pix_const_buff_data;

	ID3D11BlendState* d3dBlendState;
	ID3D11RasterizerState* m_render_state;
};
#endif