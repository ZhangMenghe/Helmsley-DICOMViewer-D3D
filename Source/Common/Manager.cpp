#include "pch.h"
#include "Manager.h"
Camera* Manager::camera = nullptr;
std::vector<bool> Manager::param_bool;
std::vector<std::string> Manager::shader_contents;
bool Manager::baked_dirty_;
bool Manager::new_data_available;
//dvr::ORGAN_IDS Manager::traversal_target_id;
int Manager::screen_w, Manager::screen_h;
bool Manager::show_ar_ray, Manager::volume_ar_hold;

Manager* Manager::myPtr_ = nullptr;

Manager* Manager::instance() {
    if (myPtr_ == nullptr) myPtr_ = new Manager;
    return myPtr_;
}

Manager::Manager(){
    if (myPtr_ != nullptr) return;
    myPtr_ = this;
    shader_contents = std::vector<std::string>(dvr::SHADER_ALL_END-1);
    screen_w = 0; screen_h = 0;
    show_ar_ray = false; volume_ar_hold = false; baked_dirty_ = true;
    onReset();
}
Manager::~Manager(){
    if(camera) delete camera;
    param_bool.clear();
    shader_contents.clear();
}

void Manager::onReset(){
    if(camera){delete camera; camera= nullptr;}
    baked_dirty_ = true;
}
void Manager::onViewChange(int w, int h){
    camera->setProjMat(w, h);
    screen_w = w; screen_h = h;
}
bool Manager::isRayCut(){return param_bool[dvr::CHECK_RAYCAST] && param_bool[dvr::CHECK_CUTTING];}
bool Manager::IsCuttingNeedUpdate(){
    return param_bool[dvr::CHECK_CUTTING] || param_bool[dvr::CHECK_CENTER_LINE_TRAVEL];
}
bool Manager::IsCuttingEnabled(){
    return param_bool[dvr::CHECK_CUTTING] ||(param_bool[dvr::CHECK_CENTER_LINE_TRAVEL] && param_bool[dvr::CHECK_TRAVERSAL_VIEW]);
}

//adder
void Manager::InitCheckParams(int num, const char* keys[], bool values[]) {
    param_checks.clear(); param_bool.clear();
    for (int i = 0; i < num; i++) {
        param_checks.push_back(std::string(keys[i]));
        Manager::param_bool.push_back(values[i]);
        //    LOGE("======SET INIT %s, %d\n", keys[i], values[i]);
    }

    m_volset_data.u_show_organ = param_bool[dvr::CHECK_MASKON];
    m_volset_data.u_mask_recolor = param_bool[dvr::CHECK_MASK_RECOLOR];

    baked_dirty_ = true;
}

void Manager::addOpacityWidget(int value_num, float* values) {
    // todo : real implementation
    float* points;
    getGraphPoints(values, points);

    for (int i = 0; i < 3; i++)
        m_volset_data.u_opacity[i] = {
        points[4 * i], 
        points[4 * i + 1] , 
        points[4 * i + 2] , 
        points[4 * i + 3] 
    };
    m_volset_data.u_widget_num = 1;
    m_volset_data.u_visible_bits = 1;
    baked_dirty_ = true;
}
void Manager::setRenderParam(int id, float value) {
    m_render_params[id] = value; Manager::baked_dirty_ = true;
    if (id == dvr::RENDER_CONTRAST_LOW)m_volset_data.u_contrast_low = value;
    else if (id == dvr::RENDER_CONTRAST_HIGH)m_volset_data.u_contrast_high = value;
    else m_volset_data.u_brightness = m_render_params[dvr::RENDER_BRIGHTNESS];
}
void Manager::setRenderParam(float* values) {
    memcpy(m_render_params, values, dvr::PARAM_RENDER_TUNE_END * sizeof(float));
    m_volset_data.u_contrast_low = m_render_params[dvr::RENDER_CONTRAST_LOW];
    m_volset_data.u_contrast_high = m_render_params[dvr::RENDER_CONTRAST_HIGH];
    m_volset_data.u_brightness = m_render_params[dvr::RENDER_BRIGHTNESS];
    Manager::baked_dirty_ = true;
}
void Manager::setCheck(std::string key, bool value) {
    auto it = std::find(param_checks.begin(), param_checks.end(), key);
    if (it != param_checks.end()) {
        param_bool[it - param_checks.begin()] = value;
        m_volset_data.u_show_organ = param_bool[dvr::CHECK_MASKON];
        m_volset_data.u_mask_recolor = param_bool[dvr::CHECK_MASK_RECOLOR];
        baked_dirty_ = true;
    }
}

void Manager::setMask(UINT num, UINT bits) {
    m_volset_data.u_organ_num = num; m_volset_data.u_maskbits = bits;
    Manager::baked_dirty_ = true;
}
//setter
void Manager::setColorScheme(int id) {
    m_volset_data.u_color_scheme = id;
    baked_dirty_ = true;
}
void Manager::setDimension(glm::vec3 dim) {
    m_volset_data.u_tex_size = { (UINT)dim.x, (UINT)dim.y, (UINT)dim.z, (UINT)0 };
    new_data_available = true;
}
void Manager::getGraphPoints(float values[], float*& points) {
    DirectX::XMFLOAT2 lb, lm, lt, rb, rm, rt;
    float half_top = values[dvr::TUNE_WIDTHTOP] / 2.0f;
    float half_bottom = std::fmax(values[dvr::TUNE_WIDTHBOTTOM] / 2.0f, half_top);

    float lb_x = values[dvr::TUNE_CENTER] - half_bottom;
    float rb_x = values[dvr::TUNE_CENTER] + half_bottom;

    lb = DirectX::XMFLOAT2(lb_x, .0f);
    rb = DirectX::XMFLOAT2(rb_x, .0f);

    lt = DirectX::XMFLOAT2(values[dvr::TUNE_CENTER] - half_top, values[dvr::TUNE_OVERALL]);
    rt = DirectX::XMFLOAT2(values[dvr::TUNE_CENTER] + half_top, values[dvr::TUNE_OVERALL]);

    float mid_y = values[dvr::TUNE_LOWEST] * values[dvr::TUNE_OVERALL];
    lm = DirectX::XMFLOAT2(lb_x, mid_y);
    rm = DirectX::XMFLOAT2(rb_x, mid_y);

    //if (points) delete points;
    points = new float[12]{
            lb.x, lb.y, lm.x, lm.y, lt.x, lt.y,
            rb.x, rb.y, rm.x, rm.y, rt.x, rt.y
    };
}
