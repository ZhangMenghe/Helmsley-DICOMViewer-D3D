#ifndef RAYCAST_VOLUME_RENDERER_H
#define RAYCAST_VOLUME_RENDERER_H

#include <D3DPipeline/Texture.h>
#include <Renderers/baseRenderer.h>

struct raycastConstantBuffer
{
	DirectX::XMFLOAT4X4 uModelMat;
	DirectX::XMFLOAT4X4 uViewProjMat;
	DirectX::XMFLOAT3 uCamPosInObjSpace;
};

class raycastVolumeRenderer:public baseRenderer {
public:
	raycastVolumeRenderer(ID3D11Device* device);
	void Draw(ID3D11DeviceContext* context);
protected:
	void create_vertex_shader(ID3D11Device* device, const std::vector<byte>& fileData);
	void create_fragment_shader(ID3D11Device* device, const std::vector<byte>& fileData);
private:
	raycastConstantBuffer m_const_buff_data;
};
#endif