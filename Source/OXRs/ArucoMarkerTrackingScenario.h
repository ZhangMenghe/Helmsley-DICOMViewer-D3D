#ifndef ARUCO_MARKER_TRACKING_SCENARIO_H
#define ARUCO_MARKER_TRACKING_SCENARIO_H

#include "Scenario.h"
#include <ResearchModeApi/ResearchModeApi.h>
#include "OXRRenderer/SlateArucoTracker.h"
#include "OXRRenderer/SlateCameraRenderer.h"
struct Line {
    glm::vec3 P0, P1;
};
class ArucoMarkerTrackingScenario : public Scenario {
public:
    ArucoMarkerTrackingScenario(std::shared_ptr<DX::DeviceResources> const& deviceResources);
    virtual ~ArucoMarkerTrackingScenario();

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

protected:
    IResearchModeSensorDevice* m_pSensorDevice;
    IResearchModeSensorDeviceConsent* m_pSensorDeviceConsent;
    std::vector<ResearchModeSensorDescriptor> m_sensorDescriptors;

    IResearchModeSensor* m_pLFCameraSensor = nullptr;
    IResearchModeSensor* m_pRFCameraSensor = nullptr;

    std::shared_ptr<SlateCameraRenderer> m_LFCameraRenderer;
    std::shared_ptr<SlateCameraRenderer> m_RFCameraRenderer;

    std::shared_ptr <SlateArucoTracker> m_LFTracker;
    std::shared_ptr <SlateArucoTracker> m_RFTracker;

    DirectX::XMFLOAT4X4 m_LFCameraPose, m_RFCameraPose;


};
#endif
