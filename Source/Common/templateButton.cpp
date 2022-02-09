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
    std::string bg_file_name, std::string template_file_name, bool from_asset,
    int img_width, int img_height,
    glm::vec3 p, glm::vec3 s, glm::mat4 r)
    : m_deviceResources(deviceResources) {

    char buffer[1024];
    std::ifstream inFile(DX::getFilePath(bg_file_name, from_asset), std::ios::in | std::ios::binary);

    if (inFile.is_open()) {
        D3D11_TEXTURE2D_DESC texDesc;
        texDesc.Width = img_width;
        texDesc.Height = img_height;
        texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        texDesc.Usage = D3D11_USAGE_DEFAULT;
        texDesc.MipLevels = 1;
        texDesc.ArraySize = 1;
        texDesc.SampleDesc.Count = 1;
        texDesc.SampleDesc.Quality = 0;
        texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        texDesc.CPUAccessFlags = 0;
        texDesc.MiscFlags = 0;

        m_button.tex = new Texture;
        m_button.tex->Initialize(m_deviceResources->GetD3DDevice(), texDesc);

        uint8_t* pixelBufferData = new uint8_t[4 * texDesc.Width * texDesc.Height];

        for (int id = 0, data_offset = 0; !inFile.eof(); id++) {
            inFile.read(buffer, 1024);
            std::streamsize len = inFile.gcount();
            if (len == 0) continue;
            UCHAR* dst_buff = pixelBufferData + data_offset;
            memcpy(dst_buff, buffer, len);
            data_offset += len;
        }

        m_button.tex->setTexData(m_deviceResources->GetD3DDeviceContext(), pixelBufferData, texDesc.Width * sizeof(uint8_t) * 4, 0);

        m_button.quad = new quadRenderer(m_deviceResources->GetD3DDevice());
        m_button.quad->setTexture(m_button.tex);
        m_button.pos = p; m_button.size = s;
        m_button.mat = DirectX::XMMatrixScaling(s.x, s.y, s.z)
            * mat42xmmatrix(r)
            * DirectX::XMMatrixTranslation(p.x, p.y, p.z);
        inFile.close();
    }

    std::vector<std::string> data_lines;
    DX::ReadAllLines(template_file_name, data_lines, from_asset);
    if (!data_lines.empty()) {
        m_template = cv::Mat::zeros(img_height, img_width, CV_8UC1);
        std::string tmp_str;
        //uint8 tmp_value;
        for (int i = 1; i < data_lines.size(); i++) {
            std::stringstream ssv(data_lines[i]);
            for (int iv = 0; ssv.good() && iv < img_width; iv++) {
                std::getline(ssv, tmp_str, ',');
                auto tmp_value = std::stoi(tmp_str);
                if (tmp_value > 0) {
                    if (m_button.sub_regions.count(tmp_value) == 0)m_button.sub_regions[tmp_value] = 0;
                    m_template.at<uint8>(i - 1, iv) = tmp_value;
                }
            }
        }
    }
}
bool templateButton::CheckHit(float screen_width, float screen_height, float px, float py, int& sub_id) {
    auto projMat = DirectX::XMMatrixMultiply(Manager::camera->getVPMat(),
        DirectX::XMMatrixTranspose(m_button.mat));
    glm::vec3 pos, size;
    HDUI::update_board_projection_pos(screen_width, screen_height, projMat, size, pos);
    if (!HDUI::point2d_inside_rectangle(pos.x, pos.y, size.x, size.y, px, py)) return false;
    sub_id = find_sub_hit_spot(int((px - pos.x) / size.x * m_template.cols), int((py - pos.y) / size.y * m_template.rows));
    return (sub_id > 0);
}
bool templateButton::CheckHit(glm::vec3 pos, float radius, int& hit_id) {
    if(!CheckHitWithinSphere(pos, radius, m_button.pos, m_button.size.x * 0.5f, m_button.size.y * 0.5f)) return false;
    auto left_top_corner = glm::vec2(m_button.pos.x - m_button.size.x * 0.5f, m_button.pos.y + m_button.size.y*0.5f);
    hit_id = find_sub_hit_spot(int((pos.x - left_top_corner.x) / m_button.size.x * m_template.cols), int((left_top_corner.y-pos.y) / m_button.size.y * m_template.rows));
    return (hit_id > 0);
}
int templateButton::find_sub_hit_spot(int px, int py) {
    auto start_x = fmax(px - m_sample_radius, 0);
    auto end_x = fmin(px + m_sample_radius, m_template.cols);
    auto start_y = fmax(py - m_sample_radius, 0);
    auto end_y = fmin(py + m_sample_radius, m_template.rows);

    for (auto y = start_y; y < end_y; y++)
        for (auto x = start_x; x < end_x; x++)
            m_button.sub_regions[m_template.at<uint8>(y, x)]++;
    int max_id = 0, max_value = -1;
    for (auto& itr : m_button.sub_regions) {
        if (itr.second > max_value) {
            max_value = itr.second; max_id = itr.first;
        }
        itr.second = 0;
    }
    return max_id;
}
void templateButton::Render() {
    m_button.quad->Draw(m_deviceResources->GetD3DDeviceContext(), m_button.mat);
}