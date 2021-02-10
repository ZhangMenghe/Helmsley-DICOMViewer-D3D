#ifndef SLATE_CAMERA_RENDERER_H
#define SLATE_CAMERA_RENDERER_H

#include <Common/DeviceResources.h>
#include <Common/StepTimer.h>
#include <D3DPipeline/Texture.h>
#include <Renderers/baseRenderer.h>

class SlateCameraRenderer:public baseRenderer {
public:
	SlateCameraRenderer(ID3D11Device* device);
	bool Draw(ID3D11DeviceContext* context, DirectX::XMMATRIX);
	void setTexture(Texture* tex) { texture = tex; }
protected:
	virtual void create_vertex_shader(ID3D11Device* device, const std::vector<byte>& fileData);
	virtual void create_fragment_shader(ID3D11Device* device, const std::vector<byte>& fileData);
private:
	dvr::ModelViewProjectionConstantBuffer m_constantBufferData;
	bool m_as_render_target;
	dvr::INPUT_LAYOUT_IDS m_input_layout_id;
};
#endif // !SLATE_CAMERA_RENDERER_H

