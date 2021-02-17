#ifndef SENSOR_VIZ_SCENARIO_H
#define SENSOR_VIZ_SCENARIO_H
#include "Scenario.h"
#include <ResearchModeApi/ResearchModeApi.h>
#include <OXRs/OXRRenderer/SlateCameraRenderer.h>
#include <OXRs/OXRRenderer/RGBFrameProcessor.h>
#include <winrt/Windows.Foundation.h>
#include <Renderers/quadRenderer.h>

class SensorVizScenario : public Scenario {
public:
    SensorVizScenario(std::shared_ptr<DX::DeviceResources> const& deviceResources);
    virtual ~SensorVizScenario();

    void IntializeSensors();
    void IntializeScene();
    void Update(DX::StepTimer& timer);
    //void PositionHologram(winrt::Windows::UI::Input::Spatial::SpatialPointerPose const& pointerPose, const DX::StepTimer& timer);
    //void PositionHologramNoSmoothing(winrt::Windows::UI::Input::Spatial::SpatialPointerPose const& pointerPose);
    //winrt::Windows::Foundation::Numerics::float3 const& GetPosition()
    //{
        //return m_modelRenderers[0]->GetPosition();
    //}
    void Render();
    void OnDeviceLost();
    void OnDeviceRestored();
    static void CamAccessOnComplete(ResearchModeSensorConsent consent);
    static void ImuAccessOnComplete(ResearchModeSensorConsent consent);

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

    winrt::Windows::Foundation::IAsyncAction InitializeVideoFrameProcessorAsync();
};
#endif // !SENSOR_VIZ_SCENARIO_H
