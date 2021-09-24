#ifndef MARKER_BASED_SCENARIO_H
#define MARKER_BASED_SCENARIO_H
#include "Scenario.h"
#include <ResearchModeApi/ResearchModeApi.h>
#include <opencv2/core.hpp>

class MarkerBasedScenario : public Scenario {
public:
    MarkerBasedScenario(const std::shared_ptr<xr::XrContext>& context);
    void Update();
private:
    std::vector<ResearchModeSensorType> kEnabledRMStreamTypes = { ResearchModeSensorType::LEFT_FRONT };
    cv::Mat m_cameraMatrix, m_distCoeffs;
    std::mutex m_mutex;
    glm::mat4 m_marker2world = glm::mat4(1.0f);
    glm::vec3 m_marker_scale = glm::vec3(0.2f);

    static void FrameReadyCallback(IResearchModeSensorFrame* pSensorFrame, PVOID frameCtx);
};
#endif
