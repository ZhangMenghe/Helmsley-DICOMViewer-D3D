#include "pch.h"
#include "HLSensorManager.h"
#include <OXRs/XrUtility/XrMath.h>
#include <OXRs/XrUtility/XrError.h>

static ResearchModeSensorConsent camAccessCheck;
static HANDLE camConsentGiven;

extern "C"
HMODULE LoadLibraryA(
    LPCSTR lpLibFileName
);

HLSensorManager::HLSensorManager(const std::vector<ResearchModeSensorType>& kEnabledSensorTypes)
:m_kEnabledSensorTypes(kEnabledSensorTypes) {
	size_t sensorCount = 0;
	camConsentGiven = CreateEvent(nullptr, true, false, nullptr);

	// Load Research Mode library
	HMODULE hrResearchMode = LoadLibraryA("ResearchModeAPI");
	if (hrResearchMode)
	{
		typedef HRESULT(__cdecl* PFN_CREATEPROVIDER) (IResearchModeSensorDevice** ppSensorDevice);
		PFN_CREATEPROVIDER pfnCreate = reinterpret_cast<PFN_CREATEPROVIDER>(GetProcAddress(hrResearchMode, "CreateResearchModeSensorDevice"));
		if (pfnCreate)
		{
			winrt::check_hresult(pfnCreate(&m_pSensorDevice));
		}
	}

	// Manage Sensor Consent
	winrt::check_hresult(m_pSensorDevice->QueryInterface(IID_PPV_ARGS(&m_pSensorDeviceConsent)));
	winrt::check_hresult(m_pSensorDeviceConsent->RequestCamAccessAsync(CamAccessOnComplete));

	m_pSensorDevice->DisableEyeSelection();

	m_pSensorDevice->GetSensorCount(&sensorCount);
	m_sensorDescriptors.resize(sensorCount);

	m_pSensorDevice->GetSensorDescriptors(m_sensorDescriptors.data(), m_sensorDescriptors.size(), &sensorCount);

	for (auto& sensorDescriptor : m_sensorDescriptors)
	{
		if (sensorDescriptor.sensorType == LEFT_FRONT)
		{
			if (std::find(m_kEnabledSensorTypes.begin(), m_kEnabledSensorTypes.end(), LEFT_FRONT) == m_kEnabledSensorTypes.end())
			{
				continue;
			}

			winrt::check_hresult(m_pSensorDevice->GetSensor(sensorDescriptor.sensorType, &m_pLFCameraSensor));
		}

		if (sensorDescriptor.sensorType == RIGHT_FRONT)
		{
			if (std::find(m_kEnabledSensorTypes.begin(), m_kEnabledSensorTypes.end(), RIGHT_FRONT) == m_kEnabledSensorTypes.end())
			{
				continue;
			}

			winrt::check_hresult(m_pSensorDevice->GetSensor(sensorDescriptor.sensorType, &m_pRFCameraSensor));
		}

		if (sensorDescriptor.sensorType == LEFT_LEFT)
		{
			if (std::find(m_kEnabledSensorTypes.begin(), m_kEnabledSensorTypes.end(), LEFT_LEFT) == m_kEnabledSensorTypes.end())
			{
				continue;
			}

			winrt::check_hresult(m_pSensorDevice->GetSensor(sensorDescriptor.sensorType, &m_pLLCameraSensor));
		}

		if (sensorDescriptor.sensorType == RIGHT_RIGHT)
		{
			if (std::find(m_kEnabledSensorTypes.begin(), m_kEnabledSensorTypes.end(), RIGHT_RIGHT) == m_kEnabledSensorTypes.end())
			{
				continue;
			}

			winrt::check_hresult(m_pSensorDevice->GetSensor(sensorDescriptor.sensorType, &m_pRRCameraSensor));
		}

		if (sensorDescriptor.sensorType == DEPTH_LONG_THROW)
		{
			if (std::find(m_kEnabledSensorTypes.begin(), m_kEnabledSensorTypes.end(), DEPTH_LONG_THROW) == m_kEnabledSensorTypes.end())
			{
				continue;
			}

			winrt::check_hresult(m_pSensorDevice->GetSensor(sensorDescriptor.sensorType, &m_pLTSensor));
		}

		if (sensorDescriptor.sensorType == DEPTH_AHAT)
		{
			if (std::find(m_kEnabledSensorTypes.begin(), m_kEnabledSensorTypes.end(), DEPTH_AHAT) == m_kEnabledSensorTypes.end())
			{
				continue;
			}
			winrt::check_hresult(m_pSensorDevice->GetSensor(sensorDescriptor.sensorType, &m_pAHATSensor));
		}
	}
}

HLSensorManager::~HLSensorManager() {
	if (m_pLFCameraSensor)
	{
		m_pLFCameraSensor->Release();
	}
	if (m_pRFCameraSensor)
	{
		m_pRFCameraSensor->Release();
	}
	if (m_pLLCameraSensor)
	{
		m_pLLCameraSensor->Release();
	}
	if (m_pRRCameraSensor)
	{
		m_pRRCameraSensor->Release();
	}
	if (m_pLTSensor)
	{
		m_pLTSensor->Release();
	}
	if (m_pAHATSensor)
	{
		m_pAHATSensor->Release();
	}

	if (m_pSensorDevice)
	{
		m_pSensorDevice->EnableEyeSelection();
		m_pSensorDevice->Release();
	}

	if (m_pSensorDeviceConsent)
	{
		m_pSensorDeviceConsent->Release();
	}
}

void HLSensorManager::InitializeXRSpaces(const XrInstance& instance, const XrSession& session) {
	GUID rigguid;
	GetRigNodeId(rigguid);

	XrSpatialGraphNodeSpaceCreateInfoMSFT createInfo;
	createInfo.type = XR_TYPE_SPATIAL_GRAPH_NODE_SPACE_CREATE_INFO_MSFT;
	createInfo.next = NULL;
	createInfo.nodeType = XR_SPATIAL_GRAPH_NODE_TYPE_DYNAMIC_MSFT;
	memset(createInfo.nodeId, 0, 16);
	memcpy(createInfo.nodeId, &(rigguid.Data1), 16);

	createInfo.pose = xr::math::Pose::Identity();

	xrGetInstanceProcAddr(instance, "xrCreateSpatialGraphNodeSpaceMSFT",
		reinterpret_cast<PFN_xrVoidFunction*>(&ext_xrCreateSpatialGraphNodeSpaceMSFT));
	CHECK_XRCMD(ext_xrCreateSpatialGraphNodeSpaceMSFT(session, &createInfo, m_sensorSpace.Put()));
}
RMCameraReader* HLSensorManager::createRMCameraReader(ResearchModeSensorType sensor_type) {
	if (std::find(m_kEnabledSensorTypes.begin(), m_kEnabledSensorTypes.end(), sensor_type) == m_kEnabledSensorTypes.end())
		return nullptr;

	if (sensor_type == LEFT_FRONT) {
		GUID guid;
		GetRigNodeId(guid);
		return new RMCameraReader(m_pLFCameraSensor, camConsentGiven, &camAccessCheck, guid);
	}
	return nullptr;
}
DirectX::XMMATRIX HLSensorManager::getSensorMatrixAtTime(xr::SpaceHandle& app_space, uint64_t time){
	// Locate the rigNode space in the app space
	XrSpaceLocation rigLocation{ XR_TYPE_SPACE_LOCATION };
	CHECK_XRCMD(xrLocateSpace(m_sensorSpace.Get(), app_space.Get(), time, &rigLocation));

	if (!xr::math::Pose::IsPoseValid(rigLocation)) {
		return DirectX::XMMatrixIdentity();
	}

	return xr::math::LoadXrPose(rigLocation.pose);
}
void HLSensorManager::GetRigNodeId(GUID& outGuid) const{
    IResearchModeSensorDevicePerception* pSensorDevicePerception;
    winrt::check_hresult(m_pSensorDevice->QueryInterface(IID_PPV_ARGS(&pSensorDevicePerception)));
    winrt::check_hresult(pSensorDevicePerception->GetRigNodeId(&outGuid));
    pSensorDevicePerception->Release();
}

void HLSensorManager::Update(DX::StepTimer& timer) {
    //m_LFCameraRenderer->Update(m_context->DeviceContext.get());
    //m_LFCameraRenderer->Update(m_deviceResources->GetD3DDeviceContext());
}
void HLSensorManager::CamAccessOnComplete(ResearchModeSensorConsent consent) {
	camAccessCheck = consent;
	SetEvent(camConsentGiven);
}