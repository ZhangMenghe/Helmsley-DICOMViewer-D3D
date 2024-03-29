﻿#include "pch.h"
#include "SensorVizScenario.h"
#include <vrController.h>
#include <Utils/TypeConvertUtils.h>
#include <fstream>

static ResearchModeSensorConsent camAccessCheck;
static HANDLE camConsentGiven;
static ResearchModeSensorConsent imuAccessCheck;
static HANDLE imuConsentGiven;

extern "C"
HMODULE LoadLibraryA(
    LPCSTR lpLibFileName
);

SensorVizScenario::SensorVizScenario(const std::shared_ptr<xr::XrContext>& context)
:Scenario(context){
    IntializeSensors();
    IntializeScene();

    //m_videoFrameProcessorOperation = InitializeVideoFrameProcessorAsync();
    InitFileSysAsync();
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

    if (m_pSensorDevice)
    {
        m_pSensorDevice->EnableEyeSelection();
        m_pSensorDevice->Release();
    }
}
winrt::Windows::Foundation::IAsyncAction SensorVizScenario::InitFileSysAsync(){
    using namespace winrt::Windows::Storage;

    winrt::Windows::Storage::StorageFolder localFolder = ApplicationData::Current().LocalFolder();
    m_archiveFolder = co_await localFolder.CreateFolderAsync(
        L"FrameCaptured",
        CreationCollisionOption::ReplaceExisting);
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
    }
}

void SensorVizScenario::IntializeScene() {
    IResearchModeSensorDevicePerception* pSensorDevicePerception;
    GUID guid;
    HRESULT hr = S_OK;
    hr = m_pSensorDevice->QueryInterface(IID_PPV_ARGS(&pSensorDevicePerception));
    hr = pSensorDevicePerception->GetRigNodeId(&guid);

    m_LFCameraRenderer = std::make_shared<SlateCameraRenderer>(m_context->Device.get(), m_pLFCameraSensor, camConsentGiven, &camAccessCheck);
    m_LFCameraRenderer->setPosition(glm::vec3(-0.2, .0, .0));
    m_LFCameraRenderer->SetGUID(guid);


    m_RFCameraRenderer = std::make_shared<SlateCameraRenderer>(m_context->Device.get(), m_pRFCameraSensor, camConsentGiven, &camAccessCheck);
    m_RFCameraRenderer->setPosition(glm::vec3(0.2, .0, .0));
    m_RFCameraRenderer->SetGUID(guid);

    D3D11_TEXTURE2D_DESC texDesc;
    texDesc.Width = 760;
    texDesc.Height = 428;
    texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    texDesc.Usage = D3D11_USAGE_DEFAULT;
    texDesc.MipLevels = 1;
    texDesc.ArraySize = 1;
    texDesc.SampleDesc.Count = 1;
    texDesc.SampleDesc.Quality = 0;
    texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    texDesc.CPUAccessFlags = 0;
    texDesc.MiscFlags = D3D11_RESOURCE_MISC_SHARED;

    D3D11_RENDER_TARGET_VIEW_DESC view_desc{
    texDesc.Format,
    D3D11_RTV_DIMENSION_TEXTURE2D,
    };
    view_desc.Texture2D.MipSlice = 0;
    m_rgbTex = std::make_shared<Texture>();
    m_rgbTex->Initialize(m_context->Device.get(), texDesc, view_desc);

    m_rgbRender = new quadRenderer(m_context->Device.get());
    m_rgbRender->setTexture(m_rgbTex.get());
}
void SensorVizScenario::Update(DX::StepTimer& timer) {
    //m_LFCameraRenderer->Update(m_deviceResources->GetD3DDeviceContext());
}
bool SensorVizScenario::Render() {
    //auto model_mat = vrController::instance()->getFrameModelMat();
    if (m_LFCameraRenderer->Update(m_context->DeviceContext.get()))
        m_RFCameraRenderer->UpdateExtrinsicsMatrix();
    else
        return false;
    return true;
    //m_LFCameraRenderer->Draw(m_deviceResources->GetD3DDeviceContext(), model_mat);
    //m_RFCameraRenderer->Draw(m_deviceResources->GetD3DDeviceContext(), model_mat);
    //m_rgbRender->Draw(m_deviceResources->GetD3DDeviceContext(), mat42xmmatrix(model_mat));
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
winrt::Windows::Foundation::IAsyncAction SensorVizScenario::InitializeVideoFrameProcessorAsync()
{
    if (m_videoFrameProcessorOperation &&
        m_videoFrameProcessorOperation.Status() == winrt::Windows::Foundation::AsyncStatus::Completed)
    {
        return;
    }

    m_videoFrameProcessor = std::make_unique<RGBFrameProcessor>(m_context);
    m_videoFrameProcessor->setTargetTexture(m_rgbTex);
    if (!m_videoFrameProcessor.get())
    {
        throw winrt::hresult(E_POINTER);
    }

    co_await m_videoFrameProcessor->InitializeAsync();
}
void SensorVizScenario::onSingle3DTouchDown(float x, float y, float z, int side) {
    //save current frame
    //auto frame = 

    const char* data;
    int size;
    m_LFCameraRenderer->getLatestFrame(data, size);
    m_size = size;
    char* sdata = new char[size];
    memcpy(sdata, data, sizeof(BYTE) * size);
    m_save_frames.push_back(sdata);
    if (m_save_frames.size() > 20) {
        std::wstring path(m_archiveFolder.Path().data());
        std::string test_fn = "\\chess";

        path += std::wstring(test_fn.begin(), test_fn.end());
        std::ofstream file(path, std::ios::out | std::ios::binary | std::ofstream::app);
        if (!file)
            return;
        for(auto mdata : m_save_frames)
            file.write(mdata, m_size);
        file.close();
    }
}
