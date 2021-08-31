#include "pch.h"
#include "TextTexture.h"
#include <Common/DirectXHelper.h>

namespace {
    constexpr DXGI_FORMAT TextFormat = DXGI_FORMAT_B8G8R8A8_UNORM;
}
TextTexture::TextTexture(const std::shared_ptr<DX::DeviceResources>& deviceResources, TextTextureInfo textInfo)
    : m_textInfo(std::move(textInfo)),
    m_deviceResources(deviceResources),
    Texture(){
    DX::ThrowIfFailed(m_deviceResources->GetDWriteFactory()->CreateTextFormat(m_textInfo.FontName,
        nullptr,
        DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        m_textInfo.FontSize,
        L"en-US",
        m_textFormat.put()));
    DX::ThrowIfFailed(m_textFormat->SetTextAlignment(m_textInfo.TextAlignment));
    DX::ThrowIfFailed(m_textFormat->SetParagraphAlignment(m_textInfo.ParagraphAlignment));

    DX::ThrowIfFailed(m_deviceResources->GetD2DFactory()->CreateDrawingStateBlock(m_stateBlock.put()));

    //
    // Set up 2D rendering modes.
    //
    const D2D1_BITMAP_PROPERTIES1 bitmapProperties = D2D1::BitmapProperties1(D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
        D2D1::PixelFormat(TextFormat, D2D1_ALPHA_MODE_PREMULTIPLIED));

    const auto texDesc = CD3D11_TEXTURE2D_DESC(TextFormat,
        m_textInfo.Width,
        m_textInfo.Height,
        1,
        1,
        D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE,
        D3D11_USAGE_DEFAULT,
        0,
        1,
        0,
        0);
    Initialize(m_deviceResources->GetD3DDevice(), texDesc);
    //DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateTexture2D(&texDesc, nullptr, m_textDWriteTexture.put()));

    winrt::com_ptr<IDXGISurface> dxgiPerfBuffer = mTex2D.as<IDXGISurface>();
    auto m_d2dContext = m_deviceResources->GetD2DDeviceContext();
    DX::ThrowIfFailed(m_d2dContext->CreateBitmapFromDxgiSurface(dxgiPerfBuffer.get(), &bitmapProperties, m_d2dTargetBitmap.put()));
    DX::ThrowIfFailed(m_d2dContext->CreateSolidColorBrush(textInfo.Foreground, m_brush.put()));
}

void TextTexture::Draw(const wchar_t* text) {
    auto m_d2dContext = m_deviceResources->GetD2DDeviceContext();

    m_d2dContext->SetTarget(m_d2dTargetBitmap.get());
    m_d2dContext->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);
    m_d2dContext->SetTransform(D2D1::Matrix3x2F::Identity());

    m_d2dContext->SaveDrawingState(m_stateBlock.get());

    const D2D1_SIZE_F renderTargetSize = m_d2dContext->GetSize();
    m_d2dContext->BeginDraw();

    //Windows::Foundation::Size logicalSize = m_deviceResources->GetOutputSize();

    //D2D1::Matrix3x2F screenTranslation = D2D1::Matrix3x2F::Translation(
    //    logicalSize.Width - 300,
    //    logicalSize.Height - 500
    //);

    //m_d2dContext->SetTransform(screenTranslation);// * m_deviceResources->GetOrientationTransform2D()


    m_d2dContext->Clear(m_textInfo.Background);

    const UINT32 textLength = text ? static_cast<UINT32>(wcslen(text)) : 0;
    if (textLength > 0) {
        const auto& margin = m_textInfo.Margin;
        m_d2dContext->DrawText(text,
                               textLength,
                               m_textFormat.get(),
                               D2D1::RectF(m_textInfo.Margin,
                                           m_textInfo.Margin,
                                           renderTargetSize.width - m_textInfo.Margin * 2,
                                           renderTargetSize.height - m_textInfo.Margin * 2),
                               m_brush.get());
    }

    m_d2dContext->EndDraw();

    m_d2dContext->RestoreDrawingState(m_stateBlock.get());
}



//std::shared_ptr<Pbr::Material> engine::TextTexture::CreatePbrMaterial(const Pbr::Resources& pbrResources) const {
//    auto material = Pbr::Material::CreateFlat(pbrResources, Pbr::RGBA::White);
//
//    winrt::com_ptr<ID3D11ShaderResourceView> textSrv;
//    CHECK_HRCMD(pbrResources.GetDevice()->CreateShaderResourceView(Texture(), nullptr, textSrv.put()));
//
//    material->SetTexture(Pbr::ShaderSlots::BaseColor, textSrv.get());
//    material->SetAlphaBlended(true);
//
//    return material;
//}
