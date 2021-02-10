#ifndef SENSOR_VIZ_SCENARIO_H
#define SENSOR_VIZ_SCENARIO_H
#include "Scenario.h"
#include <ResearchModeApi/ResearchModeApi.h>
#include <OXRs/OXRRenderer/SlateCameraRenderer.h>

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
    IResearchModeSensor* m_pLTSensor = nullptr;
    IResearchModeSensor* m_pAHATSensor = nullptr;
    IResearchModeSensor* m_pAccelSensor = nullptr;
    IResearchModeSensor* m_pGyroSensor = nullptr;
    IResearchModeSensor* m_pMagSensor = nullptr;

    std::shared_ptr<SlateCameraRenderer> m_LFCameraRenderer;
    std::shared_ptr<SlateCameraRenderer> m_LTCameraRenderer;
};
#endif // !SENSOR_VIZ_SCENARIO_H
