#ifndef INFO_ANNOTATER_H
#define INFO_ANNOTATER_H
#include <winrt/base.h>
#include <Common/DeviceResources.h>
#include <D3DPipeline/Texture.h>
#include <Renderers/sphereRenderer.h>
class infoAnnotater {
public:
	infoAnnotater();
	void onCreateCanvas(ID3D11Device* device, UINT ph, UINT pw, UINT pd);
	Texture* getCanvasTexture() { return tex_info.get(); }
	void onDrawCube(ID3D11DeviceContext* context, glm::vec3 center,
		glm::vec3 vol_dim_scale,
		int sz, std::vector<int> pos, std::vector<unsigned char> value);
	bool Draw(ID3D11DeviceContext* context, DirectX::XMMATRIX modelMat);
private:
	//texture
	std::unique_ptr<Texture> tex_info;
	UINT m_ph, m_pw, m_pd;
	sphereRenderer* m_sphere;
};
#endif