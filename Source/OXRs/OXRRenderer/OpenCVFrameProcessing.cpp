#include "pch.h"
#include "OpenCVFrameProcessing.h"
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>  // cv::Canny()
#include <opencv2/aruco.hpp>
#include <opencv2/core/mat.hpp>

void ProcessRmFrameWithAruco(IResearchModeSensorFrame* pSensorFrame, cv::Mat& cvResultMat, std::vector<int> &ids, std::vector<std::vector<cv::Point2f>> &corners)
{
    HRESULT hr = S_OK;
    ResearchModeSensorResolution resolution;
    size_t outBufferCount = 0;
    const BYTE *pImage = nullptr;
    IResearchModeSensorVLCFrame *pVLCFrame = nullptr;
    pSensorFrame->GetResolution(&resolution);
    static cv::Ptr<cv::aruco::Dictionary> dictionary = cv::aruco::getPredefinedDictionary(cv::aruco::DICT_6X6_250);

    hr = pSensorFrame->QueryInterface(IID_PPV_ARGS(&pVLCFrame));

    if (SUCCEEDED(hr))
    {
        pVLCFrame->GetBuffer(&pImage, &outBufferCount);

        cv::Mat processed(resolution.Height, resolution.Width, CV_8U, (void*)pImage);
        cv::aruco::detectMarkers(processed, dictionary, corners, ids);

        cvResultMat = processed;

        // if at least one marker detected
        if (ids.size() > 0)
            cv::aruco::drawDetectedMarkers(cvResultMat, corners, ids);
    }

    if (pVLCFrame)
    {
        pVLCFrame->Release();
    }
}

void TrackAruco(IResearchModeSensorFrame* pSensorFrame, float marker_len, 
    const cv::Mat& cameraMatrix, const cv::Mat& distCoeffs, 
    std::vector<cv::Vec3d>& rvecs, std::vector<cv::Vec3d>& tvecs) {

    HRESULT hr = S_OK;
    ResearchModeSensorResolution resolution;
    size_t outBufferCount = 0;
    const BYTE* pImage = nullptr;
    IResearchModeSensorVLCFrame* pVLCFrame = nullptr;
    pSensorFrame->GetResolution(&resolution);
    static cv::Ptr<cv::aruco::Dictionary> dictionary = cv::aruco::getPredefinedDictionary(cv::aruco::DICT_6X6_250);

    hr = pSensorFrame->QueryInterface(IID_PPV_ARGS(&pVLCFrame));

    if (SUCCEEDED(hr))
    {
        pVLCFrame->GetBuffer(&pImage, &outBufferCount);

        cv::Mat processed(resolution.Height, resolution.Width, CV_8U, (void*)pImage);
        std::vector<int> ids;
        std::vector<std::vector<cv::Point2f>> corners;
        cv::aruco::detectMarkers(processed, dictionary, corners, ids);
        // if at least one marker detected
        if (ids.size() > 0) {
            cv::aruco::estimatePoseSingleMarkers(corners, marker_len, cameraMatrix, distCoeffs, rvecs, tvecs);
        }
    }
    if (pVLCFrame)
    {
        pVLCFrame->Release();
    }
}
void ProcessRmFrameWithCanny(IResearchModeSensorFrame* pSensorFrame, cv::Mat& cvResultMat)
{
    HRESULT hr = S_OK;
    ResearchModeSensorResolution resolution;
    size_t outBufferCount = 0;
    const BYTE *pImage = nullptr;
    IResearchModeSensorVLCFrame *pVLCFrame = nullptr;
    pSensorFrame->GetResolution(&resolution);

    hr = pSensorFrame->QueryInterface(IID_PPV_ARGS(&pVLCFrame));

    if (SUCCEEDED(hr))
    {
        pVLCFrame->GetBuffer(&pImage, &outBufferCount);

        cv::Mat processed(resolution.Height, resolution.Width, CV_8U, (void*)pImage);

        cv::Canny(processed, cvResultMat, 400, 1000, 5);
    }

    if (pVLCFrame)
    {
        pVLCFrame->Release();
    }
}


