#ifndef MANAGER_H
#define MANAGER_H

#include <vector>
#include <unordered_map>
#include <Common/ConstantAndStruct.h>
#include <D3DPipeline/Camera.h>

struct reservedStatus {
    glm::mat4 rot_mat;
    glm::vec3 scale_vec, pos_vec;
    reservedStatus(glm::mat4 rm, glm::vec3 sv, glm::vec3 pv) {
        rot_mat = rm; scale_vec = sv; pos_vec = pv;
    }
    reservedStatus() :rot_mat(dvr::DEFAULT_ROTATE), scale_vec(dvr::DEFAULT_SCALE), pos_vec(dvr::DEFAULT_POS) {}
};

struct volumeSetupConstBuffer
{
    DirectX::XMUINT4 u_tex_size;

    //opacity widget
    DirectX::XMFLOAT4 u_opacity[30];
    int u_widget_num;
    int u_visible_bits;

    //contrast
    float u_contrast_low;
    float u_contrast_high;
    float u_brightness;
    float u_base_value;

    //mask
    UINT u_maskbits;
    UINT u_organ_num;
    int u_mask_recolor;

    //others
    int u_show_organ;
    UINT u_color_scheme; //COLOR_GRAYSCALE COLOR_HSV COLOR_BRIGHT
};

class Manager
{
public:
    static Manager *instance();

    static Camera *camera;
    static std::vector<bool> param_bool;
    static std::vector<std::string> shader_contents;

    static bool baked_dirty_, mvp_dirty_;
    static dvr::ORGAN_IDS traversal_target_id;
    static int screen_w, screen_h;
    static bool show_ar_ray, volume_ar_hold;
    static bool new_data_available;
    static float indiv_rendering_params[3];

    Manager();
    ~Manager();
    void onReset();
    void onViewChange(int w, int h);
    void updateCamera(const DirectX::XMMATRIX pose, const DirectX::XMMATRIX proj);
    void InitCheckParams(std::vector<std::string> keys, std::vector<bool> values);

    static bool IsCuttingEnabled();
    static bool IsCuttingNeedUpdate();
    static void setTraversalTargetId(int id);

    //getter
    void getVolumeSetupConstData(volumeSetupConstBuffer& vd) { vd = m_volset_data; }
    
    UINT getMaskBits() { return m_volset_data.u_maskbits; }
    bool getCheck(dvr::PARAM_BOOL id) { return param_bool[id]; }
    bool isDrawVolume() { return !param_bool[dvr::CHECK_MASKON] || param_bool[dvr::CHECK_VOLUME_ON]; }
    bool isDrawCenterLine() { return param_bool[dvr::CHECK_MASKON] && Manager::param_bool[dvr::CHECK_CENTER_LINE]; }
    bool isDrawMesh() { return param_bool[dvr::CHECK_MASKON] && Manager::param_bool[dvr::CHECK_DRAW_POLYGON]; }
    int getDirtyOpacityId() { return m_dirty_wid; }
    float* getDefaultWidgetPoints() { return default_widget_points_; }
    float* getDirtyWidgetPoints() { return dirty_widget_points_; }
    std::vector<bool>* getOpacityWidgetVisibility() { return &widget_visibilities_; }

    //adder
    void addOpacityWidget(float *values, int value_num);
    void removeOpacityWidget(int wid);
    void removeAllOpacityWidgets();

    //setter
    void setRenderParam(int id, float value);
    void setRenderParam(float *values);
    void setCheck(std::string key, bool value);
    void setMask(UINT num, UINT bits);
    void setColorScheme(int id);
    void setDimension(glm::vec3 dim);
    void setOpacityWidgetId(int id);
    void setOpacityValue(int pid, float value);
    void setOpacityWidgetVisibility(int wid, bool visible);
    void resetDirtyOpacityId() { m_dirty_wid = -1; }

    //mvp status
    void addMVPStatus(const std::string& name, glm::mat4 rm, glm::vec3 sv, glm::vec3 pv, bool use_as_current_status);
    void addMVPStatus(const std::string& name, bool use_as_current_status);
    bool removeMVPStatus(const std::string& name);
    bool setMVPStatus(const std::string& name);
    void getCurrentMVPStatus(glm::mat4& rm, glm::vec3& sv, glm::vec3& pv);
private:
    static Manager* myPtr_;

    volumeSetupConstBuffer m_volset_data;

    //contrast, brightness, etc
    float m_render_params[dvr::PARAM_RENDER_TUNE_END] = {.0f};
    
    //opacity widgets
    std::vector<std::vector<float>> widget_params_;
    std::vector<bool> widget_visibilities_;
    float *default_widget_points_ = nullptr, *dirty_widget_points_ = nullptr;
    int m_current_wid = -1;
    int m_dirty_wid;

    //check names
    std::vector<std::string> param_checks;

    //mvp status
    std::string m_last_mvp_name, m_current_mvp_name;
    std::unordered_map<std::string, reservedStatus*> m_mvp_status;

    void clear_opacity_widgets();
    //todo: move to graph renderer
    void getGraphPoints(float values[], float *&points);
};

#endif //VOLUME_RENDERING_MANAGER_H
