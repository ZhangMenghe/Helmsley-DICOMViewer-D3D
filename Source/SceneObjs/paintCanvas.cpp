#include "pch.h"
#include "paintCanvas.h"
#include <Utils/TypeConvertUtils.h>
#include <Common/ConstantAndStruct.h>
#include <Common/uiTestHelper.h>
paintCanvas::paintCanvas(ID3D11Device* device, ID3D11DeviceContext* context, UINT ph, UINT pw, glm::vec4 color)
:m_ph(ph), m_pw(pw),
m_brush_type(DRAW_BRUSH_TYPE_ROUND), m_brush_radius(dvr::DRAW_2D_BRUSH_RADIUS),
m_brush_color(glm::vec4(255)){
	m_tex = std::make_unique<Texture>();
	DXGI_SAMPLE_DESC sample_desc{ 1,0 };
	D3D11_TEXTURE2D_DESC texInfoDesc{
		pw, ph,
		1, 1,
		DXGI_FORMAT_R8G8B8A8_UNORM,
		sample_desc,
		D3D11_USAGE_DEFAULT,
		D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE,
		0,
		0
	};
	BYTE* canvas_tex_data = new unsigned char[ph * pw * 4];
	fillArrayWithRGBA(canvas_tex_data, ph * pw, color);
	m_tex->Initialize(device, context, texInfoDesc, canvas_tex_data);
}
void paintCanvas::setBrushPos(ID3D11DeviceContext* context, float px, float py) {
	//calculate
	m_canvas_offset = glm::vec3(.0f);
	m_canvas_size = glm::vec3(1.0f);
	m_tex_u = -1; m_tex_v = -1;
	onBrushDraw(context, px, py);
	m_isdrawing = true;
}
void paintCanvas::setBrushPos(ID3D11DeviceContext* context, glm::vec3 proj_pos, glm::vec3 proj_size, float px, float py) {
	m_canvas_offset = proj_pos;
	m_canvas_size = proj_size;
	m_tex_u = -1; m_tex_v = -1;
	onBrushDraw(context, px, py);
	m_isdrawing = true;
}
void paintCanvas::onBrushDraw(ID3D11DeviceContext* context, float px, float py) {
	if (!m_isdrawing) return;
	float curr_tex_u = (px - m_canvas_offset.x) / m_canvas_size.x * m_pw;
	float curr_tex_v = (py - m_canvas_offset.y) / m_canvas_size.y * m_ph;


	if (curr_tex_u < 0 || curr_tex_u >= m_pw || curr_tex_v < 0 || curr_tex_v >= m_ph) { m_isdrawing = false; return; }

	std::vector<float> interpx, interpy;
	if (m_interpolate_sparse
		&& m_tex_u>0
		&& !HDUI::bounding_boxes_overlap(m_tex_u, m_tex_v, curr_tex_u, curr_tex_v, m_brush_radius)) {
		
		int x0 = m_tex_u, y0 = m_tex_v;
		int x1 = curr_tex_u, y1 = curr_tex_v;

		float slope = float(y1-y0) / (x1-x0);
		if (fabs(slope) < 1.0f) {
			for (auto cx = std::min(x0, x1); cx < std::max(x0, x1); cx += m_brush_radius) {
				auto cy = y0 + slope * (cx - x0);
				interpx.push_back(cx); interpy.push_back(cy);
			}
		}else{
			slope = 1.0f / slope;
			for (auto cy = std::min(y0, y1); cy < std::max(y0, y1); cy += 1.4f * m_brush_radius) {
				auto cx = x0 + slope * (cy - y0);
				interpx.push_back(cx); interpy.push_back(cy);
			}
		}

	}
	interpx.push_back(curr_tex_u); interpy.push_back(curr_tex_v);

	D3D11_BOX destRegion;
	BYTE* mask = nullptr;

	destRegion.left = std::max(0, int(*std::min_element(interpx.begin(), interpx.end()) - m_brush_radius));
	destRegion.right = std::min(m_pw, UINT(*std::max_element(interpx.begin(), interpx.end()) + m_brush_radius));
	destRegion.top = std::max(0, int(*std::min_element(interpy.begin(), interpy.end()) - m_brush_radius));
	destRegion.bottom = std::min(m_ph, UINT(*std::max_element(interpy.begin(), interpy.end()) + m_brush_radius));
	destRegion.front = 0; destRegion.back = 1;

	int box_width = destRegion.right - destRegion.left;
	int box_height = destRegion.bottom - destRegion.top;

	size_t sz = box_width * box_height;
	mask = new BYTE[sz];
	memset(mask, 0x00, sz);
	for (int i = 0; i < interpx.size(); i++) {
		int cx = interpx[i] - destRegion.left;
		int cy = interpy[i] - destRegion.top;

		//fill with square
		for (auto y = std::max(0, cy - m_brush_radius); y < std::min(box_height, cy + m_brush_radius); y++) {
			int line_start = y * box_width;
			memset(mask + line_start + (cx - m_brush_radius), 0xff, std::min(2 * m_brush_radius, box_width - (cx - m_brush_radius)));
		}
	}

	m_tex->setTexData(context, &destRegion, { 0,1,2,3 }, { (BYTE)m_brush_color.r, (BYTE)m_brush_color.g, (BYTE)m_brush_color.b, (BYTE)m_brush_color.a }, 4, mask);
	m_tex_u = curr_tex_u; m_tex_v = curr_tex_v;
}
/*
* Function to convert distance to depth force to brush size
dist: projected distance to the 2D plane(m)
a finger offset if ussally a few cms (0.0x m)
*/
void paintCanvas::setDepthForceBrushSize(float dist) {
	m_depth_offset = int(dist * 100) * 0.1f;
	EvaluateBrushSize();
}
void paintCanvas::onBrushDrawWithDepthForce(ID3D11DeviceContext* context, float px, float py, int delta) {
	m_depth_offset += float(delta / 120.0f) * 0.1f;
	EvaluateBrushSize();
	onBrushDraw(context, px, py);
}

void paintCanvas::EvaluateBrushSize() {
	m_brush_radius = dvr::DRAW_2D_BRUSH_RADIUS * (1.0f + m_depth_offset);
}


