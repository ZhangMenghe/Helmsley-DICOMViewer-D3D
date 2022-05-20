#include "pch.h"
#include "paintCanvas.h"
#include <Utils/TypeConvertUtils.h>
#include <Common/ConstantAndStruct.h>
#include <Common/uiTestHelper.h>
paintCanvas::paintCanvas(ID3D11Device* device, ID3D11DeviceContext* context, UINT ph, UINT pw, glm::vec4 color)
:m_ph(ph), m_pw(pw),
m_brush_type(DRAW_BRUSH_TYPE_ROUND), m_brush_radius(dvr::DRAW_2D_BRUSH_RADIUS){
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
void paintCanvas::setBrushPos(ID3D11DeviceContext* context, glm::vec3 proj_pos, glm::vec3 proj_size, float px, float py) {
	m_canvas_offset = proj_pos;
	m_canvas_size = proj_size;
	m_tex_u = -1; m_tex_v = -1;
	onBrushDraw(context, px, py);
	m_isdrawing = true;

	//float start_u = 300, start_v = 300, step = 50;
	//float mpx = start_u, mpy = start_v;
	//for (int i = 0; i < 3; i++) {
	//	onBrushDraw(context, mpx, mpy);
	//	mpx += step; mpy = 0.5 * (mpx - start_u) + start_v;
	//}
}
void paintCanvas::onBrushDraw(ID3D11DeviceContext* context, float px, float py) {
	if (!m_isdrawing) return;
	float curr_tex_u = (px - m_canvas_offset.x) / m_canvas_size.x * m_pw;
	float curr_tex_v = (py - m_canvas_offset.y) / m_canvas_size.y * m_ph;
	//float curr_tex_u = px;
	//float curr_tex_v = py;


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

	/*for (int i = 0; i < interpx.size(); i++) {
		D3D11_BOX destRegion;
		BYTE* mask = nullptr;
		GenerateUpdateMask(interpx[i], interpy[i], destRegion, mask);
		if (mask) {
			m_tex->setTexData(context, &destRegion, { 0,1,2,3 }, { 0xff, 0xff, 0xff, 0xff }, 4, mask);
			delete mask; mask = nullptr;
		}
	}*/
	D3D11_BOX destRegion;
	BYTE* mask = nullptr;

	//if (interpx.size() == 1) {
	//	GenerateUpdateMask(interpx.front(), interpy.front(), destRegion, mask);
	//}
	//else {
		destRegion.left = std::max(0, int(*std::min_element(interpx.begin(), interpx.end()) - m_brush_radius));
		destRegion.right = std::min(m_pw, UINT(*std::max_element(interpx.begin(), interpx.end()) + m_brush_radius));
		destRegion.top = std::max(0, int(*std::min_element(interpy.begin(), interpy.end()) - m_brush_radius));
		destRegion.bottom = std::min(m_ph, UINT(*std::max_element(interpy.begin(), interpy.end()) + m_brush_radius));
		destRegion.front = 0; destRegion.back = 1;

		int box_width = destRegion.right - destRegion.left;
		int box_height = destRegion.bottom - destRegion.top;

		//std::vector<std::vector<int>> mask;
		
		//mask.resize(box_height, std::vector<int>(box_width, 0));
		size_t sz = box_width * box_height;
		mask = new BYTE[sz];
		memset(mask, 0x00, sz);

		for (int i = 0; i < interpx.size(); i++) {
			int cx = interpx[i] - destRegion.left;
			int cy = interpy[i] - destRegion.top;
			//mask[cy * box_width + cx] = 0xff;

			for (auto y = std::max(0, cy - m_brush_radius); y < std::min(box_height, cy + m_brush_radius); y++) {
				int line_start = y * box_width;
				memset(mask + line_start + (cx - m_brush_radius), 0xff, std::min(2 * m_brush_radius, box_width - (cx - m_brush_radius)));
			}
		}
	//}

	m_tex->setTexData(context, &destRegion, { 0,1,2,3 }, { 0xff, 0xff, 0xff, 0xff }, 4, mask);
	m_tex_u = curr_tex_u; m_tex_v = curr_tex_v;


}
void paintCanvas::onBrushDrawWithDepthForce(ID3D11DeviceContext* context, float px, float py, int delta) {
	m_depth_offset += float(delta / 120.0f) * 0.1f;
	EvaluateBrushSize();
	onBrushDraw(context, px, py);
}
void paintCanvas::onBrushDraw(
	ID3D11DeviceContext* context, glm::vec3 center, 
	std::vector<int> pos, std::vector<unsigned char> value) {

	//d3d11_box: origin at left, top, front corner.
	float cpx = (center.x + 0.5f) * m_pw, cpy = (0.5f - center.y) * m_ph;

	D3D11_BOX destRegion;
	destRegion.left = std::max(0, int(cpx - m_brush_radius));
	destRegion.right = std::min(m_pw, UINT(cpx + m_brush_radius));
	destRegion.top = std::max(0, int(cpy - m_brush_radius));
	destRegion.bottom = std::min(m_ph, UINT(cpy + m_brush_radius));
	destRegion.front = 0;
	destRegion.back = 1;

	//m_tex->setTexData(context, &destRegion, pos, value, 4);
}
void paintCanvas::EvaluateBrushSize() {
	m_brush_radius = dvr::DRAW_2D_BRUSH_RADIUS * (1.0f + m_depth_offset);
}
void paintCanvas::GenerateUpdateMask(float curr_tex_u, float curr_tex_v, D3D11_BOX& box, BYTE*& mask) {

	box.left = int(curr_tex_u - m_brush_radius);
	box.right = UINT(curr_tex_u + m_brush_radius);
	box.top = int(curr_tex_v - m_brush_radius);
	box.bottom = UINT(curr_tex_v + m_brush_radius);

	box.front = 0;
	box.back = 1;

	
	if (box.right >= m_pw || box.left < 0 || box.top <0 || box.bottom > m_ph ) return;
	
	auto width = box.right - box.left;
	auto height = box.bottom - box.top;
		

	//auto depth = box->back - box->front;

	//auto row_pitch = width * 4;
	//auto depth_pitch = row_pitch * height;
	size_t sz = height * width;

	int idx = 0;
	mask = new BYTE[sz];
	
	//auto fillMask = [](BYTE*& mask, int width, int height, int cx, int cy, int r) {
	//	for (auto y = std::max(0,cy - r); y < std::min(height, cy + r); y++) {
	//		int line_start = y * width;
	//		memset(mask + line_start + (cx - r), 0xff, std::min(2 * r, width -(cx-r)));
	//	}
	//};
	//if (need_interpolate) {
	//	memset(mask, 0x00, sz);

	//	int u1 = m_tex_u, v1 = m_tex_v, u2 = curr_tex_u, v2 = curr_tex_v, r= m_brush_radius;

	//	//fillMask(mask, width, height, )
	//	auto tra_type_test = (u1 - u2) * (v1 - v2);
	//	if (tra_type_test > 0) {
	//		fillMask(mask, width, height, r, r, r);
	//		fillMask(mask, width, height, width - r, height - r, r);
	//	}
	//	else {
	//		fillMask(mask, width, height, r, height-r, r);
	//		fillMask(mask, width, height, width - r, r, r);
	//	}
	//	//if(m_tex_u > curr_tex_u) fillMask(mask, width, height, width-m_brush_radius, (m_tex_v<curr_tex_v)?m_brush_radius:height- m_brush_radius, m_brush_radius);
	//	//else fillMask(mask, width, height, m_brush_radius, (m_tex_v < curr_tex_v) ? m_brush_radius : height - m_brush_radius, m_brush_radius);
	//	
	//	//if (width >= height) {
	//	//	float slope = float(height) / width;
	//	//	//sample x
	//	//	for (auto cx = 2.0f * m_brush_radius; cx < width; cx += 2 * m_brush_radius) {
	//	//		auto cy = slope * cx + box.bottom;
	//	//		fillMask(mask, width, height, cx, cy, m_brush_radius);
	//	//	}
	//	//}
	//}else
		memset(mask, 0xff, sz);



	//memset(mask, 0xff, width *int(height * 0.2f));
}

