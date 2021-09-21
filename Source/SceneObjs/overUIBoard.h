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
	DirectX::XMMATRIX mat;
};
// Renders the current FPS value in the bottom right corner of the screen using Direct2D and DirectWrite.
class overUIBoard{
public:
	overUIBoard(const std::shared_ptr<DX::DeviceResources>& deviceResources);

	void CreateWindowSizeDependentResources(float width, float height);

	void AddBoard(std::string name, glm::vec3 pos, glm::vec3 scale, glm::mat4 rot);
	bool CheckHit(std::string name, float x, float y);
	bool CheckHit(std::string name, float x, float y, float z);

	void Update(std::string name, std::wstring new_content);
	void Render();

private:
	std::shared_ptr<DX::DeviceResources> m_deviceResources;
	std::unordered_map<std::string, TextQuad> m_tquads;

	float m_screen_x, m_screen_y;
};
#endif
