#include "pch.h"
#include "ArucoMarkerTrackingScenario.h"
#include <vrController.h>
#include <opencv2/calib3d.hpp>
#include <glm/gtc/type_ptr.hpp>
static ResearchModeSensorConsent camAccessCheck;
static HANDLE camConsentGiven;
static ResearchModeSensorConsent imuAccessCheck;
static HANDLE imuConsentGiven;

extern "C"
HMODULE LoadLibraryA(
    LPCSTR lpLibFileName
);

ArucoMarkerTrackingScenario::ArucoMarkerTrackingScenario(const std::shared_ptr<xr::XrContext>& context)
:Scenario(context){
    IntializeSensors();
    IntializeScene();

}

ArucoMarkerTrackingScenario::~ArucoMarkerTrackingScenario(){
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

void ArucoMarkerTrackingScenario::IntializeSensors() {
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
    winrt::check_hresult(m_pSensorDeviceConsent->RequestCamAccessAsync(ArucoMarkerTrackingScenario::CamAccessOnComplete));
    winrt::check_hresult(m_pSensorDeviceConsent->RequestIMUAccessAsync(ArucoMarkerTrackingScenario::ImuAccessOnComplete));

    m_pSensorDevice->DisableEyeSelection();

    winrt::check_hresult(m_pSensorDevice->GetSensorCount(&sensorCount));
    m_sensorDescriptors.resize(sensorCount);

    winrt::check_hresult(m_pSensorDevice->GetSensorDescriptors(m_sensorDescriptors.data(), m_sensorDescriptors.size(), &sensorCount));

    for (auto sensorDescriptor : m_sensorDescriptors){
        IResearchModeSensor* pSensor = nullptr;
        IResearchModeCameraSensor* pCameraSensor = nullptr;

        if (sensorDescriptor.sensorType == LEFT_FRONT){
            winrt::check_hresult(m_pSensorDevice->GetSensor(sensorDescriptor.sensorType, &m_pLFCameraSensor));
            winrt::check_hresult(m_pLFCameraSensor->QueryInterface(IID_PPV_ARGS(&pCameraSensor)));
            winrt::check_hresult(pCameraSensor->GetCameraExtrinsicsMatrix(&m_LFCameraPose));
        }

        if (sensorDescriptor.sensorType == RIGHT_FRONT){
            winrt::check_hresult(m_pSensorDevice->GetSensor(sensorDescriptor.sensorType, &m_pRFCameraSensor));
            winrt::check_hresult(m_pRFCameraSensor->QueryInterface(IID_PPV_ARGS(&pCameraSensor)));
            winrt::check_hresult(pCameraSensor->GetCameraExtrinsicsMatrix(&m_RFCameraPose));
        }
    }
}

void ArucoMarkerTrackingScenario::IntializeScene() {
    m_LFCameraRenderer = std::make_shared<SlateCameraRenderer>(m_context->Device.get(), m_pLFCameraSensor, camConsentGiven, &camAccessCheck);
    //m_RFCameraRenderer = std::make_shared<SlateCameraRenderer>(m_deviceResources->GetD3DDevice(), m_pRFCameraSensor, camConsentGiven, &camAccessCheck);

    m_LFTracker = std::make_shared<SlateArucoTracker>();
    m_LFTracker->StartCVProcessing(0xff);
    m_LFCameraRenderer->SetFrameCallBack(SlateArucoTracker::FrameReadyCallback, m_LFTracker.get());

    //m_RFTracker = std::make_shared<SlateArucoTracker>(m_deviceResources);
    //m_RFTracker->StartCVProcessing(0xff);
    //m_RFCameraRenderer->SetFrameCallBack(SlateArucoTracker::FrameReadyCallback, m_RFTracker.get());
}
glm::vec3 get_3d_line_intersection(Line L1, Line L2)
{
    glm::vec3   u = L1.P1 - L1.P0;
    glm::vec3    v = L2.P1 - L2.P0;
    glm::vec3    w = L1.P0 - L2.P0;
    float    a = dot(u, u);         // always >= 0
    float    b = dot(u, v);
    float    c = dot(v, v);         // always >= 0
    float    d = dot(u, w);
    float    e = dot(v, w);
    float    D = a * c - b * b;        // always >= 0
    float    sc, tc;

    // compute the line parameters of the two closest points
    if (D < 0.00000001f) {          // the lines are almost parallel
        sc = 0.0;
        tc = (b > c ? d / b : e / c);    // use the largest denominator
    }
    else {
        sc = (b * e - c * d) / D;
        tc = (a * e - b * d) / D;
    }

    // get the difference of the two closest points
    glm::vec3   dP = w + (sc * u) - (tc * v);  // =  L1(sc) - L2(tc)

    return w + (sc * u);
    //return glm::normalize(dP);   // return the closest distance
}
void ArucoMarkerTrackingScenario::Update(DX::StepTimer& timer) {
    ResearchModeSensorTimestamp timeStamp;

    cv::Vec3d rvec, tvec;
    if (!m_LFTracker->GetFirstTransformation(rvec, tvec)) return;
    cv::Mat R;
    Rodrigues(rvec, R);

    glm::mat4 rot_mat(1.0f);

    for (int i = 0; i < 3; i++) {
        const float* Ri = R.ptr<float>(i);
        for (int j = 0; j < 3; j++) {
            rot_mat[i][j] = Ri[j];
        }
    }
    
    glm::mat4 model_mat = 
        rot_mat *
        glm::translate(glm::mat4(1.0), glm::vec3(tvec[0], tvec[1], tvec[2]));

    vrController::instance()->setPosition(model_mat);
    /*float uv_left[2], uv_right[2];
    float xy_left[2], xy_right[2];

    IResearchModeCameraSensor* pCameraSensor_left = nullptr, *pCameraSensor_right=nullptr;

    if (!m_LFTracker->GetFirstCenter(uv_left, uv_left + 1, &timeStamp) || !m_RFTracker->GetFirstCenter(uv_right, uv_right + 1, &timeStamp)) return;
    

    winrt::check_hresult(m_pLFCameraSensor->QueryInterface(IID_PPV_ARGS(&pCameraSensor_left)));
    pCameraSensor_left->MapImagePointToCameraUnitPlane(uv_left, xy_left);


    winrt::check_hresult(m_pRFCameraSensor->QueryInterface(IID_PPV_ARGS(&pCameraSensor_right)));
    pCameraSensor_right->MapImagePointToCameraUnitPlane(uv_right, xy_right);


    DirectX::XMMATRIX cameraNodeToRigPoseInverted;
    DirectX::XMMATRIX cameraNodeToRigPose;
    DirectX::XMVECTOR det;

    //cameraNodeToRigPose
    winrt::check_hresult(pCameraSensor_right->GetCameraExtrinsicsMatrix(&m_RFCameraPose));
    cameraNodeToRigPose = DirectX::XMLoadFloat4x4(&m_RFCameraPose);
    det = XMMatrixDeterminant(cameraNodeToRigPose);
    cameraNodeToRigPoseInverted = DirectX::XMMatrixInverse(&det, cameraNodeToRigPose);

    DirectX::XMFLOAT3 pwr0 = DirectX::XMFLOAT3(xy_right[0], xy_right[1], .0f);
    DirectX::XMFLOAT3 pwr1 = DirectX::XMFLOAT3(xy_right[0], xy_right[1], -1.0f);

    auto pwr_world0 = DirectX::XMVector3Transform(DirectX::XMLoadFloat3(&pwr0), cameraNodeToRigPoseInverted);
    auto pwr_world1 = DirectX::XMVector3Transform(DirectX::XMLoadFloat3(&pwr1), cameraNodeToRigPoseInverted);

    DirectX::XMStoreFloat3(&pwr0, pwr_world0);
    DirectX::XMStoreFloat3(&pwr1, pwr_world1);


    Line lf, lr;
    lf.P0 = glm::vec3(xy_left[0], xy_left[1], .0f);
    lf.P1 = glm::vec3(xy_left[0], xy_left[1], -1.0f);

    lr.P0 = glm::vec3(pwr0.x, pwr0.y, pwr0.z);
    lr.P1 = glm::vec3(pwr1.x, pwr1.y, pwr1.z);

    auto center_pos = get_3d_line_intersection(lf, lr);
    DirectX::XMFLOAT3 center_pos_dx = DirectX::XMFLOAT3(center_pos.x, center_pos.y, center_pos.z);

    auto inv_mat = DirectX::XMMatrixInverse(nullptr, Manager::instance()->camera->getViewMat());
    auto pos_world = DirectX::XMVector3Transform(DirectX::XMLoadFloat3(&center_pos_dx), inv_mat);
    DirectX::XMFLOAT3 pw;
    DirectX::XMStoreFloat3(&pw, pos_world);

    vrController::instance()->setPosition(glm::vec3(pw.x, pw.y, -1.0f));*/
}
bool ArucoMarkerTrackingScenario::Render() {
    //auto model_mat = vrController::instance()->getFrameModelMat();
    return true;
}
void ArucoMarkerTrackingScenario::OnDeviceLost() {

} 
void ArucoMarkerTrackingScenario::OnDeviceRestored() {

}
void ArucoMarkerTrackingScenario::CamAccessOnComplete(ResearchModeSensorConsent consent) {
    camAccessCheck = consent;
    SetEvent(camConsentGiven);
}
void ArucoMarkerTrackingScenario::ImuAccessOnComplete(ResearchModeSensorConsent consent) {
    imuAccessCheck = consent;
    SetEvent(imuConsentGiven);
}