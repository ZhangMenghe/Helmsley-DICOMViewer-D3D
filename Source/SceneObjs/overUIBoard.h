#ifndef OVER_UI_BOARD_H
#define OVER_UI_BOARD_H

#include <Common/DeviceResources.h>
#include <Common/StepTimer.h>
#include <Renderers/quadRenderer.h>
#include <Common/TextTexture.h>
#include <unordered_map>
struct TextQuad {
	std::wstring unselected_text;
	std::wstring selected_text;

	quadRenderer* quad;
	TextTexture* ttex;
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

	void CreateBackgroundBoard(glm::vec3 pos, glm::vec3 scale);

	void AddBoard(std::string name, std::wstring unsel_tex = L"", std::wstring sel_tex = L"", bool default_state = false);
	void AddBoard(std::string name, glm::vec3 pos, glm::vec3 scale, glm::mat4 rot, D2D1::ColorF color, std::wstring unsel_tex = L"", std::wstring sel_tex = L"", bool default_state = false);
	bool CheckHit(std::string name, float x, float y);
	bool CheckHit(std::string name, float x, float y, float z);
	bool CheckHit(const uint64_t frameIndex, std::string& name, glm::vec3 pos, float radius);
	bool CheckHit(const uint64_t frameIndex, std::string& name, float x, float y);

	bool IsSelected(std::string name) { return m_tquads[name].selected; }
	void Update(std::string name, std::wstring new_content);
	void Update(std::string name, D2D1::ColorF color);
	void Render();

	void onWindowSizeChanged();

private:
	std::shared_ptr<DX::DeviceResources> m_deviceResources;
	std::unordered_map<std::string, TextQuad> m_tquads;
	std::unique_ptr<TextQuad> m_background_board = nullptr;

	float m_screen_x, m_screen_y;
	const int64_t m_action_threshold = 20;

	bool on_board_hit(TextQuad& texquad, const uint64_t frameIndex);
	void update_board_projection_pos(DirectX::XMMATRIX& proj_mat, glm::vec3& size, glm::vec3& pos);
};
#endif
