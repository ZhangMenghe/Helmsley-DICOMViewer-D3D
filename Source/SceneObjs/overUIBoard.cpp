#include "pch.h"
#include "overUIBoard.h"
#include <Common/DirectXHelper.h>
#include <Utils/TypeConvertUtils.h>
#include <D3DPipeline/Primitive.h>
#include "strsafe.h"

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
void overUIBoard::AddBoard(std::string name, glm::vec3 p, glm::vec3 s, glm::mat4 r){
	TextQuad tq;
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
	tq.dir = glm::vec3(glm::vec4(.0, .0, 1.0, .0) * r);
	tq.quad = new quadRenderer(m_deviceResources->GetD3DDevice());
	tq.mat = DirectX::XMMatrixScaling(s.x, s.y, s.z)
		* mat42xmmatrix(r)
		* DirectX::XMMatrixTranslation(p.x, p.y, p.z);
	tq.quad->setTexture(tq.ttex);
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
bool overUIBoard::CheckHit(std::string name, float x, float y, float z) {
	auto pp = m_tquads[name].pos;
	auto sz = m_tquads[name].size;
	auto pn = m_tquads[name].dir;

	auto mid_point = pp + sz * 0.5f;
	//auto dst = glm::dot((glm::vec3(x, y, z) - mid_point), pn);

	auto dst = glm::length(glm::vec3(x, y, z) - mid_point);
	return dst < fmin(sz.x, sz.y);
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
	for (auto tq : m_tquads) {
		tq.second.ttex->Draw(tq.second.content.c_str());
		tq.second.quad->Draw(m_deviceResources->GetD3DDeviceContext(), tq.second.mat);
	}
}