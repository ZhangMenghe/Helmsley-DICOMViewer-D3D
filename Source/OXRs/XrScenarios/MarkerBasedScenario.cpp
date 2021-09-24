#include "pch.h"
#include "MarkerBasedScenario.h"
#include <vrController.h>
#include <Utils/TypeConvertUtils.h>
#include <OXRs/OXRManager.h>
#include <OXRs/XrSceneLib/HLSensorManager.h>
#include <OXRs/OXRRenderer/OpenCVFrameProcessing.h>
#include <Utils/CVMathUtils.h>
#include <glm/gtx/rotate_vector.hpp> 

#include <winrt/Windows.Perception.Spatial.h>
#include <winrt/Windows.Perception.Spatial.Preview.h>
using namespace winrt::Windows::Perception;
using namespace winrt::Windows::Perception::Spatial;
using namespace winrt::Windows::Perception::Spatial::Preview;
using namespace DirectX;
using namespace DX;

static bool m_initialized = false;

MarkerBasedScenario::MarkerBasedScenario(const std::shared_ptr<xr::XrContext>& context) 
	:Scenario(context) {
	if (kEnabledRMStreamTypes.size() > 0) 
		OXRManager::instance()->InitHLSensors(kEnabledRMStreamTypes);
	HLSensorManager* manager = OXRManager::instance()->getHLSensorManager();
	if (manager) {
		auto rm_reader = manager->createRMCameraReader(LEFT_FRONT);
		rm_reader->SetFrameCallBack(FrameReadyCallback, this);
		vrController::instance()->setUseSpaceMat(true);
	}
}

void MarkerBasedScenario::FrameReadyCallback(IResearchModeSensorFrame* pSensorFrame, PVOID frameCtx) {
	MarkerBasedScenario* scenario = (MarkerBasedScenario*)frameCtx;
	if (!m_initialized) {
		scenario->m_cameraMatrix = (cv::Mat1d(3, 3) << 370.06088626, 0, 322.86274468, 0, 374.89346096, 232.52886492, 0, 0, 1);
		scenario->m_distCoeffs = (cv::Mat1d(1, 5) << -0.04411806, 0.13393567, -0.00460908, 0.00304738, -0.09357143);
		m_initialized = true;
	}

	//ResearchModeSensorResolution resolution;
	//IResearchModeSensorVLCFrame* pVLCFrame = nullptr;
	//const BYTE* pImage = nullptr;
	//if (m_texture_data != nullptr) { m_texture_data = nullptr; }

	//size_t outBufferCount = 0;
	//m_pSensorFrame->GetResolution(&resolution);

	//DX::ThrowIfFailed();
	//if (FAILED(m_pSensorFrame->QueryInterface(IID_PPV_ARGS(&pVLCFrame)))) return;
	//pVLCFrame->GetBuffer(&m_texture_data, &outBufferCount);

	ResearchModeSensorTimestamp timeStamp;
	pSensorFrame->GetTimeStamp(&timeStamp);
	//HRESULT hr = S_OK;

	//if (m_pSensorFrame)
	//{
	//	m_pSensorFrame->Release();
	//}

	//if (!m_referenceFrame) {
	//	return;
	//}


	//auto camLocator = SpatialLocator::GetDefault();

	//LARGE_INTEGER qpcNow;
	//QueryPerformanceCounter(&qpcNow);
	//Calendar c;
	//c.SetToNow();

	auto timestamp = PerceptionTimestampHelper::FromSystemRelativeTargetTime(winrt::Windows::Foundation::TimeSpan{ timeStamp.HostTicks });//FromHistoricalTargetTime(c.GetDateTime());//TimeSpanFromQpcTicks(qpcNow.QuadPart));//winrt::Windows::Foundation::TimeSpan{ timeStamp.HostTicks });
	//auto coordinateSystem = m_referenceFrame;//m_referenceFrame.CoordinateSystem();//GetStationaryCoordinateSystemAtTimestamp(timestamp);
	// Get position infos
	//auto location = locator.TryLocateAtTimestamp(timestamp, m_referenceFrame);
	//auto camLocation = camLocator.TryLocateAtTimestamp(timestamp, m_referenceFrame);
	glm::mat4 cachedSensorMatrix = xmmatrix2mat4(DX::OXRManager::instance()->getSensorMatrixAtTime(timeStamp.HostTicks * 100));
	std::vector<cv::Vec3d> rvecs, tvecs;
	TrackAruco(pSensorFrame, 0.15, scenario->m_cameraMatrix, scenario->m_distCoeffs, rvecs, tvecs);

	//if (location && camLocation) {

		//auto sensorCameraToWorld = xmmatrix2mat4(spatialLocationToMatrix(location));
		//auto cameraToWorld = xmmatrix2mat4(spatialLocationToMatrix(camLocation));

		//try estimate marker
		//static cv::Ptr<cv::aruco::Dictionary> dictionary = cv::aruco::getPredefinedDictionary(cv::aruco::DICT_6X6_250);

		//cv::Mat processed(m_slateHeight, m_slateWidth, CV_8U, (void*)m_texture_data);

		//std::vector<int> ids;
		//std::vector<std::vector<cv::Point2f>> corners;
		//cv::aruco::detectMarkers(processed, dictionary, corners, ids);
		//if (ids.empty()) return;

		// if at least one marker detected
		//cv::aruco::estimatePoseSingleMarkers(corners, 0.15, m_cameraMatrix, m_distCoeffs, m_rvecs, m_tvecs);
	if (tvecs.empty()) return;
	auto tvec = tvecs[0];

	//TODO: UPDATE MARKER SCALE BASED ON MARKER
	glm::mat4 rot_mat = glm::toMat4(getQuaternion(rvecs[0]));
	glm::mat4 model_mat = glm::translate(glm::mat4(1.0), glm::vec3(tvec[0], tvec[1], tvec[2])) 
						* rot_mat
						* glm::scale(glm::mat4(1.0f), scenario->m_marker_scale);

	std::lock_guard<std::mutex> guard(scenario->m_mutex);
	scenario->m_marker2world = cachedSensorMatrix * model_mat;
}
void MarkerBasedScenario::Update() {
	std::lock_guard<std::mutex> guard(m_mutex);
	vrController::instance()->setPosition(m_marker2world);
}
