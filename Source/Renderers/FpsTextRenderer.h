#ifndef FPS_TEXT_RENDERER_H
#define FPS_TEXT_RENDERER_H
#include "pch.h"
#include <Common/DeviceResources.h>
#include <Common/StepTimer.h>
#include <Renderers/quadRenderer.h>
#include <Common/TextTexture.h>

// Renders the current FPS value in the bottom right corner of the screen using Direct2D and DirectWrite.
class FpsTextRenderer
{
public:
	FpsTextRenderer(const std::shared_ptr<DX::DeviceResources>& deviceResources);
	void CreateDeviceDependentResources();
	void ReleaseDeviceDependentResources();
	void Update(DX::StepTimer const& timer);
	void Render();

private:
	std::wstring                                    m_text;

	quadRenderer* m_tex_quad;
	TextTexture* m_text_texture;

	//// Cached pointer to device resources.
	std::shared_ptr<DX::DeviceResources> m_deviceResources;

	//// Resources related to text rendering.
	//DWRITE_TEXT_METRICS	                            m_textMetrics;
	//Microsoft::WRL::ComPtr<ID2D1SolidColorBrush>    m_whiteBrush;
	//Microsoft::WRL::ComPtr<ID2D1DrawingStateBlock1> m_stateBlock;
	//Microsoft::WRL::ComPtr<IDWriteTextLayout3>      m_textLayout;
	//Microsoft::WRL::ComPtr<IDWriteTextFormat2>      m_textFormat;
};
#endif // !FPS_TEXT_RENDERER_H
