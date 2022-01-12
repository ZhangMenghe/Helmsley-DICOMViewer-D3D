#include "pch.h"
#include "Manager.h"
#include <D3DPipeline/Primitive.h>
Camera* Manager::camera = nullptr;
std::vector<bool> Manager::param_bool;
std::vector<std::string> Manager::shader_contents;
bool Manager::baked_dirty_, Manager::mvp_dirty_;
bool Manager::new_data_available;
dvr::ORGAN_IDS Manager::traversal_target_id;
int Manager::screen_w, Manager::screen_h;
bool Manager::show_ar_ray, Manager::volume_ar_hold;
float Manager::indiv_rendering_params[3] = { 1.0f, 0.3f, 100.f };

Manager* Manager::myPtr_ = nullptr;

Manager* Manager::instance() {
    if (myPtr_ == nullptr) myPtr_ = new Manager;
    return myPtr_;
}

Manager::Manager() {
    if (myPtr_ != nullptr) return;
    myPtr_ = this;
    shader_contents = std::vector<std::string>(dvr::SHADER_ALL_END - 1);
    screen_w = 0; screen_h = 0;
    show_ar_ray = false; volume_ar_hold = false; baked_dirty_ = true;
    onReset();
}
Manager::~Manager() {
    if (camera) delete camera;
    param_bool.clear();
    shader_contents.clear();
}

void Manager::onReset() {
    if (camera) { delete camera; camera = nullptr; }
    clear_opacity_widgets();
    baked_dirty_ = true; mvp_dirty_ = false;
    m_dirty_wid = -1;
}

void Manager::clear_opacity_widgets() {
    m_volset_data.u_visible_bits = 0;
    m_volset_data.u_widget_num = 0;
    for (auto param : widget_params_) param.clear();
    widget_params_.clear();
    widget_visibilities_.clear();
    if (default_widget_points_ != nullptr) { delete[]default_widget_points_; default_widget_points_ = nullptr; }
    baked_dirty_ = true;
}
void Manager::updateCamera(const DirectX::XMMATRIX pose, const DirectX::XMMATRIX proj){
    camera->update(pose, proj);
}

void Manager::onViewChange(int w, int h) {
    camera->setProjMat(w, h);
    screen_w = w; screen_h = h;
}

void Manager::InitCheckParams(std::vector<std::string> keys, std::vector<bool> values) {
    if (keys.size() != values.size()) return;
    param_checks = keys; param_bool = values;

    m_volset_data.u_show_organ = param_bool[dvr::CHECK_MASKON];
    m_volset_data.u_mask_recolor = param_bool[dvr::CHECK_MASK_RECOLOR];
    //TODO: MAKE IT A VARABLE
    m_volset_data.u_show_annotation = true;

    baked_dirty_ = true;
}
void Manager::addOpacityWidget(float* values, int value_num) {
    widget_params_.push_back(std::vector<float>(dvr::TUNE_END, 0));
    widget_visibilities_.push_back(true);

    int wid = m_volset_data.u_widget_num;
    if (value_num < dvr::TUNE_END) memset(widget_params_[wid].data(), .0f, dvr::TUNE_END * sizeof(float));
    memcpy(widget_params_[wid].data(), values, value_num * sizeof(float));
    if (!default_widget_points_) getGraphPoints(values, default_widget_points_);

    //update data for shader
    for (int i = 0; i < 3; i++) {
        m_volset_data.u_opacity[3 * wid + i] = {
            default_widget_points_[4 * i],
            default_widget_points_[4 * i + 1] ,
            default_widget_points_[4 * i + 2] ,
            default_widget_points_[4 * i + 3]
        };
    }
    m_volset_data.u_widget_num = wid + 1;
    m_volset_data.u_visible_bits |= 1 << wid;
    baked_dirty_ = true;
    m_dirty_wid = wid;
}
void Manager::removeOpacityWidget(int wid) {
    if (wid >= m_volset_data.u_widget_num) return;
    widget_params_.erase(widget_params_.begin() + wid);
    widget_visibilities_.erase(widget_visibilities_.begin() + wid);

    //update data for shader
    for (int swid = wid; swid < m_volset_data.u_widget_num - 1; swid++) {
        for (int i = 0; i < 3; i++) {
            auto p = m_volset_data.u_opacity[3 * (swid + 1) + i];
            m_volset_data.u_opacity[3 * wid + i] = {
                p.x, p.y, p.z, p.w
            };
        }
    }
    m_volset_data.u_widget_num--;
    m_volset_data.u_visible_bits = 0;
    for (int i = 0; i < m_volset_data.u_widget_num; i++)m_volset_data.u_visible_bits |= int(widget_visibilities_[i]) << i;
    baked_dirty_ = true;
    m_dirty_wid = wid;
}
void Manager::removeAllOpacityWidgets() {
    clear_opacity_widgets();
}
void Manager::setRenderParam(int id, float value) {
    m_render_params[id] = value; Manager::baked_dirty_ = true;
    if (id == dvr::RENDER_CONTRAST_LOW)m_volset_data.u_contrast_low = value;
    else if (id == dvr::RENDER_CONTRAST_HIGH)m_volset_data.u_contrast_high = value;
    else if(id == dvr::RENDER_BRIGHTNESS) m_volset_data.u_brightness = value;
    else m_volset_data.u_base_value = value;
}
void Manager::setRenderParam(float* values) {
    memcpy(m_render_params, values, dvr::PARAM_RENDER_TUNE_END * sizeof(float));
    m_volset_data.u_contrast_low = m_render_params[dvr::RENDER_CONTRAST_LOW];
    m_volset_data.u_contrast_high = m_render_params[dvr::RENDER_CONTRAST_HIGH];
    m_volset_data.u_brightness = m_render_params[dvr::RENDER_BRIGHTNESS];
    m_volset_data.u_base_value = m_render_params[dvr::RENDER_BASE_VALUE];
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
void Manager::setColorScheme(int id) {
    m_volset_data.u_color_scheme = id;
    baked_dirty_ = true;
}
void Manager::setAnnotationMixedRate(float ratio) {
    m_volset_data.u_annotate_rate = ratio;
    baked_dirty_ = true;
}
void Manager::setDimension(glm::vec3 dim) {
    m_volset_data.u_tex_size = { (UINT)dim.x, (UINT)dim.y, (UINT)dim.z, (UINT)0 };
    new_data_available = true;
}
void Manager::setOpacityWidgetId(int id) {
    if (id >= m_volset_data.u_widget_num) return;
    m_current_wid = id;
    baked_dirty_ = true;
}
void Manager::setOpacityValue(int pid, float value) {
    if (pid >= dvr::TUNE_END) return;
    m_dirty_wid = m_current_wid;
    widget_params_[m_current_wid][pid] = value;

    getGraphPoints(widget_params_[m_current_wid].data(), dirty_widget_points_);

    for (int i = 0; i < 3; i++)
        m_volset_data.u_opacity[3 * m_current_wid + i] = {
        dirty_widget_points_[4 * i],
        dirty_widget_points_[4 * i + 1],
        dirty_widget_points_[4 * i + 2],
        dirty_widget_points_[4 * i + 3]};

    baked_dirty_ = true;
}
void Manager::setOpacityWidgetVisibility(int wid, bool visible) {
    widget_visibilities_[wid] = visible;
    if (visible) m_volset_data.u_visible_bits |= 1 << wid;
    else m_volset_data.u_visible_bits &= ~(1 << wid);
    baked_dirty_ = true;
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

    if (points != nullptr) { delete[] points; points = nullptr; }
    points = new float[12]{
            lb.x, lb.y, lm.x, lm.y, lt.x, lt.y,
            rb.x, rb.y, rm.x, rm.y, rt.x, rt.y
    };
}

void Manager::addMVPStatus(const std::string& name, glm::mat4 rm, glm::vec3 sv, glm::vec3 pv, bool use_as_current_status) {
    auto it = m_mvp_status.find(name);
    if (it == m_mvp_status.end()) {
        m_mvp_status[name] = new reservedStatus(rm, sv, pv);
    }
    else {
        m_mvp_status[name]->pos_vec = pv; m_mvp_status[name]->rot_mat = rm; m_mvp_status[name]->scale_vec = sv;
    }
    if (use_as_current_status) setMVPStatus(name);
}

void Manager::addMVPStatus(const std::string& name, bool use_as_current_status) {
    auto it = m_mvp_status.find(name);
    if (it != m_mvp_status.end()) delete m_mvp_status[name];
    m_mvp_status[name] = new reservedStatus();
    if (use_as_current_status) setMVPStatus(name);
}
bool Manager::removeMVPStatus(const std::string& name) {
    auto it = m_mvp_status.find(name);
    if (it == m_mvp_status.end()) return false;
    m_mvp_status.erase(name);
    setMVPStatus(m_mvp_status.begin()->first);
}

bool Manager::setMVPStatus(const std::string& name) {
    if (m_mvp_status.find(name) == m_mvp_status.end()) return false;
    if (name == m_current_mvp_name) return true;
    m_last_mvp_name = m_current_mvp_name; m_current_mvp_name = name;
    mvp_dirty_ = true;
    return true;
}
void Manager::getCurrentMVPStatus(glm::mat4& rm, glm::vec3& sv, glm::vec3& pv) {
    //camera->Reset();
    //if (screen_w != 0)camera->setProjMat(screen_w, screen_h);

    if (!m_last_mvp_name.empty() && m_mvp_status.find(m_last_mvp_name) != m_mvp_status.end()) {
        m_mvp_status[m_last_mvp_name]->rot_mat = rm; m_mvp_status[m_last_mvp_name]->scale_vec = sv; m_mvp_status[m_last_mvp_name]->pos_vec = pv;
    }

    auto rstate_ = m_mvp_status[m_current_mvp_name];
    rm = rstate_->rot_mat; sv = rstate_->scale_vec; pv = rstate_->pos_vec;
    mvp_dirty_ = false;

    //TCHAR buf[1024];
    //size_t cbDest = 1024 * sizeof(TCHAR);
    //StringCbPrintf(buf, cbDest, TEXT("====get:%s: (%f,%f,%f)\n"), m_current_mvp_name.c_str(), sv.x, sv.y, sv.z);
    //OutputDebugString(buf);
}

/// <summary>
/// static functions
/// </summary>
bool Manager::IsCuttingNeedUpdate() {
    return param_bool[dvr::CHECK_CUTTING] || param_bool[dvr::CHECK_CENTER_LINE_TRAVEL];
}
bool Manager::IsCuttingEnabled() {
    return param_bool[dvr::CHECK_CUTTING] || (param_bool[dvr::CHECK_CENTER_LINE_TRAVEL] && param_bool[dvr::CHECK_TRAVERSAL_VIEW]);
}
void Manager::setTraversalTargetId(int id) {
    traversal_target_id = (id == 0) ? dvr::ORGAN_COLON : dvr::ORGAN_ILEUM;
}