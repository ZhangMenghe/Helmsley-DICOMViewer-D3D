﻿#ifndef MARKER_BASED_SCENARIO_H
#define MARKER_BASED_SCENARIO_H
#include "Scenario.h"
#include <ResearchModeApi/ResearchModeApi.h>
#include <opencv2/core.hpp>

class MarkerBasedScenario : public Scenario {
public:
    MarkerBasedScenario(const std::shared_ptr<xr::XrContext>& context);
    void setMarkerSize(float sz) { m_marker_size = sz; }
    void Update();
private:
    std::vector<ResearchModeSensorType> kEnabledRMStreamTypes = { ResearchModeSensorType::LEFT_FRONT };
    cv::Mat m_cameraMatrix, m_distCoeffs;
    std::mutex m_mutex;
    glm::mat4 m_marker2world = glm::mat4(1.0f);
    glm::vec3 m_marker_scale = glm::vec3(0.2f);

    //Marker size in meter. 0.15 by default
    float m_marker_size = 0.15;
    //bool m_first_time_detected = false;
    bool m_is_tracking = false;
    int m_no_tracking_frames = 0;

    static void FrameReadyCallback(IResearchModeSensorFrame* pSensorFrame, PVOID frameCtx);
};
#endif