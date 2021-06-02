#ifndef RAYCAST_VOLUME_RENDERER_H
#define RAYCAST_VOLUME_RENDERER_H

#include <D3DPipeline/Texture.h>
#include <Renderers/baseDicomRenderer.h>

struct raycastConstantBuffer
{
	DirectX::XMFLOAT4X4 uModelMat;
	DirectX::XMFLOAT4X4 uViewProjMat;
	DirectX::XMFLOAT4 uCamPosInObjSpace;
};
struct raypixConstantBuffer {
	alignas(16)bool u_cut;
	alignas(16)bool u_cutplane_realsample;
	alignas(16)DirectX::XMFLOAT4 u_pp;
	alignas(16)DirectX::XMFLOAT4 u_pn;
};

class raycastVolumeRenderer:public baseDicomRenderer {
public:
	raycastVolumeRenderer(ID3D11Device* device);

	bool Draw(ID3D11DeviceContext* context, Texture* tex, DirectX::XMMATRIX modelMat);
	void setRenderingParameters(float* values) { m_sample_steps = values[0]; }
protected:
	void create_vertex_shader(ID3D11Device* device, const std::vector<byte>& fileData);
	void create_fragment_shader(ID3D11Device* device, const std::vector<byte>& fileData);
private:
	float m_sample_steps = 100.f;

	winrt::com_ptr<ID3D11Buffer> m_planeConstantBuffer = nullptr;
	raycastConstantBuffer m_const_buff_data;
	raypixConstantBuffer m_pix_const_buff_data;
	ID3D11RasterizerState* m_render_state;
};
#endif