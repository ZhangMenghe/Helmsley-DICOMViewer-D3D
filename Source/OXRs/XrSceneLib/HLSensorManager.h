#ifndef HL_SENSOR_MANAGER_H
#define HL_SENSOR_MANAGER_H

#include <ResearchModeApi/ResearchModeApi.h>
#include <Common/StepTimer.h>
#include <OXRs/XrUtility/XrHandle.h>
#include "RMCameraReader.h"
class HLSensorManager {
public:
    HLSensorManager(const std::vector<ResearchModeSensorType>& kEnabledSensorTypes);
    virtual ~HLSensorManager();
    void InitializeXRSpaces(const XrInstance& instance, const XrSession& session);
    RMCameraReader* createRMCameraReader(ResearchModeSensorType sensor_type);

    DirectX::XMMATRIX getSensorMatrixAtTime(xr::SpaceHandle& app_space, uint64_t time);
    void Update(DX::StepTimer& timer);

    static void CamAccessOnComplete(ResearchModeSensorConsent consent);
private:
    const std::vector<ResearchModeSensorType>& m_kEnabledSensorTypes;

    IResearchModeSensorDevice* m_pSensorDevice;
    IResearchModeSensorDeviceConsent* m_pSensorDeviceConsent;
    std::vector<ResearchModeSensorDescriptor> m_sensorDescriptors;

    // Supported RM sensors
    IResearchModeSensor* m_pLFCameraSensor = nullptr;
    IResearchModeSensor* m_pRFCameraSensor = nullptr;
    IResearchModeSensor* m_pLLCameraSensor = nullptr;
    IResearchModeSensor* m_pRRCameraSensor = nullptr;
    IResearchModeSensor* m_pLTSensor = nullptr;
    IResearchModeSensor* m_pAHATSensor = nullptr;

    xr::SpaceHandle m_sensorSpace;
    PFN_xrCreateSpatialGraphNodeSpaceMSFT   ext_xrCreateSpatialGraphNodeSpaceMSFT;

    void GetRigNodeId(GUID& outGuid) const;
};
#endif // !SENSOR_VIZ_SCENARIO_H
