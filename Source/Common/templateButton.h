#ifndef TEMPLATE_BUTTON_H
#define TEMPLATE_BUTTON_H

#include <Common/DeviceResources.h>
#include <Renderers/quadRenderer.h>
#include <D3DPipeline/Texture.h>
#include <opencv2/core.hpp>


//struct TextTextureInfo {
//    TextTextureInfo(uint32_t width, uint32_t height)
//        : Width(width)
//        , Height(height) {
//    }
//
//    uint32_t Width;
//    uint32_t Height;
//    const wchar_t* FontName = L"Segoe UI";
//    float FontSize = 36;
//    float Margin = 0;
//    D2D1::ColorF Foreground = D2D1::ColorF::White;
//    D2D1::ColorF Background = D2D1::ColorF::Black;
//    DWRITE_TEXT_ALIGNMENT TextAlignment = DWRITE_TEXT_ALIGNMENT_CENTER;
//    DWRITE_PARAGRAPH_ALIGNMENT ParagraphAlignment = DWRITE_PARAGRAPH_ALIGNMENT_CENTER;
//};

// Manages a texture which can be drawn to.
class templateButton {
public:
    templateButton(const std::shared_ptr<DX::DeviceResources>& deviceResources, std::string bg_file_name, std::string template_file_name, glm::vec3 p, glm::vec3 s, glm::mat4 r);
    void Render();

    ////setter
    //void setBackgroundColor(D2D1::ColorF color) {
    //    m_textInfo.Background = color;
    //}
private:
    std::shared_ptr<DX::DeviceResources> m_deviceResources;

    std::unique_ptr<Texture> m_button_texture;
    std::unique_ptr<quadRenderer> m_button_quad;
    DirectX::XMMATRIX m_button_mat;

    cv::Mat m_template;

    //TextTextureInfo m_textInfo;
    //winrt::com_ptr<ID2D1Bitmap1> m_d2dTargetBitmap;
    ////ID2D1Bitmap1* m_d2dTargetBitmap;
    //winrt::com_ptr<ID2D1SolidColorBrush> m_brush;
    //winrt::com_ptr<ID2D1DrawingStateBlock> m_stateBlock;
    //winrt::com_ptr<IDWriteTextFormat> m_textFormat;
    ////winrt::com_ptr<ID3D11Texture2D> m_textDWriteTexture;
};

#endif // !TEXT_TEXTURE_H