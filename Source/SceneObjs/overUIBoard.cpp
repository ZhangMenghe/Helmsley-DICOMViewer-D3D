#include "pch.h"
#include "overUIBoard.h"
#include <Common/DirectXHelper.h>
#include <Utils/TypeConvertUtils.h>
#include <D3DPipeline/Primitive.h>
#include "strsafe.h"
namespace {
	// Test if a 2D point (x,y) is in polygon with npol edges and xp,yp vertices
	// The following code is by Randolph Franklin, it returns 1 for interior points and 0 for exterior points.
	int pnpoly(int npol, float* xp, float* yp, float x, float y) {
		int i, j, c = 0;
		for (i = 0, j = npol - 1; i < npol; j = i++) {
			if ((((yp[i] <= y) && (y < yp[j])) ||
				((yp[j] <= y) && (y < yp[i]))) &&
				(x < (xp[j] - xp[i]) * (y - yp[i]) / (yp[j] - yp[i]) + xp[i]))
				c = !c;
		}
		return c;
	}
	// Test if a sphere intersect with plane
	bool sphere_intersects_plane_point(glm::vec3 sc, float sr, glm::vec3 pp, glm::vec3 pn, float hprx, float hpry) {
		float d = glm::dot((sc - pp), pn);
		if (fabs(d) <= sr) {
			glm::vec3 proj = pn * d;
			auto point = sc - proj;
			float xps[4];
			float yps[4];
			xps[0] = pp.x - hprx; xps[1] = pp.x + hprx; xps[2] = pp.x + hprx; xps[3] = pp.x - hprx;
			yps[0] = pp.y + hpry; yps[1] = pp.y + hpry; yps[2] = pp.y - hpry; yps[3] = pp.y - hpry;

			return (pnpoly(4, xps, yps, point.x, point.y) == 1);
		}
		return false;
	}
}
overUIBoard::overUIBoard(const std::shared_ptr<DX::DeviceResources>& deviceResources)
:m_deviceResources(deviceResources){
	auto outputSize = m_deviceResources->GetOutputSize();
	m_screen_x = outputSize.Width; m_screen_y = outputSize.Height;
}
void overUIBoard::onWindowSizeChanged() {
	auto outputSize = m_deviceResources->GetOutputSize();
	if (outputSize.Width == 0 || !m_background_board || (m_screen_x == outputSize.Width && m_screen_y == outputSize.Height)) return;
	auto op = m_background_board->pos;
	auto osz = m_background_board->size;
	auto sz = glm::vec3(osz.x * 0.4f, osz.y * 0.2f, osz.z);

	int id = 0;
	for (auto& tq : m_tquads) {
		auto tx = id % 2 ? op.x + 0.5f * sz.x + 0.02 * osz.x : op.x - 0.5f * sz.x - 0.02 * osz.x;
		auto ty = op.y + m_background_board->size.y * 0.5f - sz.y * (int(id / 2) + 1);

		tq.second.size = glm::vec3(sz.x * outputSize.Width, sz.y * outputSize.Height, .0f);
		tq.second.pos.x = (tx + 0.5f) * outputSize.Width - tq.second.size.x * 0.5f;
		tq.second.pos.y = (0.5f - ty) * outputSize.Height - tq.second.size.y * 0.5f;
		id++;
	}
	m_screen_x = outputSize.Width; m_screen_y = outputSize.Height;
}
void overUIBoard::CreateBackgroundBoard(glm::vec3 pos, glm::vec3 scale) {
	m_background_board = std::make_unique<TextQuad>();
	m_background_board->pos = pos;
	m_background_board->size = scale;
	m_background_board->quad = new quadRenderer(m_deviceResources->GetD3DDevice(), DirectX::XMFLOAT4({ 0.6f, 0.7f, 0.95f, 0.8f }));
	m_background_board->mat = DirectX::XMMatrixScaling(scale.x, scale.y, scale.z)
		//* mat42xmmatrix(r)
		* DirectX::XMMatrixTranslation(pos.x, pos.y, pos.z);
}
void overUIBoard::AddBoard(std::string name, std::wstring unsel_tex, std::wstring sel_tex, bool default_state){
	int id = m_tquads.size();
	auto op = m_background_board->pos;
	auto osz = m_background_board->size;
	auto sz = glm::vec3(osz.x * 0.4f, osz.y * 0.2f, osz.z);
	AddBoard(name,
		glm::vec3(id%2?op.x+ 0.5f * sz.x+0.02*osz.x :op.x-0.5f*sz.x- 0.02 * osz.x,
			op.y + m_background_board->size.y*0.5f - sz.y * (int(id/2)+1), 
			op.z+0.01f),
		sz,
		glm::mat4(1.0f),
		default_state ? D2D1::ColorF::Chocolate : D2D1::ColorF::SlateBlue,
		unsel_tex, sel_tex, default_state);
}

void overUIBoard::AddBoard(std::string name, glm::vec3 p, glm::vec3 s, glm::mat4 r, D2D1::ColorF color, std::wstring unsel_tex, std::wstring sel_tex, bool default_state){
	TextQuad tq;
	tq.unselected_text = unsel_tex.empty()?std::wstring(name.begin(), name.end()): unsel_tex;
	tq.selected_text = sel_tex.empty()? std::wstring(name.begin(), name.end()) : unsel_tex;

	auto outputSize = m_deviceResources->GetOutputSize();
	if (outputSize.Width == 0) {
		tq.pos = p; tq.size = s;
	}else {
		tq.size = glm::vec3(s.x * outputSize.Width, s.y * outputSize.Height, .0f);
		tq.pos.x = (p.x + 0.5) * outputSize.Width - tq.size.x * 0.5f;
		tq.pos.y = (0.5 - p.y) * outputSize.Height - tq.size.y * 0.5f;
	}
	tq.selected = default_state;

	TextTextureInfo textInfo{ 256, 128 }; // pixels
	textInfo.Margin = 5; // pixels
	textInfo.TextAlignment = DWRITE_TEXT_ALIGNMENT_LEADING;
	textInfo.ParagraphAlignment = DWRITE_PARAGRAPH_ALIGNMENT_NEAR;
	tq.ttex = new TextTexture(m_deviceResources, textInfo);
	tq.ttex->setBackgroundColor(color);
	tq.dir = glm::vec3(glm::vec4(.0, .0, 1.0, .0) * r);
	tq.quad = new quadRenderer(m_deviceResources->GetD3DDevice());
	tq.mat = DirectX::XMMatrixScaling(s.x, s.y, s.z)
		* mat42xmmatrix(r)
		* DirectX::XMMatrixTranslation(p.x, p.y, p.z);
	tq.quad->setTexture(tq.ttex);
	tq.last_action_time = -1;
	m_tquads[name] = tq;
}
bool overUIBoard::CheckHit(std::string name, float px, float py) {
	glm::vec3 m_offset = m_tquads[name].pos;
	glm::vec3 m_size = m_tquads[name].size;

	return (px >= m_offset.x
		&& px < m_offset.x + m_size.x
		&& py >= m_offset.y
		&& py < m_offset.y + m_size.y); 
}
bool overUIBoard::CheckHit(const uint64_t frameIndex, std::string& name, float px, float py) {
	for (auto& tq : m_tquads) {
		glm::vec3 m_offset = tq.second.pos;
		glm::vec3 m_size = tq.second.size;
		if (px >= m_offset.x
			&& px < m_offset.x + m_size.x
			&& py >= m_offset.y
			&& py < m_offset.y + m_size.y) {
			if (on_board_hit(tq.second, frameIndex)) {
				name = tq.first; return true;
			}
			else { return false; }
		}
	}
	return false;
}
//todo: deprecate
bool overUIBoard::CheckHit(std::string name, float x, float y, float z) {
	auto pp = m_tquads[name].pos;
	auto sz = m_tquads[name].size;
	auto pn = m_tquads[name].dir;

	auto mid_point = pp + sz * 0.5f;
	//auto dst = glm::dot((glm::vec3(x, y, z) - mid_point), pn);

	auto dst = glm::length(glm::vec3(x, y, z) - mid_point);
	return dst < fmin(sz.x, sz.y);
}

bool overUIBoard::CheckHit(const uint64_t frameIndex, std::string& name, glm::vec3 pos, float radius) {
	glm::vec3 sc(pos.x, pos.y, pos.z);
	if (!sphere_intersects_plane_point(sc, radius, m_background_board->pos, glm::vec3(.0, .0, 1.0), m_background_board->size.x * 0.5f, m_background_board->size.y * 0.5f))
		return false;
	for (auto &tq : m_tquads) {
		if (sphere_intersects_plane_point(sc, radius, tq.second.pos, glm::vec3(.0, .0, 1.0), tq.second.size.x * 0.5f, tq.second.size.y * 0.5f)) {
			if (on_board_hit(tq.second, frameIndex)) {
				name = tq.first; return true;
			} else { return false; }
		}
	}
	return false;
}

bool overUIBoard::on_board_hit(TextQuad& texquad, const uint64_t frameIndex) {
	if (texquad.last_action_time == -1 || frameIndex - texquad.last_action_time > m_action_threshold) {
		texquad.last_action_time = frameIndex;
		texquad.selected = !texquad.selected;
		texquad.ttex->setBackgroundColor(texquad.selected ? D2D1::ColorF::Chocolate : D2D1::ColorF::SlateBlue);
		return true;
	}
	return false;
}

void overUIBoard::Update(std::string name, D2D1::ColorF color) {
	if (m_tquads.find(name) != m_tquads.end()) {
		m_tquads[name].ttex->setBackgroundColor(color);
	}
}
void overUIBoard::Update(std::string name, std::wstring new_content) {
	if (m_tquads.find(name) != m_tquads.end()) {
		m_tquads[name].unselected_text = new_content;
		m_tquads[name].selected_text = new_content;

	}
}
void overUIBoard::Render(){
	if (m_background_board) {
		m_background_board->quad->Draw(m_deviceResources->GetD3DDeviceContext(), m_background_board->mat);
	}
	for (auto tq : m_tquads) {
		tq.second.ttex->Draw(tq.second.selected?tq.second.selected_text.c_str():tq.second.unselected_text.c_str());
		tq.second.quad->Draw(m_deviceResources->GetD3DDeviceContext(), tq.second.mat);
	}
}