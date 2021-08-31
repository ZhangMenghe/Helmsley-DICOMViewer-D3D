#include "pch.h"
#include "overUIBoard.h"
#include <Common/DirectXHelper.h>
#include <Utils/TypeConvertUtils.h>
#include <D3DPipeline/Primitive.h>
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
		tq.pos = p; tq.size = glm::vec3(s.x * outputSize.Width, s.y * outputSize.Height, s.z);
		getAbsoluteValue(tq.pos.x, tq.pos.y, outputSize.Width, outputSize.Height);
		tq.pos.x -= tq.size.x * 0.5f;
		tq.pos.y -= tq.size.y * 0.5f;
		

		TextTextureInfo textInfo{ 256, 128 }; // pixels
		textInfo.Margin = 5; // pixels
		textInfo.TextAlignment = DWRITE_TEXT_ALIGNMENT_LEADING;
		textInfo.ParagraphAlignment = DWRITE_PARAGRAPH_ALIGNMENT_NEAR;
		textInfo.Background = D2D1::ColorF::Chocolate;
		
		tq.ttex = new TextTexture(m_deviceResources, textInfo);

		if (r == glm::mat4(1.0)) {
			float* vertices = new float[24];
			memcpy(vertices, quad_vertices_pos_w_tex_full, 24 * sizeof(float));
			for (int i = 0; i < 4; i++) {
				vertices[6 * i] = quad_vertices_pos_w_tex_full[6 * i] * s.x + p.x;
				vertices[6 * i + 1] = quad_vertices_pos_w_tex_full[6 * i + 1] * s.y + p.y;
				vertices[6 * i + 2] = quad_vertices_pos_w_tex_full[6 * i + 2];
			}
			tq.quad = new quadRenderer(m_deviceResources->GetD3DDevice(), L"Naive2DTexVertexShader.cso", L"QuadPixelShader.cso", vertices);
			tq.mat = mat42xmmatrix(r);
		}
		else {
			tq.quad = new quadRenderer(m_deviceResources->GetD3DDevice());
			tq.mat = DirectX::XMMatrixScaling(s.x, s.y, s.z)
				* mat42xmmatrix(r)
				* DirectX::XMMatrixTranslation(p.x, p.y, p.z);
		}
		tq.quad->setTexture(tq.ttex);
		m_tquads[name] = tq;
}
bool overUIBoard::CheckHit(std::string name, float px, float py) {
	glm::vec3 m_offset = m_tquads[name].pos;
	glm::vec3 m_size = m_tquads[name].size;

	return (px >= m_offset.x &&
		px < m_offset.x + m_size.x &&
		py >= m_offset.y &&
		py < m_offset.y + m_size.y);
}

void overUIBoard::Update(std::string name, std::wstring new_content) {
	if (m_tquads.find(name) != m_tquads.end()) {
		m_tquads[name].content = new_content;
	}
}

void overUIBoard::Render(){
	for (auto tq : m_tquads) {
		tq.second.ttex->Draw(tq.second.content.c_str());
		tq.second.quad->Draw(m_deviceResources->GetD3DDeviceContext(), tq.second.mat);
	}
}