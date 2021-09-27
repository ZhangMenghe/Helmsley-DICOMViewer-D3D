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

	ResearchModeSensorTimestamp timeStamp;
	pSensorFrame->GetTimeStamp(&timeStamp);

	auto timestamp = PerceptionTimestampHelper::FromSystemRelativeTargetTime(winrt::Windows::Foundation::TimeSpan{ timeStamp.HostTicks });//FromHistoricalTargetTime(c.GetDateTime());//TimeSpanFromQpcTicks(qpcNow.QuadPart));//winrt::Windows::Foundation::TimeSpan{ timeStamp.HostTicks });

	glm::mat4 Sensor2World = xmmatrix2mat4(DX::OXRManager::instance()->getSensorMatrixAtTime(timeStamp.HostTicks * 100));
	std::vector<cv::Vec3d> rvecs, tvecs;
	TrackAruco(pSensorFrame, 0.15, scenario->m_cameraMatrix, scenario->m_distCoeffs, rvecs, tvecs);

	if (tvecs.empty()) return;
	glm::vec3 tvec = glm::vec3(tvecs[0][0], tvecs[0][1], tvecs[0][2]);
	glm::mat4 rot_mat = glm::toMat4(getQuaternion(rvecs[0]));
	
	glm::mat4 Marker2Sensor = glm::translate(glm::mat4(1.0), tvec) 
							* rot_mat
							* glm::scale(glm::mat4(1.0f), glm::vec3(scenario->m_marker_size));

	std::lock_guard<std::mutex> guard(scenario->m_mutex);
	scenario->m_marker2world = Sensor2World * Marker2Sensor;
}
void MarkerBasedScenario::Update() {
	std::lock_guard<std::mutex> guard(m_mutex);
	vrController::instance()->setPosition(m_marker2world);
}
