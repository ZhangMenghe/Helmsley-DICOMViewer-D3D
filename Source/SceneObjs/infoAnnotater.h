#ifndef INFO_ANNOTATER_H
#define INFO_ANNOTATER_H
#include <winrt/base.h>
#include <Common/DeviceResources.h>
#include <D3DPipeline/Texture.h>
#include <Renderers/sphereRenderer.h>
enum INFO_ANNOTATION_TARGET {
	ANNOTATE_INFO_BRUSH_H = 0,
	ANNOTATE_INFO_BRUSH_S,
	ANNOTATE_INFO_IDK1,
	ANNOTATE_INFO_IDK2
};
class infoAnnotater {
public:
	infoAnnotater();
	void onCreateCanvas(ID3D11Device* device, glm::vec3 vol_dim_scale, UINT ph, UINT pw, UINT pd);
	Texture* getCanvasTexture() { return tex_info.get(); }

	bool stepCubeAnnotation(ID3D11DeviceContext* context, dvr::ANNOTATE_DIR dir, bool isBrush);
	void setBrushCenter(glm::vec3 center) { m_brush_center = center; }
	bool Draw(ID3D11DeviceContext* context, DirectX::XMMATRIX modelMat);
private:
	//texture
	std::unique_ptr<Texture> tex_info;
	//unsigned char*** m_brush_map = nullptr;

	const float m_step_size = 0.05f;
	//Brush position
	glm::vec3 m_brush_center;
	int m_brush_radius;


	UINT m_ph, m_pw, m_pd;
	glm::vec3 m_vol_dim_scale;

	sphereRenderer* m_sphere;

	bool onDrawCube(ID3D11DeviceContext* context, glm::vec3 center,
		glm::vec3 vol_dim_scale,
		int sz, std::vector<int> pos, std::vector<unsigned char> value);
};
#endif