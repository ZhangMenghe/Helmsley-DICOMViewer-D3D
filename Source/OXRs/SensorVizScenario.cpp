#include "pch.h"
#include "SensorVizScenario.h"
#include <vrController.h>

static ResearchModeSensorConsent camAccessCheck;
static HANDLE camConsentGiven;
static ResearchModeSensorConsent imuAccessCheck;
static HANDLE imuConsentGiven;

extern "C"
HMODULE LoadLibraryA(
    LPCSTR lpLibFileName
);

SensorVizScenario::SensorVizScenario(std::shared_ptr<DX::DeviceResources> const& deviceResources)
:Scenario(deviceResources){
    IntializeSensors();
    IntializeScene();
}

SensorVizScenario::~SensorVizScenario(){
    if (m_pLFCameraSensor)
    {
        m_pLFCameraSensor->Release();
    }
    if (m_pRFCameraSensor)
    {
        m_pRFCameraSensor->Release();
    }
    if (m_pLTSensor)
    {
        m_pLTSensor->Release();
    }
    if (m_pLTSensor)
    {
        m_pLTSensor->Release();
    }

    if (m_pSensorDevice)
    {
        m_pSensorDevice->EnableEyeSelection();
        m_pSensorDevice->Release();
    }
}

void SensorVizScenario::IntializeSensors() {
    HRESULT hr = S_OK;
    size_t sensorCount = 0;
    camConsentGiven = CreateEvent(nullptr, true, false, nullptr);
    imuConsentGiven = CreateEvent(nullptr, true, false, nullptr);

    HMODULE hrResearchMode = LoadLibraryA("ResearchModeAPI");
    if (hrResearchMode)
    {
        typedef HRESULT(__cdecl* PFN_CREATEPROVIDER) (IResearchModeSensorDevice** ppSensorDevice);
        PFN_CREATEPROVIDER pfnCreate = reinterpret_cast<PFN_CREATEPROVIDER>(GetProcAddress(hrResearchMode, "CreateResearchModeSensorDevice"));
        if (pfnCreate)
        {
            winrt::check_hresult(pfnCreate(&m_pSensorDevice));
        }
        else
        {
            winrt::check_hresult(E_INVALIDARG);
        }
    }

    winrt::check_hresult(m_pSensorDevice->QueryInterface(IID_PPV_ARGS(&m_pSensorDeviceConsent)));
    winrt::check_hresult(m_pSensorDeviceConsent->RequestCamAccessAsync(SensorVizScenario::CamAccessOnComplete));
    winrt::check_hresult(m_pSensorDeviceConsent->RequestIMUAccessAsync(SensorVizScenario::ImuAccessOnComplete));

    m_pSensorDevice->DisableEyeSelection();

    winrt::check_hresult(m_pSensorDevice->GetSensorCount(&sensorCount));
    m_sensorDescriptors.resize(sensorCount);

    winrt::check_hresult(m_pSensorDevice->GetSensorDescriptors(m_sensorDescriptors.data(), m_sensorDescriptors.size(), &sensorCount));

    for (auto sensorDescriptor : m_sensorDescriptors)
    {
        IResearchModeSensor* pSensor = nullptr;

        if (sensorDescriptor.sensorType == LEFT_FRONT)
        {
            winrt::check_hresult(m_pSensorDevice->GetSensor(sensorDescriptor.sensorType, &m_pLFCameraSensor));
        }

        if (sensorDescriptor.sensorType == RIGHT_FRONT)
        {
            winrt::check_hresult(m_pSensorDevice->GetSensor(sensorDescriptor.sensorType, &m_pRFCameraSensor));
        }

        // Long throw and AHAT modes can not be used at the same time.
#define DEPTH_USE_LONG_THROW

#ifdef DEPTH_USE_LONG_THROW
        if (sensorDescriptor.sensorType == DEPTH_LONG_THROW)
        {
            winrt::check_hresult(m_pSensorDevice->GetSensor(sensorDescriptor.sensorType, &m_pLTSensor));
        }
#else
        if (sensorDescriptor.sensorType == DEPTH_AHAT)
        {
            winrt::check_hresult(m_pSensorDevice->GetSensor(sensorDescriptor.sensorType, &m_pAHATSensor));
        }
#endif
        if (sensorDescriptor.sensorType == IMU_ACCEL)
        {
            winrt::check_hresult(m_pSensorDevice->GetSensor(sensorDescriptor.sensorType, &m_pAccelSensor));
        }
        if (sensorDescriptor.sensorType == IMU_GYRO)
        {
            winrt::check_hresult(m_pSensorDevice->GetSensor(sensorDescriptor.sensorType, &m_pGyroSensor));
        }
        if (sensorDescriptor.sensorType == IMU_MAG)
        {
            winrt::check_hresult(m_pSensorDevice->GetSensor(sensorDescriptor.sensorType, &m_pMagSensor));
        }
    }
}

void SensorVizScenario::IntializeScene() {
    m_LFCameraRenderer = std::make_shared<SlateCameraRenderer>(m_deviceResources->GetD3DDevice(), m_pLFCameraSensor, camConsentGiven, &camAccessCheck);
}
void SensorVizScenario::Update(DX::StepTimer& timer) {

}
void SensorVizScenario::Render() {
    auto model_mat = vrController::instance()->getFrameModelMat();
    m_LFCameraRenderer->Draw(m_deviceResources->GetD3DDeviceContext(), model_mat);
}
void SensorVizScenario::OnDeviceLost() {

}
void SensorVizScenario::OnDeviceRestored() {

}
void SensorVizScenario::CamAccessOnComplete(ResearchModeSensorConsent consent) {
    camAccessCheck = consent;
    SetEvent(camConsentGiven);
}
void SensorVizScenario::ImuAccessOnComplete(ResearchModeSensorConsent consent) {
    imuAccessCheck = consent;
    SetEvent(imuConsentGiven);
}