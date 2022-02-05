#ifndef TEMPLATE_BUTTON_H
#define TEMPLATE_BUTTON_H

#include <Common/DeviceResources.h>
#include <Renderers/quadRenderer.h>
#include <D3DPipeline/Texture.h>
#include <opencv2/core.hpp>
#include <unordered_map>
class templateButton {
public:
    templateButton(const std::shared_ptr<DX::DeviceResources>& deviceResources, std::string bg_file_name, std::string template_file_name, glm::vec3 p, glm::vec3 s, glm::mat4 r);
    void Render();
    bool CheckHit(float screen_width, float screen_height, float px, float py, int& sub_id);
private:
    std::shared_ptr<DX::DeviceResources> m_deviceResources;

    std::unique_ptr<Texture> m_button_texture;
    std::unique_ptr<quadRenderer> m_button_quad;
    DirectX::XMMATRIX m_button_mat;

    cv::Mat m_template;
    const int m_sample_radius = 4;
    int find_sub_hit_spot(int px, int py);
    //<id, count>
    std::unordered_map<int, int> m_sub_regions;
};

#endif