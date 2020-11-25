#ifndef MANAGER_H
#define MANAGER_H

#include "pch.h"
#include <vector>
#include <Common/ConstantAndStruct.h>
#include <D3DPipeline/Camera.h>
class Manager {
public:
    static Manager* instance();

    static Camera* camera;
    static std::vector<bool> param_bool;
    static std::vector<std::string> shader_contents;

    static bool baked_dirty_;
    static int color_scheme_id;
    static dvr::ORGAN_IDS traversal_target_id;
    static int screen_w, screen_h;
    static bool show_ar_ray, volume_ar_hold;
    static bool isRayCut();
    static bool new_data_available;

    Manager();
    ~Manager();
    void onReset();
    void onViewChange(int w, int h);
    void updateCamera(const XrPosef& pose, const XrFovf& fov);
    static bool IsCuttingEnabled();
    static bool IsCuttingNeedUpdate();
private:
    static Manager* myPtr_;
};


#endif //VOLUME_RENDERING_MANAGER_H
