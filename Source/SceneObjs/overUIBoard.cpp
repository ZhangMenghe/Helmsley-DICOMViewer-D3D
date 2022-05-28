#include "pch.h"
#include "overUIBoard.h"
#include <Common/DirectXHelper.h>
#include <Utils/TypeConvertUtils.h>
#include <D3DPipeline/Primitive.h>
#include "strsafe.h"
#include <Common/Manager.h>
#include <Common/uiTestHelper.h>

overUIBoard::overUIBoard(const std::shared_ptr<DX::DeviceResources>& deviceResources)
:m_deviceResources(deviceResources){
	auto outputSize = m_deviceResources->GetOutputSize();
	m_screen_x = outputSize.Width; m_screen_y = outputSize.Height;
}
void overUIBoard::onWindowSizeChanged(float Width, float Height) {
	//if (!m_background_board || (m_screen_x == Width && m_screen_y == Height)) return;
	
	//auto op = m_background_board->pos;
	//auto osz = m_background_board->size;
	////auto sz = glm::vec3(osz.x * 0.4f, osz.y * 0.2f, osz.z);

	////int id = 0;
	//for (auto& tq : m_tquads) {
	////	auto tx = id % 2 ? op.x + 0.5f * sz.x + 0.02 * osz.x : op.x - 0.5f * sz.x - 0.02 * osz.x;
	////	auto ty = op.y + m_background_board->size.y * 0.5f - sz.y * (int(id / 2) + 1);

	////	tq.second.size = glm::vec3(sz.x * outputSize.Width, sz.y * outputSize.Height, .0f);
	////	tq.second.pos.x = (tx + 0.5f) * outputSize.Width - tq.second.size.x * 0.5f;
	////	tq.second.pos.y = (0.5f - ty) * outputSize.Height - tq.second.size.y * 0.5f;
	////	id++;
	//}
	m_screen_x = Width; m_screen_y = Height;
}
void overUIBoard::CreateBackgroundBoard(glm::vec3 pos, glm::vec3 scale, bool drawable) {
	m_background_board = std::make_unique<TextQuad>();
	m_background_board->pos = pos;
	m_background_board->size = scale;
	m_background_board->mat = DirectX::XMMatrixScaling(scale.x, scale.y, scale.z)
		//* mat42xmmatrix(r)
		* DirectX::XMMatrixTranslation(pos.x, pos.y, pos.z);

	if (drawable) {
		m_background_board->dtex = new paintCanvas(
			m_deviceResources->GetD3DDevice(),
			m_deviceResources->GetD3DDeviceContext(), scale.y * m_default_tex_height, scale.x * m_default_tex_width, glm::vec4(0.6f, 0.7f, 0.95f, 1.0f));
		m_background_board->quad = new quadRenderer(m_deviceResources->GetD3DDevice());

		m_background_board->quad->setTexture(m_background_board->dtex->getCanvasTexture());
	}
	else {
		m_background_board->quad = new quadRenderer(m_deviceResources->GetD3DDevice(), DirectX::XMFLOAT4({ 0.6f, 0.7f, 0.95f, 0.8f }));
	}
}
void overUIBoard::AddBoard(std::string name, std::wstring unsel_tex, std::wstring sel_tex, bool default_state){
	int id = m_tquads.size();
	auto op = m_background_board->pos;
	auto osz = m_background_board->size;
	auto sz = glm::vec3(osz.x * 0.4f, osz.y * 0.2f, osz.z);
	AddBoard(name,
		glm::vec3(id%2?op.x+ 0.5f * sz.x+0.02*osz.x :op.x-0.5f*sz.x- 0.02 * osz.x,
			op.y + osz.y*0.5f - sz.y * (int(id/2)+1),
			op.z+0.01f),
		sz,
		glm::mat4(1.0f),
		default_state ? D2D1::ColorF::Chocolate : D2D1::ColorF::SlateBlue,
		unsel_tex, sel_tex, default_state);
}
void overUIBoard::AddBoard(std::string name, int rows, int cols, int id, std::wstring unsel_tex, std::wstring sel_tex, bool default_state) {
	auto op = m_background_board->pos;
	auto osz = m_background_board->size;
	auto sz = glm::vec3(osz.x /cols * 0.8f, osz.y, osz.z);
	AddBoard(name,
		glm::vec3(
			op.x-osz.x*0.5f+(id-0.5f)*sz.x*1.3f,
			op.y,
			op.z + 0.01f),
		sz,
		glm::mat4(1.0f),
		default_state ? D2D1::ColorF::Chocolate : D2D1::ColorF::SlateBlue,
		unsel_tex, sel_tex, default_state);
}
void overUIBoard::AddBoard(std::string name, glm::vec3 p, glm::vec3 s, glm::mat4 r, D2D1::ColorF color, std::wstring unsel_tex, std::wstring sel_tex, bool default_state){
	TextQuad tq;
	tq.unselected_text = unsel_tex.empty()?std::wstring(name.begin(), name.end()): unsel_tex;
	tq.selected_text = sel_tex.empty()? std::wstring(name.begin(), name.end()) : unsel_tex;
	tq.selected = default_state;
	
	tq.pos = p; tq.size = s;
	tq.mat = DirectX::XMMatrixScaling(s.x, s.y, s.z)
		* mat42xmmatrix(r)
		* DirectX::XMMatrixTranslation(p.x, p.y, p.z);

	TextTextureInfo textInfo{ 256, 128 }; // pixels

	textInfo.Margin = 5; // pixels
	textInfo.TextAlignment = DWRITE_TEXT_ALIGNMENT_LEADING;
	textInfo.ParagraphAlignment = DWRITE_PARAGRAPH_ALIGNMENT_NEAR;
	textInfo.FontSize = 48;
	tq.ttex = new TextTexture(m_deviceResources, textInfo);
	tq.ttex->setBackgroundColor(color);
	tq.dir = glm::vec3(glm::vec4(.0, .0, 1.0, .0) * r);
	tq.quad = new quadRenderer(m_deviceResources->GetD3DDevice());
	tq.quad->setTexture(tq.ttex);
	tq.last_action_time = -1;
	m_tquads[name] = tq;
}

bool overUIBoard::CheckHit(std::string name, float px, float py) {
	glm::vec3 proj_pos, proj_size;

	if (name == "background") {
		bool hit = CheckHitRespToModelMtx(Manager::camera->getVPMat(), m_background_board->mat, 
			m_screen_x, m_screen_y, px, py, proj_pos, proj_size);
		if (hit) {
			m_background_board->selected = !m_background_board->selected;
			//if(m_background_board->selected) 
				m_background_board->dtex->setBrushPos(m_deviceResources->GetD3DDeviceContext(), proj_pos, proj_size, px, py);
		}
		return hit;
	}

	return CheckHitRespToModelMtx(Manager::camera->getVPMat(), m_tquads[name].mat, m_screen_x, m_screen_y, px, py, proj_pos, proj_size);
	//glm::vec3 m_offset = m_tquads[name].pos;
	//glm::vec3 m_size = m_tquads[name].size;
	//return point2d_inside_rectangle(m_offset.x, m_offset.y, m_size.x, m_size.y, px, py);
}
bool overUIBoard::CheckHit(const uint64_t frameIndex, std::string& name, float px, float py) {
	if (m_background_board) {
		glm::vec3 proj_pos, proj_size;

		if(!CheckHitRespToModelMtx(Manager::camera->getVPMat(), m_background_board->mat, m_screen_x, m_screen_y, px, py, proj_pos, proj_size))return false;
		//glm::vec3 m_offset = m_background_board->pos;
		//glm::vec3 m_size = m_background_board->size;
		//if (!point2d_inside_rectangle(m_offset.x, m_offset.y, m_size.x, m_size.y, px, py)) return false;
	}
	for (auto& tq : m_tquads) {
		//glm::vec3 m_offset = tq.second.pos;
		//glm::vec3 m_size = tq.second.size;
		if(CheckHit(tq.first, px, py)){
		//if (point2d_inside_rectangle(m_offset.x, m_offset.y, m_size.x, m_size.y, px, py)) {
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
	if (!CheckHitWithinSphere(pos, radius, m_background_board->pos, m_background_board->size.x * 0.5f, m_background_board->size.y * 0.5f))
		return false;
	//check if there are sub hitable objects
	for (auto &tq : m_tquads) {
		if (CheckHitWithinSphere(pos, radius, tq.second.pos, tq.second.size.x * 0.5f, tq.second.size.y * 0.5f)) {
			if (on_board_hit(tq.second, frameIndex)) {
				name = tq.first; return true;
			} else { return false; }
		}
	}
	return false;
}
bool overUIBoard::DrawOnBoard(const uint64_t frameIndex, std::string& name, glm::vec3 pos, float radius) {
	if (!m_background_board || !m_background_board->dtex) return false;
	if (!m_3d_pressed) {
		if (!CheckHitWithinSphere(pos, radius, m_background_board->pos, m_background_board->size.x * 0.5f, m_background_board->size.y * 0.5f))
			return false;
	}

	glm::mat4 plane_model = xmmatrix2mat4(m_background_board->mat);
	glm::vec4 pn_ = plane_model * glm::vec4(0, 0, -1.0f, 1.0f);
	glm::vec3 pn = glm::vec3(pn_.x, pn_.y, pn_.z) / pn_.w;
	float proj_dist; glm::vec3 proj_pos_world;
	PointProjectOnPlane(m_background_board->pos, pn, pos, proj_dist, proj_pos_world);
	//LOGINFO("====DIST: %f\n", proj_dist);
	if (proj_dist < -0.02f) {
		m_3d_pressed = false; return false;
	}
	if (dvr::SET_DEPTH_FORCE_ON && proj_dist > .0f) m_background_board->dtex->setDepthForceBrushSize(proj_dist);

	glm::vec4 proj_pos_canvas = glm::inverse(plane_model) * glm::vec4(proj_pos_world, 1.0f);
	glm::vec3 proj_pos = glm::vec3(proj_pos_canvas.x, proj_pos_canvas.y, proj_pos_canvas.z) / proj_pos_canvas.w;

	glm::vec4 back_proj_pos_plane(proj_pos.x, proj_pos.y, .0f, 1.0f);
	glm::vec4 back_proj_pos_world_ = plane_model * back_proj_pos_plane;
	glm::vec3 back_proj_pos_world = glm::vec3(back_proj_pos_world_.x, back_proj_pos_world_.y, back_proj_pos_world_.z) / back_proj_pos_world_.w;
	m_background_board->dtex->setBrushMeshPos(back_proj_pos_world);


	if (!m_3d_pressed) {
		m_background_board->dtex->setBrushPos(m_deviceResources->GetD3DDeviceContext(), proj_pos.x + 0.5f, 0.5f - proj_pos.y);
		m_3d_pressed = true;
	}else {
		m_background_board->dtex->onBrushDraw(m_deviceResources->GetD3DDeviceContext(), proj_pos.x + 0.5f, 0.5f - proj_pos.y);
	}
	return true;
}
void overUIBoard::FilterBoardSelection(std::string name) {
	for (auto& tq : m_tquads) {
		if (tq.first != name) {
			tq.second.selected = false;
			tq.second.ttex->setBackgroundColor(D2D1::ColorF::SlateBlue);
		}
	}
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
//void overUIBoard::update_board_projection_pos(DirectX::XMMATRIX& proj_mat, glm::vec3& size, glm::vec3& pos) {
//	DirectX::XMFLOAT4X4 mmat_f;
//	DirectX::XMStoreFloat4x4(&mmat_f, proj_mat);
//
//	size = glm::vec3(mmat_f.m[0][0] * m_screen_x, mmat_f.m[1][1] * m_screen_y, .0f);
//
//	auto ndc_y = std::clamp(mmat_f.m[1][3], -mmat_f.m[3][3], mmat_f.m[3][3]) / mmat_f.m[3][3];
//	pos.y = (1.0f - 0.5f * (ndc_y + 1.0f)) * m_screen_y - size.y * 0.5f;
//
//	auto ndc_x = std::clamp(mmat_f.m[0][3], -mmat_f.m[3][3], mmat_f.m[3][3]) / mmat_f.m[3][3];
//	pos.x = (ndc_x + 1.0f) * 0.5f * m_screen_x - size.x * 0.5f;
//}
void overUIBoard::Render(){
	if (m_background_board) {
		m_background_board->quad->Draw(m_deviceResources->GetD3DDeviceContext(), m_background_board->mat);
		if (m_3d_pressed &&m_background_board->dtex) m_background_board->dtex->Draw(m_deviceResources->GetD3DDeviceContext());
	}
	//auto outputSize = m_deviceResources->GetOutputSize();
	//if(outputSize.Width!=0) {
	//	for (auto& tq : m_tquads) {
	//		auto projMat = DirectX::XMMatrixMultiply(Manager::camera->getVPMat(),
	//			DirectX::XMMatrixTranspose(tq.second.mat));
	//		update_board_projection_pos(projMat, tq.second.size, tq.second.pos);
	//	}
	//	if (m_background_board) {
	//		auto projMat = DirectX::XMMatrixMultiply(Manager::camera->getVPMat(),
	//			DirectX::XMMatrixTranspose(m_background_board->mat));
	//		update_board_projection_pos(projMat, m_background_board->size, m_background_board->pos);
	//	}
	//}
	for (auto tq : m_tquads) {
		tq.second.ttex->Draw(tq.second.selected?tq.second.selected_text.c_str():tq.second.unselected_text.c_str());
		tq.second.quad->Draw(m_deviceResources->GetD3DDeviceContext(), tq.second.mat);
	}
}