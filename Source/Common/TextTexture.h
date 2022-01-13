#ifndef TEXT_TEXTURE_H
#define TEXT_TEXTURE_H

#include <Common/DeviceResources.h>
#include <D3DPipeline/Texture.h>



struct TextTextureInfo {
    TextTextureInfo(uint32_t width, uint32_t height)
        : Width(width)
        , Height(height) {
    }

    uint32_t Width;
    uint32_t Height;
    const wchar_t* FontName = L"Segoe UI";
    float FontSize = 36;
    float Margin = 0;
    D2D1::ColorF Foreground = D2D1::ColorF::White;
    D2D1::ColorF Background = D2D1::ColorF::Black;
    DWRITE_TEXT_ALIGNMENT TextAlignment = DWRITE_TEXT_ALIGNMENT_CENTER;
    DWRITE_PARAGRAPH_ALIGNMENT ParagraphAlignment = DWRITE_PARAGRAPH_ALIGNMENT_CENTER;
};

// Manages a texture which can be drawn to.
class TextTexture:public Texture {
public:
    TextTexture(const std::shared_ptr<DX::DeviceResources>& deviceResources, TextTextureInfo textInfo);
    void Draw(const wchar_t* text);

    //setter
    void setBackgroundColor(D2D1::ColorF color) {
        m_textInfo.Background = color;
    }
private:
    std::shared_ptr<DX::DeviceResources> m_deviceResources;

    TextTextureInfo m_textInfo;
    winrt::com_ptr<ID2D1Bitmap1> m_d2dTargetBitmap;
    //ID2D1Bitmap1* m_d2dTargetBitmap;
    winrt::com_ptr<ID2D1SolidColorBrush> m_brush;
    winrt::com_ptr<ID2D1DrawingStateBlock> m_stateBlock;
    winrt::com_ptr<IDWriteTextFormat> m_textFormat;
    //winrt::com_ptr<ID3D11Texture2D> m_textDWriteTexture;
};

#endif // !TEXT_TEXTURE_H