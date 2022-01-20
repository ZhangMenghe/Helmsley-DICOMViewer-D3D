#ifndef OVER_UI_BOARD_H
#define OVER_UI_BOARD_H

#include <Common/DeviceResources.h>
#include <Common/StepTimer.h>
#include <Renderers/quadRenderer.h>
#include <Common/TextTexture.h>
#include <unordered_map>
struct TextQuad {
	std::wstring content;
	quadRenderer* quad;
	TextTexture* ttex;
	glm::vec3 pos;
	glm::vec3 size;
	glm::vec3 dir;
	bool selected;
	DirectX::XMMATRIX mat;
	XrTime last_action_time;
};
// Renders the current FPS value in the bottom right corner of the screen using Direct2D and DirectWrite.
class overUIBoard{
public:
	overUIBoard(const std::shared_ptr<DX::DeviceResources>& deviceResources);

	void CreateWindowSizeDependentResources(float width, float height);
	void CreateBackgroundBoard(glm::vec3 pos, glm::vec3 scale);

	void AddBoard(std::string name);
	void AddBoard(std::string name, glm::vec3 pos, glm::vec3 scale, glm::mat4 rot, D2D1::ColorF color);
	bool CheckHit(std::string name, float x, float y);
	bool CheckHit(std::string name, float x, float y, float z);
	bool CheckHit(const uint64_t frameIndex, std::string& name, XrVector3f pos, float radius);

	void Update(std::string name, std::wstring new_content);
	void Update(std::string name, D2D1::ColorF color);
	void Render();

private:
	std::shared_ptr<DX::DeviceResources> m_deviceResources;
	std::unordered_map<std::string, TextQuad> m_tquads;
	std::unique_ptr<TextQuad> m_background_board = nullptr;

	float m_screen_x, m_screen_y;
	const XrTime m_action_threshold = 20;
};
#endif
