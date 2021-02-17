#ifndef OPENCV_FRAME_PROCESSING_H
#define OPENCV_FRAME_PROCESSING_H

#include <ResearchModeApi/ResearchModeApi.h>
#include <opencv2/core/mat.hpp>
struct DetectedMarker{
    int32_t markerId;

    glm::vec3 point;
    glm::vec3 dir;

    // Image position
    int x;
    int y;
};

void ProcessRmFrameWithAruco(IResearchModeSensorFrame* pSensorFrame, cv::Mat& cvResultMat, std::vector<int> &ids, std::vector<std::vector<cv::Point2f>> &corners);
void TrackAruco(IResearchModeSensorFrame* pSensorFrame, float marker_len, const cv::Mat& cameraMatrix, const cv::Mat& distCoeffs, std::vector<cv::Vec3d>& rvecs, std::vector<cv::Vec3d>& tvecs);
void ProcessRmFrameWithCanny(IResearchModeSensorFrame* pSensorFrame, cv::Mat& cvResultMat);
#endif // !OPENCV_FRAME_PROCESSING_H


