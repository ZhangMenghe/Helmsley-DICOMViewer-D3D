#include "pch.h"
#include "templateButton.h"
#include <Common/DirectXHelper.h>
#include <opencv2/imgcodecs.hpp>
#include <Utils/TypeConvertUtils.h>
#include <Common/Manager.h>
#include <Common/uiTestHelper.h>
#include <opencv2/core/mat.hpp>
//namespace {
//    constexpr DXGI_FORMAT TextFormat = DXGI_FORMAT_B8G8R8A8_UNORM;
//}
std::string type2str(int type) {
    std::string r;

    uchar depth = type & CV_MAT_DEPTH_MASK;
    uchar chans = 1 + (type >> CV_CN_SHIFT);

    switch (depth) {
    case CV_8U:  r = "8U"; break;
    case CV_8S:  r = "8S"; break;
    case CV_16U: r = "16U"; break;
    case CV_16S: r = "16S"; break;
    case CV_32S: r = "32S"; break;
    case CV_32F: r = "32F"; break;
    case CV_64F: r = "64F"; break;
    default:     r = "User"; break;
    }

    r += "C";
    r += (chans + '0');

    return r;
}

templateButton::templateButton(const std::shared_ptr<DX::DeviceResources>& deviceResources, 
    std::string bg_file_name, std::string template_file_name,
    glm::vec3 p, glm::vec3 s, glm::mat4 r)
    : m_deviceResources(deviceResources){
    
    cv::Mat button_image = cv::imread(DX::getFilePath(bg_file_name, true), cv::IMREAD_COLOR);
    auto test = type2str(button_image.type());
    if (!button_image.empty()) {
        //std::cout << gizmo_image.cols << " " << gizmo_image.rows << std::endl;
        D3D11_TEXTURE2D_DESC texDesc;
        texDesc.Width = button_image.cols;
        texDesc.Height = button_image.rows;
        texDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        texDesc.Usage = D3D11_USAGE_DEFAULT;
        texDesc.MipLevels = 1;
        texDesc.ArraySize = 1;
        texDesc.SampleDesc.Count = 1;
        texDesc.SampleDesc.Quality = 0;
        texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        texDesc.CPUAccessFlags = 0;
        texDesc.MiscFlags = 0;

        m_button_texture = std::make_unique<Texture>();
        m_button_texture->Initialize(m_deviceResources->GetD3DDevice(), texDesc);
        
        uint8_t* pixelBufferData = new uint8_t[4* texDesc.Width * texDesc.Height];

        for (int r = 0; r < texDesc.Height; ++r) {
            cv::Vec3b* color_r = button_image.ptr<cv::Vec3b>(r);
            auto rid = 4 * r * texDesc.Width;
            for (int c = 0; c < texDesc.Width; ++c) {
                cv::Vec3b& bgr = color_r[c];
                pixelBufferData[rid + c * 4] = bgr[0]; pixelBufferData[rid + c * 4 + 1] = bgr[1]; pixelBufferData[rid + c * 4 + 2] = bgr[2];
            }
        }

        m_button_texture->setTexData(m_deviceResources->GetD3DDeviceContext(), pixelBufferData, texDesc.Width * sizeof(uint8_t) * 4, 0);
    
        m_button_quad = std::make_unique<quadRenderer>(m_deviceResources->GetD3DDevice());
        m_button_quad->setTexture(m_button_texture.get());
        m_button_mat = DirectX::XMMatrixScaling(s.x, s.y, s.z)
            * mat42xmmatrix(r)
            * DirectX::XMMatrixTranslation(p.x, p.y, p.z);
    }

    std::vector<std::string> data_lines;
    DX::ReadAllLines("textures\\gizmo-bounding.txt", data_lines, true);
    if (!data_lines.empty()) {
        m_template = cv::Mat::zeros(button_image.rows, button_image.cols, CV_8UC1);
        std::string tmp_str;
        //uint8 tmp_value;
        for (int i = 1; i < data_lines.size(); i++) {
            std::stringstream ssv(data_lines[i]);
            for (int iv = 0; ssv.good() && iv<button_image.cols; iv++) {
                std::getline(ssv, tmp_str, ',');
                auto tmp_value = std::stoi(tmp_str);
                if (tmp_value > 0) {
                    if (m_sub_regions.count(tmp_value) == 0)m_sub_regions[tmp_value] = 0;
                    m_template.at<uint8>(i - 1, iv) = tmp_value;
                }
            }
        }
    }
}
bool templateButton::CheckHit(float screen_width, float screen_height, float px, float py, int& sub_id) {
    auto projMat = DirectX::XMMatrixMultiply(Manager::camera->getVPMat(),
        DirectX::XMMatrixTranspose(m_button_mat));
    glm::vec3 pos, size;
    HDUI::update_board_projection_pos(screen_width, screen_height, projMat, size, pos);
    if (!HDUI::point2d_inside_rectangle(pos.x, pos.y, size.x, size.y, px, py)) return false;
    sub_id = find_sub_hit_spot(int((px-pos.x) / size.x * m_template.cols), int((py-pos.y) / size.y * m_template.rows));
    return (sub_id > 0);
}
int templateButton::find_sub_hit_spot(int px, int py) {
    auto start_x = fmax(px - m_sample_radius, 0);
    auto end_x = fmin(px + m_sample_radius, m_template.cols);
    auto start_y = fmax(py - m_sample_radius, 0);
    auto end_y = fmin(py + m_sample_radius, m_template.rows);

    for (auto y = start_y; y < end_y; y++) 
        for (auto x = start_x; x < end_x; x++) 
            m_sub_regions[m_template.at<uint8>(y, x)]++;
    int max_id = 0, max_value = -1;
    for (auto& itr : m_sub_regions) { 
        if (itr.second > max_value) {
            max_value = itr.second; max_id = itr.first;
        }
        itr.second = 0;
    }
    return max_id;
}
void templateButton::Render() {
    m_button_quad->Draw(m_deviceResources->GetD3DDeviceContext(), m_button_mat);
}
//void TextTexture::Draw(const wchar_t* text) {
//    auto m_d2dContext = m_deviceResources->GetD2DDeviceContext();
//
//    m_d2dContext->SetTarget(m_d2dTargetBitmap.get());
//    m_d2dContext->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);
//    m_d2dContext->SetTransform(D2D1::Matrix3x2F::Identity());
//
//    m_d2dContext->SaveDrawingState(m_stateBlock.get());
//
//    const D2D1_SIZE_F renderTargetSize = m_d2dContext->GetSize();
//    m_d2dContext->BeginDraw();
//
//    //Windows::Foundation::Size logicalSize = m_deviceResources->GetOutputSize();
//
//    //D2D1::Matrix3x2F screenTranslation = D2D1::Matrix3x2F::Translation(
//    //    logicalSize.Width - 300,
//    //    logicalSize.Height - 500
//    //);
//
//    //m_d2dContext->SetTransform(screenTranslation);// * m_deviceResources->GetOrientationTransform2D()
//
//
//    m_d2dContext->Clear(m_textInfo.Background);
//
//    const UINT32 textLength = text ? static_cast<UINT32>(wcslen(text)) : 0;
//    if (textLength > 0) {
//        const auto& margin = m_textInfo.Margin;
//        m_d2dContext->DrawText(text,
//                               textLength,
//                               m_textFormat.get(),
//                               D2D1::RectF(m_textInfo.Margin,
//                                           m_textInfo.Margin,
//                                           renderTargetSize.width - m_textInfo.Margin * 2,
//                                           renderTargetSize.height - m_textInfo.Margin * 2),
//                               m_brush.get());
//    }
//
//    m_d2dContext->EndDraw();
//
//    m_d2dContext->RestoreDrawingState(m_stateBlock.get());
//}
//
//
//
////std::shared_ptr<Pbr::Material> engine::TextTexture::CreatePbrMaterial(const Pbr::Resources& pbrResources) const {
////    auto material = Pbr::Material::CreateFlat(pbrResources, Pbr::RGBA::White);
////
////    winrt::com_ptr<ID3D11ShaderResourceView> textSrv;
////    CHECK_HRCMD(pbrResources.GetDevice()->CreateShaderResourceView(Texture(), nullptr, textSrv.put()));
////
////    material->SetTexture(Pbr::ShaderSlots::BaseColor, textSrv.get());
////    material->SetAlphaBlended(true);
////
////    return material;
////}
