#ifndef OVER_UI_BOARD_H
#define OVER_UI_BOARD_H

#include <Common/DeviceResources.h>
#include <Common/StepTimer.h>
#include <Renderers/quadRenderer.h>
#include <SceneObjs/paintCanvas.h>

#include <Common/TextTexture.h>
#include <unordered_map>
struct TextQuad {
	std::wstring unselected_text;
	std::wstring selected_text;

	quadRenderer* quad;
	TextTexture* ttex=nullptr;
	paintCanvas* dtex=nullptr;

	glm::vec3 pos;
	glm::vec3 size;
	glm::vec3 dir;
	bool selected;
	DirectX::XMMATRIX mat;
	int64_t last_action_time;
};
// Renders the current FPS value in the bottom right corner of the screen using Direct2D and DirectWrite.
class overUIBoard{
public:
	overUIBoard(const std::shared_ptr<DX::DeviceResources>& deviceResources);

	void CreateBackgroundBoard(glm::vec3 pos, glm::vec3 scale, bool drawble = false);

	void AddBoard(std::string name, std::wstring unsel_tex = L"", std::wstring sel_tex = L"", bool default_state = false);
	void AddBoard(std::string name, int rows, int cols, int id, std::wstring unsel_tex = L"", std::wstring sel_tex = L"", bool default_state = false);
	void AddBoard(std::string name, glm::vec3 pos, glm::vec3 scale, glm::mat4 rot, D2D1::ColorF color, std::wstring unsel_tex = L"", std::wstring sel_tex = L"", bool default_state = false);
	
	bool CheckHit(std::string name, float x, float y);
	bool CheckHit(std::string name, float x, float y, float z);
	bool CheckHit(const uint64_t frameIndex, std::string& name, glm::vec3 pos, float radius);
	bool CheckHit(const uint64_t frameIndex, std::string& name, float x, float y);

	bool IsSelected(std::string name) { 
		if (name == "background") return m_background_board->selected;
		return m_tquads[name].selected;
	}
	void onTouchMove(float px, float py) {
		if (m_background_board->dtex) {
			m_background_board->dtex->onBrushDraw(m_deviceResources->GetD3DDeviceContext(), px, py);
		}
	}
	void onTouchMove(float px, float py, int delta) {
		if (m_background_board->dtex) {
			m_background_board->dtex->onBrushDrawWithDepthForce(m_deviceResources->GetD3DDeviceContext(), px, py, delta);
			//depth force
			//m_background_board->dtex->onBrushDraw(m_deviceResources->GetD3DDeviceContext(), px, py);
		}
	}
	void onTouchReleased() {
		if (m_background_board->dtex) m_background_board->dtex->onBrushUp();
	}
	//void FilpBoardSelection(std::string name);

	void FilterBoardSelection(std::string name);
	void Update(std::string name, std::wstring new_content);
	void Update(std::string name, D2D1::ColorF color);
	void Render();

	void onWindowSizeChanged(float width, float height);

private:
	std::shared_ptr<DX::DeviceResources> m_deviceResources;
	std::unordered_map<std::string, TextQuad> m_tquads;
	std::unique_ptr<TextQuad> m_background_board = nullptr;
	paintCanvas* m_2dcanvas;

	float m_screen_x, m_screen_y;
	const int64_t m_action_threshold = 20;
	const int32_t m_default_tex_height = 800, m_default_tex_width = 800;

	bool on_board_hit(TextQuad& texquad, const uint64_t frameIndex);
	//void update_board_projection_pos(DirectX::XMMATRIX& proj_mat, glm::vec3& size, glm::vec3& pos);
};
#endif
