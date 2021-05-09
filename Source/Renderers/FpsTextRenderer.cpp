#include "pch.h"
#include "FpsTextRenderer.h"
#include <Common/DirectXHelper.h>

using namespace Microsoft::WRL;

// Initializes D2D resources used for text rendering.
FpsTextRenderer::FpsTextRenderer(const std::shared_ptr<DX::DeviceResources>& deviceResources)
:m_deviceResources(deviceResources){
	TextTextureInfo textInfo{ 256, 128 }; // pixels
	textInfo.Margin = 5; // pixels
	textInfo.TextAlignment = DWRITE_TEXT_ALIGNMENT_LEADING;
	textInfo.ParagraphAlignment = DWRITE_PARAGRAPH_ALIGNMENT_NEAR;
	textInfo.Background = D2D1::ColorF::Chocolate;
	m_text_texture = new TextTexture(deviceResources, textInfo);

	m_tex_quad = new quadRenderer(deviceResources->GetD3DDevice());
	m_tex_quad->setTexture(m_text_texture);
}

// Updates the text to be displayed.
void FpsTextRenderer::Update(DX::StepTimer const& timer)
{
	// Update display text.
	uint32 fps = timer.GetFramesPerSecond();

	m_text = (fps > 0) ? std::to_wstring(fps) + L" FPS" : L" - FPS";
}

// Renders a frame to the screen.
void FpsTextRenderer::Render()
{
	DirectX::XMFLOAT3 y_axis = { .0f, 1.0f, .0f };
	DirectX::XMVECTOR vy = XMLoadFloat3(&y_axis);

	m_text_texture->Draw(m_text.c_str());

	//m_deviceResources->GetD3DDeviceContext()->ClearDepthStencilView(m_deviceResources->GetDepthStencilView(), D3D11_CLEAR_DEPTH, 1.0f, 0);

	m_tex_quad->Draw(m_deviceResources->GetD3DDeviceContext(),
		DirectX::XMMatrixScaling(0.3, 0.2, 0.2)
		* DirectX::XMMatrixRotationAxis(vy, .2f)
		* DirectX::XMMatrixTranslation(0.8f, -0.8f, .0f) 
	);
}

void FpsTextRenderer::CreateDeviceDependentResources()
{
	//DX::ThrowIfFailed(
	//	m_deviceResources->GetD2DDeviceContext()->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &m_whiteBrush)
	//	);
}
void FpsTextRenderer::ReleaseDeviceDependentResources()
{
	//m_whiteBrush.Reset();
}