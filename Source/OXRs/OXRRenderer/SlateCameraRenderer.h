#ifndef SLATE_CAMERA_RENDERER_H
#define SLATE_CAMERA_RENDERER_H

#include <Common/DeviceResources.h>
#include <Common/StepTimer.h>
#include <D3DPipeline/Texture.h>
#include <Renderers/baseRenderer.h>
#include <ResearchModeApi/ResearchModeApi.h>
#include <functional>
#include <mutex>
#include <opencv2/core.hpp>
#include <winrt/Windows.Perception.Spatial.h>
#include <winrt/Windows.Perception.Spatial.Preview.h>
using namespace winrt::Windows::Perception;
using namespace winrt::Windows::Perception::Spatial;
using namespace winrt::Windows::Perception::Spatial::Preview;

class SlateCameraRenderer:public baseRenderer {
public:
	SlateCameraRenderer(ID3D11Device* device);
    SlateCameraRenderer(ID3D11Device* device, IResearchModeSensor* pLLSensor, 
        HANDLE hasData, ResearchModeSensorConsent* pCamAccessConsent);

	bool Draw(ID3D11DeviceContext* context, glm::mat4 model_mat);
    bool Update(ID3D11DeviceContext* context);
    void UpdateExtrinsicsMatrix();
    void setPosition(glm::vec3 pos);
    void SetFrameCallBack(std::function<void(IResearchModeSensorFrame*, PVOID frameCtx)> frameCallback, PVOID frameCtx)
    {
        m_frameCallback = frameCallback;
        m_frameCtx = frameCtx;
    }
    void getLatestFrame(const char*& data, int& size) { data = (const char*)m_texture_data; size = m_tex_size; }
    bool GetFirstTransformation(cv::Vec3d& rvec, cv::Vec3d& tvec);
    void SetupReferenceFrame(winrt::Windows::Perception::Spatial::SpatialCoordinateSystem referenceFrame) {
      m_referenceFrame = referenceFrame;
    }

    void SetGUID(GUID guid);

protected:
	virtual void create_vertex_shader(ID3D11Device* device, const std::vector<byte>& fileData);
	virtual void create_fragment_shader(ID3D11Device* device, const std::vector<byte>& fileData);
private:
    IResearchModeSensor* m_pRMCameraSensor = nullptr;
    IResearchModeSensorFrame* m_pSensorFrame = nullptr;
    uint64_t m_refreshTimeInMilliseconds = 0;
    uint64_t m_sensorRefreshTime = 0;
    uint64_t m_lastHostTicks = 0;

    float m_slateWidth, m_slateHeight;

    std::function<void(IResearchModeSensorFrame*, PVOID frameCtx)> m_frameCallback;
    PVOID m_frameCtx;

    std::thread* m_pCameraUpdateThread;
    bool m_fExit = { false };

    std::mutex m_mutex;

    glm::mat4 cameraToWorld = glm::mat4(1.0f);

	dvr::ModelViewProjectionConstantBuffer m_constantBufferData;
	dvr::INPUT_LAYOUT_IDS m_input_layout_id;
    glm::mat4 m_quad_matrix = glm::mat4(1.0);
    
    const BYTE* m_texture_data;
    int m_tex_size;

    GUID m_guid;
    SpatialLocator locator = { nullptr };

    cv::Mat m_cameraMatrix, m_distCoeffs;
    const float m_inverse_[16] = {
                   1.0, -1.0, -1.0, 1.0,
                   1.0,-1.0,-1.0,1.0,
                   1.0,-1.0,-1.0,1.0,
                   1.0, -1.0, -1.0, 1.0 };
    std::vector<cv::Vec3d> m_rvecs, m_tvecs;
    glm::mat4 m_extrinsics_mat = glm::mat4(1.0);

    winrt::Windows::Perception::Spatial::SpatialCoordinateSystem m_referenceFrame = {nullptr};

    static void CameraUpdateThread(SlateCameraRenderer* pSlateCameraRenderer, HANDLE hasData, ResearchModeSensorConsent* pCamAccessConsent);
    bool update_cam_texture(ID3D11DeviceContext* context);
};
#endif // !SLATE_CAMERA_RENDERER_H

