#ifndef CONSTANTS_H
#define CONSTANTS_H
#include "pch.h"
#include <string>

namespace dvr
{
    enum PARAM_BOOL
    {
        CHECK_RAYCAST = 0,
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
    enum PARAM_TUNE
    {
        TUNE_OVERALL = 0,
        TUNE_LOWEST,
        TUNE_WIDTHBOTTOM,
        TUNE_WIDTHTOP,
        TUNE_CENTER,
        TUNE_END
    };
    enum PARAM_CUT_ID
    {
        CUT_CUTTING_PLANE = 0,
        CUT_TRAVERSAL
    };
    enum PARAM_DUAL
    {
        CONTRAST_LIMIT = 0,
        DUAL_END
    };
    enum AR_REQUEST
    {
        PLACE_VOLUME = 0,
        PLACE_ANCHOR
    };
    enum PARAM_RENDER_TUNE
    {
        //        RENDER_CONTRAST_LEVEL=0,
        RENDER_CONTRAST_LOW = 0,
        RENDER_CONTRAST_HIGH,
        RENDER_BRIGHTNESS,
        PARAM_RENDER_TUNE_END
    };
    enum SHADER_FILES
    {
        SHADER_TEXTUREVOLUME_VERT = 0,
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
    enum ANDROID_SHADER_FILES
    {
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
    enum TEX_IDS
    {
        BAKED_TEX_ID = 0,
        SCREEN_QUAD_TEX_ID,
        BACK_GROUND_AR_ID,
        PLANE_AR_ID,
    };
    enum DRAW_OVERLAY_IDS
    {
        OVERLAY_GRAPH = 0,
        OVERLAY_COLOR_BARS
    };
    enum TOUCH_TARGET
    {
        TOUCH_VOLUME = 0,
        TOUCH_AR_BUTTON = 1
    };
    enum ORGAN_IDS
    {
        ORGAN_BALDDER = 0,
        ORGAN_KIDNEY,
        ORGAN_COLON,
        ORGAN_SPLEEN,
        ORGAN_ILEUM,
        ORGAN_AROTA,
        ORGAN_END
    };
    enum INPUT_LAYOUT_IDS
    {
        INPUT_POS_TEX_2D = 0,
        INPUT_POS_3D,
        INPUT_LAYOUT_END
    };
    //UIs
    const float MOUSE_ROTATE_SENSITIVITY = 0.005f;
    const float MOUSE_SCALE_SENSITIVITY = 0.5f;
    const float MOUSE_PAN_SENSITIVITY = 1.2f;

    //setting
    const bool CONNECT_TO_SERVER = true;
    const bool LOAD_DATA_FROM_SERVER = false;
    static const ORGAN_IDS DEFAULT_TRAVERSAL_ORGAN = ORGAN_COLON;

    //Names
    const static std::string CACHE_FOLDER_NAME = "helmsley_cached", CONFIG_NAME = "pacs_local.txt";
    const static std::string ASSET_RESERVE_DS = "Larry_Smarr_2016", ASSET_RESERVE_VL = "series_23_Cor_LAVA_PRE-Amira";
    const static std::string DCM_FILE_NAME = "data", DCM_MASK_FILE_NAME = "mask", DCM_WMASK_FILE_NAME = "data_w_mask", DCM_CENTERLINE_FILE_NAME = "center_line";

    //TRS
    //const DirectX::XMFLOAT4X4 DEFAULT_ROTATE(
    //    1.0f, .0f,.0f,.0f,
    //    .0f, 1.0f, .0f, .0f,
    //    .0f, .0f, 1.0f, .0f,
    //    .0f, .0f, .0f, 1.0f);
    //const DirectX::XMFLOAT3 DEFAULT_SCALE = { 0.8f, 0.8f, 0.8f };
    //const DirectX::XMFLOAT3 DEFAULT_POS = { .0f, .0f, .0f };

    const glm::mat4 DEFAULT_ROTATE = glm::mat4(1.0f);
    const glm::vec3 DEFAULT_SCALE = glm::vec3(0.5f);
    const glm::vec3 DEFAULT_POS = glm::vec3(.0f, .0f, 0.0f);

    const float SCREEN_CLEAR_COLOR[4] = {
        0.f, 0.f, 0.f, 0.f};
    //color scheme
    constexpr const char *COLOR_SCHEMES[3] = {"COLOR_GRAYSCALE", "COLOR_HSV", "COLOR_BRIGHT"};

    struct Rect
    {
        float width;
        float height;
        float left;
        float top;
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
        DirectX::XMFLOAT4X4 uViewProjMat;
    };
    struct ColorConstantBuffer
    {
        DirectX::XMFLOAT4 u_color;
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
        DirectX::XMFLOAT3 pos;
        DirectX::XMFLOAT3 tex;
    };
    struct VertexPos3d
    {
        DirectX::XMFLOAT3 pos;
    };

    static const D3D11_INPUT_ELEMENT_DESC g_vinput_pos_tex_desc[] =
        {
            {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"TEXCOORD", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };
    static const D3D11_INPUT_ELEMENT_DESC g_vinput_pos_3d_desc[] =
        {
            {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };
} // namespace dvr
#endif