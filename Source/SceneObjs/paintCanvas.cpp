#include "pch.h"
#include "paintCanvas.h"
#include <Utils/TypeConvertUtils.h>
#include <Common/ConstantAndStruct.h>
#include <Common/uiTestHelper.h>
#include <opencv2/imgproc.hpp>
paintCanvas::paintCanvas(ID3D11Device* device, ID3D11DeviceContext* context, UINT ph, UINT pw, glm::vec4 color)
:m_ph(ph), m_pw(pw),
m_brush_type(DRAW_BRUSH_TYPE_ROUND), m_brush_radius(dvr::DRAW_2D_BRUSH_RADIUS),
m_brush_color(cv::Scalar(255,255,255,255)){
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
	m_canvas = cv::Mat::zeros(ph, pw, CV_8UC4);
	fillArrayWithRGBA(canvas_tex_data, ph * pw, color);
	memcpy(m_canvas.ptr(), canvas_tex_data, ph * pw * 4);
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

	D3D11_BOX destRegion;
	if (m_tex_u < 0) {
		destRegion.left = std::max(0, int(curr_tex_u - m_brush_radius));
		destRegion.right = std::min(m_pw, UINT(curr_tex_u + m_brush_radius));
		destRegion.top = std::max(0, int(curr_tex_v - m_brush_radius));
		destRegion.bottom = std::min(m_ph, UINT(curr_tex_v + m_brush_radius));
		destRegion.front = 0; destRegion.back = 1;
		m_tex_u = curr_tex_u; m_tex_v = curr_tex_v;
	}
	else {
		destRegion.left = std::max(0, int(std::min(curr_tex_u, m_tex_u) - m_brush_radius));
		destRegion.right = std::min(m_pw, UINT(std::max(curr_tex_u, m_tex_u) + m_brush_radius));
		destRegion.top = std::max(0, int(std::min(curr_tex_v, m_tex_v) - m_brush_radius));
		destRegion.bottom = std::min(m_ph, UINT(std::max(curr_tex_v, m_tex_v) + m_brush_radius));
		destRegion.front = 0; destRegion.back = 1;
	}
	
	cv::line(m_canvas, cv::Point(m_tex_u, m_tex_v), cv::Point(curr_tex_u, curr_tex_v), m_brush_color, m_brush_radius, cv::LINE_8);

	int box_width = destRegion.right - destRegion.left;
	int box_height = destRegion.bottom - destRegion.top;
	cv::Mat sub_canvas = m_canvas(cv::Rect(destRegion.left, destRegion.top, box_width, box_height)).clone();

	m_tex->setTexData(context, destRegion, box_height, box_width, 1, 4, sub_canvas.ptr());

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


