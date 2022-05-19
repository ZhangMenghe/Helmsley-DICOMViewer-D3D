#ifndef PAINT_CANVAS_H
#define PAINT_CANVAS_H
#include <winrt/base.h>
#include <Common/DeviceResources.h>
#include <D3DPipeline/Texture.h>

enum DRAW_BRUSH_TYPE {
	DRAW_BRUSH_TYPE_SQUARE = 0,
	DRAW_BRUSH_TYPE_ROUND,
	DRAW_BRUSH_TYPE_IDK,
	DRAW_BRUSH_TYPE_IDK2
};

class paintCanvas {
public:
	paintCanvas(ID3D11Device* device, ID3D11DeviceContext* context, 
		UINT ph, UINT pw, glm::vec4 color);
	Texture* getCanvasTexture() { return m_tex.get(); }
	

	void setBrushPos(ID3D11DeviceContext* context, glm::vec3 proj_pos, glm::vec3 proj_size, float px, float py);
	void onBrushDraw(ID3D11DeviceContext* context, float px, float py);
	
	void onBrushDraw(ID3D11DeviceContext* context, glm::vec3 center,
		std::vector<int> pos, std::vector<unsigned char> value);

	void onBrushDrawWithDepthForce(ID3D11DeviceContext* context, float px, float py, int delta);
	void onBrushUp() { m_isdrawing = false; }
	//bool stepCubeAnnotation(ID3D11DeviceContext* context, dvr::ANNOTATE_DIR dir, bool isBrush);
	//void setBrushCenter(glm::vec3 center) { m_brush_center = center; }
	//bool Draw(ID3D11DeviceContext* context, DirectX::XMMATRIX modelMat);
private:
	//texture
	std::unique_ptr<Texture> m_tex;
	
	UINT m_ph, m_pw;
	//float m_px, m_py, m_pz;
	float m_tex_u, m_tex_v;
	glm::vec3 m_canvas_offset, m_canvas_size;

	bool m_isdrawing = false;
	//Brush
	DRAW_BRUSH_TYPE m_brush_type;
	glm::vec3 m_brush_center;
	int m_brush_radius;
	float m_depth_offset;
	bool m_interpolate_sparse = true;

	void EvaluateBrushSize();
	void GenerateUpdateMask(float curr_tex_u, float curr_tex_v, D3D11_BOX& box, BYTE*& mask);
};
#endif