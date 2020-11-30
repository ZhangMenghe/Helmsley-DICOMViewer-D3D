#ifndef CUTTING_PLANE_H
#define CUTTING_PLANE_H

#include <Renderers/quadRenderer.h>

typedef struct{
    DirectX::XMFLOAT3 p;
    DirectX::XMFLOAT3 n;
}cPlane;


class cuttingPlane {
private:
    const float CUTTING_RADIUS = 0.5f;
    const float DEFAULT_CUTTING_SCALE = 1.5f;
    const float DEFAULT_TRAVERSAL_SCALE = 0.1f;

    const int center_sample_gap = 5;
    float CMOVE_UNIT_SIZE = 0.02f;

    cPlane cplane_;
    quadRenderer* quad_render_ = nullptr;

    //in object coordinate
    DirectX::XMFLOAT3 p_start_, p_norm_, p_point_;
    DirectX::XMFLOAT3 p_scale = { 1.0f, 1.0f, 1.0f };
    DirectX::XMMATRIX p_rotate_mat_;

    //in world space
    DirectX::XMFLOAT3 p_point_world;

    //cached transformation matrix
    DirectX::XMMATRIX p_p2w_mat, p_p2o_mat;
    bool p_p2o_dirty = true;

    //reserved params
    struct reservedVec{
        DirectX::XMFLOAT3 point, scale;
        DirectX::XMMATRIX rotate_mat;
        float move_value;
        reservedVec(){}
    };

    //Center Line Traversal cutting position, range[0,4000]
    int clp_id_;
    reservedVec rc, rt;
    dvr::PARAM_CUT_ID last_mode = dvr::CUT_CUTTING_PLANE;
    float cmove_value = .0f;
    const float plane_color_[4] = {
        1.0, .0, .0, 0.4f
    };
    const float CUTTING_FACTOR = 0.00002f;
    std::unordered_map<dvr::ORGAN_IDS, float*> pmap;
    float thick_scale;
    bool centerline_available;

    void draw_plane(){}
    void draw_baked(){}
    bool keep_cutting_position(){}
    void update_modelMat_o(){}
    void update_plane_(DirectX::XMMATRIX rotMat){}
    void update_plane_(DirectX::XMFLOAT3 pNorm){}
    void set_centerline_cutting(dvr::ORGAN_IDS oid, int& id, DirectX::XMFLOAT3& pp, DirectX::XMFLOAT3& pn) {}

public:
    cuttingPlane(ID3D11Device* device);
    cuttingPlane(ID3D11Device* device, DirectX::XMFLOAT3 ps, DirectX::XMFLOAT3 pn);
    void Update() {}
    void UpdateAndDraw() {}
    void Draw(bool pre_draw) {}
    void SwitchCuttingPlane(dvr::PARAM_CUT_ID cut_plane_id) {}
    void setupCenterLine(dvr::ORGAN_IDS id, float* data) {}
    void setCenterLinePos(int id, int delta_id = 0) {}
    void setDimension(int pd, float thickness_scale){CMOVE_UNIT_SIZE = 1.0f / (float)pd;thick_scale = thickness_scale;centerline_available= false;}

    void setCutPlane(float value) {}
    void setCuttingPlaneDelta(int delta) {}
    void setCutPlane(DirectX::XMFLOAT3 normal) {}
    void setCutPlane(DirectX::XMFLOAT3 startPoint, DirectX::XMFLOAT3 normal) {}
    float* getCutPlane() { return nullptr; }
    void getCurrentTraversalInfo(DirectX::XMFLOAT3& pp, DirectX::XMFLOAT3& pn) {}
    DirectX::XMMATRIX getRotationMat(){return p_rotate_mat_;}
    void onReset(){}
    void onRotate(float offx, float offy) {}
    void onScale(float sx, float sy=-1.0f, float sz=-1.0f) {}
    void onTranslate(float offx, float offy) {}
    bool IsCenterLineAvailable(){return centerline_available;}
    void getCuttingPlane(DirectX::XMFLOAT3& pp, DirectX::XMFLOAT3& pn) {}
};


#endif
