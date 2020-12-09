#ifndef MANAGER_H
#define MANAGER_H

#include "pch.h"
#include <vector>
#include <Common/ConstantAndStruct.h>
#include <D3DPipeline/Camera.h>
struct volumeSetupConstBuffer {
    DirectX::XMUINT4 u_tex_size;

    //opacity widget
    DirectX::XMFLOAT4 u_opacity[30];
    int u_widget_num;
    int u_visible_bits;

    //contrast
    float u_contrast_low;
    float u_contrast_high;
    float u_brightness;

    //mask
    UINT u_maskbits;
    UINT u_organ_num;
    int u_mask_recolor;

    //others
    int u_show_organ;
    UINT u_color_scheme;//COLOR_GRAYSCALE COLOR_HSV COLOR_BRIGHT
};

class Manager {
public:
    static Manager* instance();

    static Camera* camera;
    static std::vector<bool> param_bool;
    static std::vector<std::string> shader_contents;

    static bool baked_dirty_;
    static dvr::ORGAN_IDS traversal_target_id;
    static int screen_w, screen_h;
    static bool show_ar_ray, volume_ar_hold;
    static bool isRayCut();
    static bool new_data_available;

    Manager();
    ~Manager();
    void onReset();
    void onViewChange(int w, int h);
    void InitCheckParams(int num, const char* keys[], bool values[]);
    
    static bool IsCuttingEnabled();
    static bool IsCuttingNeedUpdate();
    static bool isRayCasting();

    //getter
    volumeSetupConstBuffer* getVolumeSetupConstData(){ return &m_volset_data; }
    UINT getMaskBits() { return m_volset_data.u_maskbits; }
    bool getCheck(dvr::PARAM_BOOL id) { return param_bool[id]; }
    bool isDrawVolume() { return !param_bool[dvr::CHECK_MASKON] || param_bool[dvr::CHECK_VOLUME_ON]; }
    bool isDrawCenterLine() { return param_bool[dvr::CHECK_MASKON] && Manager::param_bool[dvr::CHECK_CENTER_LINE]; }
    bool isDrawMesh() { return param_bool[dvr::CHECK_MASKON] && Manager::param_bool[dvr::CHECK_DRAW_POLYGON]; }

    //adder
    void addOpacityWidget(float* values, int value_num);
    void removeOpacityWidget(int wid);
    void removeAllOpacityWidgets();

    //setter
    void setRenderParam(int id, float value);
    void setRenderParam(float* values);
    void setCheck(std::string key, bool value);
    void setMask(UINT num, UINT bits);
    void setColorScheme(int id);
    void setDimension(glm::vec3 dim);
    void setOpacityWidgetId(int id);
    void setOpacityValue(int pid, float value);
    void setOpacityWidgetVisibility(int wid, bool visible);
private:
    static Manager* myPtr_;
    volumeSetupConstBuffer m_volset_data;
    
    //contrast, brightness, etc
    float m_render_params[dvr::PARAM_RENDER_TUNE_END] = { .0f };
    //opacity widgets
    std::vector<std::vector<float>> widget_params_;
    std::vector<bool> widget_visibilities_;
    float* default_widget_points_ = nullptr;
    float* u_opacity_data_ = nullptr;
    int m_current_wid = -1;

    //check names
    std::vector<std::string> param_checks;

    void clear_opacity_widgets();
    //todo: move to graph renderer
    void getGraphPoints(float values[], float*& points);
};


#endif //VOLUME_RENDERING_MANAGER_H
