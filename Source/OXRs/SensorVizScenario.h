#ifndef SENSOR_VIZ_SCENARIO_H
#define SENSOR_VIZ_SCENARIO_H
#include "Scenario.h"
#include <ResearchModeApi/ResearchModeApi.h>
#include <OXRs/OXRRenderer/SlateCameraRenderer.h>
#include <OXRs/OXRRenderer/RGBFrameProcessor.h>
#include <winrt/Windows.Foundation.h>
#include <Renderers/quadRenderer.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Media.Devices.Core.h>
#include <winrt/Windows.Media.Capture.Frames.h>
#include <winrt/Windows.Perception.Spatial.h>
#include <winrt/Windows.Graphics.Imaging.h>
#include <winrt/Windows.Storage.h>
#include <winrt/Windows.Perception.Spatial.h>
class SensorVizScenario : public Scenario {
public:
    SensorVizScenario(std::shared_ptr<DX::DeviceResources> const& deviceResources);
    virtual ~SensorVizScenario();

    void SetupReferenceFrame(winrt::Windows::Perception::Spatial::SpatialCoordinateSystem referenceFrame) {
      m_LFCameraRenderer->SetupReferenceFrame(referenceFrame);
      m_RFCameraRenderer->SetupReferenceFrame(referenceFrame);
    }
    void IntializeSensors();
    void IntializeScene();
    void Update(DX::StepTimer& timer);
    //void PositionHologram(winrt::Windows::UI::Input::Spatial::SpatialPointerPose const& pointerPose, const DX::StepTimer& timer);
    //void PositionHologramNoSmoothing(winrt::Windows::UI::Input::Spatial::SpatialPointerPose const& pointerPose);
    //winrt::Windows::Foundation::Numerics::float3 const& GetPosition()
    //{
        //return m_modelRenderers[0]->GetPosition();
    //}
    bool Render();
    void OnDeviceLost();
    void OnDeviceRestored();
    static void CamAccessOnComplete(ResearchModeSensorConsent consent);
    static void ImuAccessOnComplete(ResearchModeSensorConsent consent);
    void onSingle3DTouchDown(float x, float y, float z, int side);
protected:
    IResearchModeSensorDevice* m_pSensorDevice;
    IResearchModeSensorDeviceConsent* m_pSensorDeviceConsent;
    std::vector<ResearchModeSensorDescriptor> m_sensorDescriptors;
    
    IResearchModeSensor* m_pLFCameraSensor = nullptr;
    IResearchModeSensor* m_pRFCameraSensor = nullptr;

    std::shared_ptr<SlateCameraRenderer> m_LFCameraRenderer;
    std::shared_ptr<SlateCameraRenderer> m_RFCameraRenderer;

    glm::mat4 m_LFCameraPose, m_RFCameraPose;
    quadRenderer* m_rgbRender;
    std::shared_ptr <Texture> m_rgbTex;

    std::unique_ptr<RGBFrameProcessor> m_videoFrameProcessor = nullptr;
    winrt::Windows::Foundation::IAsyncAction m_videoFrameProcessorOperation = nullptr;


    winrt::Windows::Storage::StorageFolder m_archiveFolder = nullptr;
    int frame_num = 0;
    std::vector<const char*> m_save_frames;
    int m_size;

    winrt::Windows::Foundation::IAsyncAction InitFileSysAsync();

    winrt::Windows::Foundation::IAsyncAction InitializeVideoFrameProcessorAsync();
};
#endif // !SENSOR_VIZ_SCENARIO_H
