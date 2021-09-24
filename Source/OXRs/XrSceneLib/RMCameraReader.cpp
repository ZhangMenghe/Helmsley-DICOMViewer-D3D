#include "pch.h"
#include "RMCameraReader.h"
#include <sstream>

//using namespace winrt::Windows::Perception;
//using namespace winrt::Windows::Perception::Spatial;
//using namespace winrt::Windows::Foundation::Numerics;
//using namespace winrt::Windows::Storage;


RMCameraReader::RMCameraReader(IResearchModeSensor* pLLSensor, HANDLE camConsentGiven, ResearchModeSensorConsent* camAccessConsent, const GUID& guid) {
    m_pRMSensor = pLLSensor;
    m_pRMSensor->AddRef();
    m_pSensorFrame = nullptr;
    m_frameCallback = nullptr;

    m_pCameraUpdateThread = new std::thread(CameraUpdateThread, this, camConsentGiven, camAccessConsent);
}
RMCameraReader::~RMCameraReader(){
    m_fExit = true;
    m_pCameraUpdateThread->join();

    if (m_pRMSensor){
        m_pRMSensor->CloseStream();
        m_pRMSensor->Release();
    }
}
void RMCameraReader::SetFrameCallBack(std::function<void(IResearchModeSensorFrame*, PVOID frameCtx)> frameCallback, PVOID frameCtx){
    m_frameCallback = frameCallback;
    m_frameCtx = frameCtx;
}
void RMCameraReader::CameraUpdateThread(RMCameraReader* pCameraReader, HANDLE camConsentGiven, ResearchModeSensorConsent* camAccessConsent){
    HRESULT hr = S_OK;
    LARGE_INTEGER qpf;
    uint64_t lastQpcNow = 0;
    QueryPerformanceFrequency(&qpf);

    if (camConsentGiven != nullptr)
    {
        DWORD waitResult = WaitForSingleObject(camConsentGiven, INFINITE);

        if (waitResult == WAIT_OBJECT_0)
        {
            switch (*camAccessConsent)
            {
            case ResearchModeSensorConsent::Allowed:
                OutputDebugString(L"Access is granted");
                break;
            case ResearchModeSensorConsent::DeniedBySystem:
                OutputDebugString(L"Access is denied by the system");
                hr = E_ACCESSDENIED;
                break;
            case ResearchModeSensorConsent::DeniedByUser:
                OutputDebugString(L"Access is denied by the user");
                hr = E_ACCESSDENIED;
                break;
            case ResearchModeSensorConsent::NotDeclaredByApp:
                OutputDebugString(L"Capability is not declared in the app manifest");
                hr = E_ACCESSDENIED;
                break;
            case ResearchModeSensorConsent::UserPromptRequired:
                OutputDebugString(L"Capability user prompt required");
                hr = E_ACCESSDENIED;
                break;
            default:
                OutputDebugString(L"Access is denied by the system");
                hr = E_ACCESSDENIED;
                break;
            }
        }
        else
        {
            hr = E_UNEXPECTED;
        }
    }

    if (FAILED(hr))
    {
        return;
    }
    hr = pCameraReader->m_pRMSensor->OpenStream();

    if (FAILED(hr))
    {
        pCameraReader->m_pRMSensor->Release();
        pCameraReader->m_pRMSensor = nullptr;
    }

    while (!pCameraReader->m_fExit && pCameraReader->m_pRMSensor)
    {
        static int gFrameCount = 0;
        HRESULT hr = S_OK;
        IResearchModeSensorFrame* pSensorFrame = nullptr;

        hr = pCameraReader->m_pRMSensor->GetNextBuffer(&pSensorFrame);

        if (SUCCEEDED(hr))
        {
            LARGE_INTEGER qpcNow;
            uint64_t uqpcNow;
            QueryPerformanceCounter(&qpcNow);
            uqpcNow = qpcNow.QuadPart;
            ResearchModeSensorTimestamp timeStamp;
            pSensorFrame->GetTimeStamp(&timeStamp);
            
            if (lastQpcNow != 0)
            {
                pCameraReader->m_refreshTimeInMilliseconds =
                    (1000 *
                        (uqpcNow - lastQpcNow)) /
                    qpf.QuadPart;
            }
            if (pCameraReader->m_lastHostTicks != 0)
            {
                pCameraReader->m_sensorRefreshTime = timeStamp.HostTicks - pCameraReader->m_lastHostTicks;
            }

            std::lock_guard<std::mutex> guard(pCameraReader->m_sensorFrameMutex);
            if (pCameraReader->m_frameCallback)
            {
                pCameraReader->m_frameCallback(pSensorFrame, pCameraReader->m_frameCtx);
            }

            if (pCameraReader->m_pSensorFrame)
            {
                pCameraReader->m_pSensorFrame->Release();
            }
            pCameraReader->m_pSensorFrame = pSensorFrame;
        }
    }

    if (pCameraReader->m_pRMSensor)
    {
        pCameraReader->m_pRMSensor->CloseStream();
    }
}