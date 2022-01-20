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
}
void getAbsoluteValue(float& x, float& y, float width, float height){
	x = (x * 0.5 + 0.5) * width;
	y = (0.5 - y * 0.5) * height;
}
void overUIBoard::CreateWindowSizeDependentResources(float width, float height) {
	for (auto tq : m_tquads) {
		tq.second.size.x *= width;
		tq.second.size.y *= height;
		getAbsoluteValue(tq.second.pos.x, tq.second.pos.y, width, height);
		tq.second.pos.x -= tq.second.size.x * 0.5f;
		tq.second.pos.y -= tq.second.size.y * 0.5f;
	}
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

void overUIBoard::AddBoard(std::string name){
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
		D2D1::ColorF::DarkSlateBlue);
}

void overUIBoard::AddBoard(std::string name, glm::vec3 p, glm::vec3 s, glm::mat4 r, D2D1::ColorF color){
	TextQuad tq;
	tq.content = std::wstring(name.begin(), name.end());

	auto outputSize = m_deviceResources->GetOutputSize();
	if (outputSize.Width == 0) {
		tq.pos = p; tq.size = s;
	}else {
		tq.pos = p; tq.size = glm::vec3(s.x * outputSize.Width, s.y * outputSize.Height, .0f);
		getAbsoluteValue(tq.pos.x, tq.pos.y, outputSize.Width, outputSize.Height);
		tq.pos.x -= tq.size.x * 0.5f;
		tq.pos.y -= tq.size.y * 0.5f;
	}
		
	TextTextureInfo textInfo{ 256, 128 }; // pixels
	textInfo.Margin = 5; // pixels
	textInfo.TextAlignment = DWRITE_TEXT_ALIGNMENT_LEADING;
	textInfo.ParagraphAlignment = DWRITE_PARAGRAPH_ALIGNMENT_NEAR;
	textInfo.Background = D2D1::ColorF::Chocolate;
	tq.ttex = new TextTexture(m_deviceResources, textInfo);
	tq.ttex->setBackgroundColor(color);
	tq.dir = glm::vec3(glm::vec4(.0, .0, 1.0, .0) * r);
	tq.quad = new quadRenderer(m_deviceResources->GetD3DDevice());
	tq.mat = DirectX::XMMatrixScaling(s.x, s.y, s.z)
		* mat42xmmatrix(r)
		* DirectX::XMMatrixTranslation(p.x, p.y, p.z);
	tq.quad->setTexture(tq.ttex);
	tq.selected = false;
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

bool overUIBoard::CheckHit(const uint64_t frameIndex, std::string& name, XrVector3f pos, float radius) {
	glm::vec3 sc(pos.x, pos.y, pos.z);
	if (!sphere_intersects_plane_point(sc, radius, m_background_board->pos, glm::vec3(.0, .0, 1.0), m_background_board->size.x * 0.5f, m_background_board->size.y * 0.5f))
		return false;
	for (auto &tq : m_tquads) {
		if (sphere_intersects_plane_point(sc, radius, tq.second.pos, glm::vec3(.0, .0, 1.0), tq.second.size.x * 0.5f, tq.second.size.y * 0.5f)) {
			if (tq.second.last_action_time == -1 || frameIndex - tq.second.last_action_time > m_action_threshold) {
				tq.second.last_action_time = frameIndex;
				name = tq.first;
				tq.second.ttex->setBackgroundColor(tq.second.selected?D2D1::ColorF::Chocolate: D2D1::ColorF::DarkBlue);
				tq.second.selected = !tq.second.selected;
				return true;
			}
		}
	}
	return false;
}
void overUIBoard::Update(std::string name, std::wstring new_content) {
	if (m_tquads.find(name) != m_tquads.end()) {
		m_tquads[name].content = new_content;
	}
}
void overUIBoard::Update(std::string name, D2D1::ColorF color) {
	if (m_tquads.find(name) != m_tquads.end()) {
		m_tquads[name].ttex->setBackgroundColor(color);
	}
}

void overUIBoard::Render(){
	if (m_background_board) {
		m_background_board->quad->Draw(m_deviceResources->GetD3DDeviceContext(), m_background_board->mat);
	}
	for (auto tq : m_tquads) {
		tq.second.ttex->Draw(tq.second.content.c_str());
		tq.second.quad->Draw(m_deviceResources->GetD3DDeviceContext(), tq.second.mat);
	}
}