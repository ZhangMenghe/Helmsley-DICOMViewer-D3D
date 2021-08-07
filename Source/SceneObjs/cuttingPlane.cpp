#include "pch.h"
#include "cuttingPlane.h"

#include <glm/gtx/transform.hpp>
#include <glm/gtx/rotate_vector.hpp>

#include <Utils/TypeConvertUtils.h>
#include <Utils/MathUtils.h>
#include <Common/Manager.h>
#include <vrController.h>
using namespace glm;

cuttingController::cuttingController(ID3D11Device* device) {
    onReset(device);
}
cuttingController::cuttingController(ID3D11Device* device, DirectX::XMMATRIX model_mat) {
    mat4 vm_inv = transpose(inverse(xmmatrix2mat4(model_mat)));
    update_plane_(vm_inv, vec3MatNorm(vm_inv, float32vec3(Manager::camera->getViewDirection())));
    update_plane_(device);
}
cuttingController::cuttingController(ID3D11Device* device, DirectX::XMFLOAT3 ps, DirectX::XMFLOAT3 pn){
    p_start_ = glm::vec3(ps.x, ps.y, ps.z); p_norm_ = glm::vec3(pn.x, pn.y, pn.z);
    p_rotate_mat_ = rotMatFromDir(p_norm_);
    update_plane_(device);
}

void cuttingController::onReset(ID3D11Device* device) {
    p_start_ = vec3(.0, .0, 0.6f);
    p_norm_ = vec3(.0, .0, -1.0);
    p_rotate_mat_ = rotMatFromDir(p_norm_);
    update_plane_(device);
}
void cuttingController::init_plane_renderer(ID3D11Device* device) {
    for (int a = 0, i = 0; a < 360; a += 360 / 20, i++) {
        double heading = a * 3.1415926535897932384626433832795 / 180;
        m_quad_vertices[3 * i] = cos(heading) * CUTTING_RADIUS;
        m_quad_vertices[3 * i + 1] = sin(heading) * CUTTING_RADIUS;
    }
    unsigned short* indices = new unsigned short[60];
    for (int i = 0; i < 20; i++) {
        indices[3 * i] = 20;
        indices[3 * i+1] = (i + 1) % 20 ;
        indices[3 * i + 2] = i;
    }
    plane_render_ = new quadRenderer(device,
        L"Naive3DVertexShader.cso", L"SimpleColorPixelShader.cso",
        m_quad_vertices, indices, 63, 60, dvr::INPUT_POS_3D);
    plane_render_->initialize();

    CD3D11_BUFFER_DESC pixconstBufferDesc(sizeof(dvr::ColorConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);
    dvr::ColorConstantBuffer tdata;
    tdata.u_color = { 1.0, .0, .0, 0.4f };
    D3D11_SUBRESOURCE_DATA color_resource;
    color_resource.pSysMem = &tdata;
    plane_render_->createPixelConstantBuffer(device, pixconstBufferDesc, &color_resource);
}
void cuttingController::Update(glm::mat4 model_mat) {
    if (keep_cutting_position()) {//keep it static
        p_norm_ = vec3MatNorm(glm::inverse(model_mat), float32vec3(Manager::camera->getViewDirection()));
        p_point_ += p_norm_ * cmove_value;

        p_rotate_mat_ = rotMatFromDir(p_norm_);
        p_p2o_mat = glm::translate(glm::mat4(1.0), p_point_) * p_rotate_mat_ * glm::scale(glm::mat4(1.0), p_scale);
        p_p2w_mat = glm::mat4(1.0);//p_p2o_mat;
        p_point_world = glm::vec3(p_p2w_mat * glm::vec4(p_point_, 1.0f));
    }
    else {
        update_modelMat_o();
        p_point_world = glm::vec3(model_mat * glm::vec4(p_point_, 1.0f));
        p_p2w_mat = model_mat * p_p2o_mat;
    }
}
bool cuttingController::Draw(ID3D11DeviceContext* context) {
    context->OMSetBlendState(d3dBlendState, 0, 0xffffffff);
    bool render_complete = plane_render_->Draw(context, mat42xmmatrix(p_p2w_mat));
    context->OMSetBlendState(nullptr, 0, 0xffffffff);
    return render_complete;
}
bool cuttingController::Draw(ID3D11DeviceContext* context, bool is_front) {
    bool is_front_same = glm::dot(p_norm_, glm::vec3(0, 0, -1.0)) > .0;
    if(is_front_same != is_front) context->RSSetState(is_front ? vrController::instance()->m_render_state_back: vrController::instance()->m_render_state_front);
    bool render_complete = Draw(context);
    if (is_front_same != is_front) context->RSSetState(is_front ? vrController::instance()->m_render_state_front: vrController::instance()->m_render_state_back);
    return render_complete;
}
void cuttingController::getCuttingPlane(glm::vec3& pp, glm::vec3& pn) {
    pp = p_point_; pn = p_norm_;
}
void cuttingController::getCuttingPlane(DirectX::XMFLOAT4& pp, DirectX::XMFLOAT4& pn) {
    pp = { p_point_.x, p_point_.y, p_point_.z, 0 };
    pn = { p_norm_.x, p_norm_.y, p_norm_.z, 0 };
}

//for texture based only
void cuttingController::setCutPlane(float value) {
    p_point_ = p_start_ + p_norm_ * value;
    cmove_value = .0f;
    p_p2o_dirty = true;
    baked_dirty = true;
}
bool cuttingController::keep_cutting_position() {
    return Manager::param_bool[dvr::CHECK_FREEZE_CPLANE];
}
void cuttingController::setCutPlane(glm::vec3 normal) {}
void cuttingController::setCutPlane(glm::vec3 pp, glm::vec3 normal) {
    p_norm_ = glm::normalize(normal); p_point_ = pp;

    p_rotate_mat_ = rotMatFromDir(p_norm_);
    cmove_value = .0f;
    p_p2o_dirty = true;
    baked_dirty = true;
}
void cuttingController::setCuttingPlaneDelta(int delta) {
    float value = delta * CMOVE_UNIT_SIZE;
    if (keep_cutting_position()) { cmove_value = value; return; }
    p_point_ += p_norm_ * value;
    cmove_value = .0f;
    p_p2o_dirty = true;
    baked_dirty = true;
}
float* cuttingController::getCutPlane() {
    float* data = new float[6];
    memcpy(data, glm::value_ptr(p_point_), 3 * sizeof(float));
    memcpy(data + 3, glm::value_ptr(p_norm_), 3 * sizeof(float));
    return data;
}

void cuttingController::onRotate(float offx, float offy) {
    update_plane_(glm::rotate(glm::mat4(1.0f), offx, glm::vec3(0, 1, 0))
        * glm::rotate(glm::mat4(1.0f), offy, glm::vec3(1, 0, 0))
        * p_rotate_mat_);
    p_p2o_dirty = true;
    baked_dirty = true;
}

void cuttingController::onScale(float sx, float sy, float sz) {
    if (sy < .0f) p_scale = p_scale * sx;
    else p_scale = p_scale * glm::vec3(sx, sy, sz);
    p_p2o_dirty = true;
    baked_dirty = true;
}
void cuttingController::onTranslate(float offx, float offy) {
    //do nothing currently
    baked_dirty = true;
}
void cuttingController::update_modelMat_o() {
    if (!p_p2o_dirty) return;
    p_p2o_mat = glm::translate(glm::mat4(1.0), p_point_) * p_rotate_mat_ * glm::scale(glm::mat4(1.0), p_scale);
    p_p2o_dirty = false;
}
void cuttingController::update_plane_(ID3D11Device* device) {
    p_point_ = p_start_;
    // p_point_world = glm::vec3(model_mat* glm::vec4(p_point_,1.0f));
    p_scale = glm::vec3(DEFAULT_CUTTING_SCALE);
    update_modelMat_o();
    rc.point = p_point_; rc.scale = p_scale; rc.rotate_mat = p_rotate_mat_; rc.move_value = cmove_value;
    baked_dirty = true;
    if (plane_render_ == nullptr) init_plane_renderer(device);

    if (d3dBlendState == nullptr) {
        D3D11_BLEND_DESC omDesc;
        ZeroMemory(&omDesc, sizeof(D3D11_BLEND_DESC));
        omDesc.RenderTarget[0].BlendEnable = TRUE;

        omDesc.RenderTarget[0].SrcBlend =  D3D11_BLEND_SRC_ALPHA;
        omDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
        omDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
        omDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
        omDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
        omDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
        omDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
        device->CreateBlendState(&omDesc, &d3dBlendState);
    }
}
void cuttingController::update_plane_(glm::mat4 rotMat) {
    p_rotate_mat_ = rotMat;
    p_norm_ = rotateNormal(p_rotate_mat_, glm::vec3(.0f, .0f, -1.0f));
    //    mat4 vm_inv = transpose(inverse(vrController::ModelMat_));
    //    glm::vec3 vp_obj = vec3MatNorm(vm_inv, vrController::camera->getCameraPosition());
    //    //cloest point
    //    p_start_ = cloestVertexToPlane(p_norm_, vp_obj);
        //todo: percent of how p_point account for the percent with ro, rd
}
void cuttingController::update_plane_(glm::mat4 vm_inv, glm::vec3 pNorm) {
    p_norm_ = pNorm;
    p_rotate_mat_ = rotMatFromDir(pNorm);
    glm::vec3 vp_obj = vec3MatNorm(vm_inv, float32vec3(Manager::camera->getCameraPosition()));
    //cloest point
    p_start_ = cloestVertexToPlane(cube_vertices_pos_w_tex, pNorm, vp_obj) - p_norm_ * 0.1f;
    //debug
//   p_start_ = cloestVertexToPlane(pNorm, vp_obj) + p_norm_*0.5f;
}
void cuttingController::SwitchCuttingPlane(dvr::PARAM_CUT_ID cut_plane_id) {
    if (cut_plane_id == last_mode)return;

    if (cut_plane_id == dvr::CUT_CUTTING_PLANE) {
        rt.point = p_point_; rt.scale = p_scale; rt.rotate_mat = p_rotate_mat_; rt.move_value = cmove_value;
        p_scale = rc.scale;
        // setCutPlane(rc.point,rc.norm);
        p_point_ = rc.point;
        cmove_value = rc.move_value;
        p_p2o_dirty = true;
        update_plane_(rc.rotate_mat);

        last_mode = dvr::CUT_CUTTING_PLANE;
    }else {
        rc.point = p_point_; rc.scale = p_scale; rc.rotate_mat = p_rotate_mat_; rc.move_value = cmove_value;
        p_scale = rt.scale;
        // setCutPlane(rt.point,rt.norm);
        p_point_ = rt.point;
        cmove_value = rt.move_value;
        p_p2o_dirty = true;
        update_plane_(rt.rotate_mat);

        last_mode = dvr::CUT_TRAVERSAL;
    }
    baked_dirty = true;
}
void cuttingController::set_centerline_cutting(dvr::ORGAN_IDS oid, int& id, glm::vec3& pp, glm::vec3& pn) {
    id = fmax(id, center_sample_gap);
    id = fmin(id, 3999 - center_sample_gap);

    auto data = pmap[oid];
    pp = glm::vec3(data[3 * id], data[3 * id + 1], data[3 * id + 2]);
    pn = glm::vec3(data[3 * (id + center_sample_gap)], data[3 * (id + center_sample_gap) + 1], data[3 * (id + center_sample_gap) + 2])
        - glm::vec3(data[3 * (id - center_sample_gap)], data[3 * (id - center_sample_gap) + 1], data[3 * (id - center_sample_gap) + 2]);
    pn = glm::vec3(pn.x, pn.y, pn.z * thick_scale);
}
void cuttingController::setupCenterLine(dvr::ORGAN_IDS id, float* data) {
    pmap[id] = std::vector<float>(data, data+4000*3);
    if (id == dvr::DEFAULT_TRAVERSAL_ORGAN) {
        clp_id_ = 0;
        glm::vec3 pp, pn;
        set_centerline_cutting(id, clp_id_, pp, pn);
        rt.point = pp; rt.scale = glm::vec3(DEFAULT_TRAVERSAL_SCALE); rt.rotate_mat = rotMatFromDir(pn); rt.move_value = .0f;
        centerline_available = true;
        baked_dirty = true;
    }
}
void cuttingController::setCenterLinePos(int id, int delta_id) {
    if (delta_id == 0) {
        clp_id_ = id % (4000 - center_sample_gap);
    }
    else {
        clp_id_ = (clp_id_ + delta_id) % (4000 - center_sample_gap);
    }
    set_centerline_cutting(Manager::traversal_target_id, clp_id_, p_point_, p_norm_);
    setCutPlane(p_point_, p_norm_);
    baked_dirty = true;
}
void cuttingController::getCurrentTraversalInfo(glm::vec3& pp, glm::vec3& pn) {
    set_centerline_cutting(Manager::traversal_target_id, clp_id_, pp, pn);
}