#ifndef CONSTANTS_H
#define CONSTANTS_H
#include "pch.h"
#include <string>

namespace dvr{
    enum PARAM_BOOL{
        CHECK_RAYCAST=0,
        CHECK_OVERLAY,
        CHECK_CUTTING,
        CHECK_FREEZE_VOLUME,
        CHECK_FREEZE_CPLANE,
        CUT_PLANE_REAL_SAMPLE,
        CHECK_CENTER_LINE_TRAVEL,
        CHECK_TRAVERSAL_VIEW,
        CHECK_MASKON,
        CHECK_MASK_RECOLOR,
        CHECK_VOLUME_ON,
        CHECK_DRAW_POLYGON,
        CHECK_POLYGON_WIREFRAME,
        CHECK_CENTER_LINE,
        CHECK_AR_ENABLED,
        CHECK_AR_DRAW_POINT,
        CHECK_AR_DRAW_PLANE,
    };
    enum PARAM_TUNE{
        TUNE_OVERALL=0,
        TUNE_LOWEST,
        TUNE_WIDTHBOTTOM,
        TUNE_WIDTHTOP,
        TUNE_CENTER,
        TUNE_END
    };
    enum PARAM_CUT_ID{
        CUT_CUTTING_PLANE=0,
        CUT_TRAVERSAL
    };
    enum PARAM_DUAL{
        CONTRAST_LIMIT=0,
        DUAL_END
    };
    enum AR_REQUEST{
        PLACE_VOLUME=0,
        PLACE_ANCHOR
    };
    enum PARAM_RENDER_TUNE{
//        RENDER_CONTRAST_LEVEL=0,
        RENDER_CONTRAST_LOW=0,
        RENDER_CONTRAST_HIGH,
        RENDER_BRIGHTNESS,
        PARAM_RENDER_TUNE_END
    };
    enum SHADER_FILES{
        SHADER_TEXTUREVOLUME_VERT=0,
        SHADER_TEXTUREVOLUME_FRAG,
        SHADER_RAYCASTVOLUME_VERT,
        SHADER_RAYCASTVOLUME_FRAG,
        SHADER_RAYCASTCOMPUTE_GLSL,
        SHADER_RAYCASTVOLUME_GLSL,
        SHADER_QUAD_VERT,
        SHADER_QUAD_FRAG,
        SHADER_CPLANE_VERT,
        SHADER_CPLANE_FRAG,
        SHADER_COLOR_VIZ_VERT,
        SHADER_COLOR_VIZ_FRAG,
        SHADER_OPA_VIZ_VERT,
        SHADER_OPA_VIZ_FRAG,
        SHADER_MARCHING_CUBE_GLSL,
        SHADER_MARCHING_CUBE_CLEAR_GLSL,
        SHADER_MC_VERT,
        SHADER_MC_FRAG,
        SHADER_END
    };
    enum ANDROID_SHADER_FILES{
        SHADER_AR_BACKGROUND_SCREEN_VERT = SHADER_END,
        SHADER_AR_BACKGROUND_SCREEN_FRAG,
        SHADER_AR_POINTCLOUD_VERT,
        SHADER_AR_POINTCLOUD_FRAG,
        SHADER_AR_PLANE_VERT,
        SHADER_AR_PLANE_FRAG,
        SHADER_NAIVE_2D_VERT,
        SHADER_CUT_PLANE_VERT,
        SHADER_CUT_PLANE_FRAG,
        SHADER_ANDROID_END,
        SHADER_ALL_END
    };
    enum TEX_IDS{
        BAKED_TEX_ID=0,
        SCREEN_QUAD_TEX_ID,
        BACK_GROUND_AR_ID,
        PLANE_AR_ID,
    };
    enum DRAW_OVERLAY_IDS{
        OVERLAY_GRAPH=0,
        OVERLAY_COLOR_BARS
    };
    enum TOUCH_TARGET{
        TOUCH_VOLUME=0,
        TOUCH_AR_BUTTON=1
    };
    enum ORGAN_IDS{
        ORGAN_BALDDER=0,
        ORGAN_KIDNEY,
        ORGAN_COLON,
        ORGAN_SPLEEN,
        ORGAN_ILEUM,
        ORGAN_AROTA,
        ORGAN_END
    };
    //UIs
    const float MOUSE_ROTATE_SENSITIVITY = 0.005f;
    const float MOUSE_SCALE_SENSITIVITY = 0.8f;
    const float MOUSE_PAN_SENSITIVITY = 1.2f;

    //TRS
    const DirectX::XMMATRIX DEFAULT_ROTATE = DirectX::XMMatrixIdentity();
    const DirectX::XMFLOAT3 DEFAULT_SCALE = DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f);
    const DirectX::XMFLOAT3 DEFAULT_POS = DirectX::XMFLOAT3(.0f, .0f, .0f);

    //color scheme
    constexpr char* COLOR_SCHEMES[3]={"COLOR_GRAYSCALE", "COLOR_HSV", "COLOR_BRIGHT"};

    struct Rect{
        float width;float height;
        float left;float top;
    };
    struct allConstantBuffer
    {
        DirectX::XMFLOAT4X4 model;
        DirectX::XMFLOAT4X4 view;
        DirectX::XMFLOAT4X4 projection;
        DirectX::XMFLOAT4 uCamPosInObjSpace;
    };
    struct ModelViewProjectionConstantBuffer
    {
        DirectX::XMFLOAT4X4 model;
        DirectX::XMFLOAT4X4 view;
        DirectX::XMFLOAT4X4 projection;
    };

    // Used to send per-vertex data to the vertex shader.
    struct VertexPositionColor
    {
        DirectX::XMFLOAT3 pos;
        DirectX::XMFLOAT3 color;
    };
    // Used to send per-vertex data to the vertex shader.
    struct VertexPosTex2d
    {
        DirectX::XMFLOAT2 pos;
        DirectX::XMFLOAT2 tex;
    };
}
#endif