#ifndef SLATE_ARUCO_TRACKER_H
#define SLATE_ARUCO_TRACKER_H

#include <Common/DeviceResources.h>
#include <ResearchModeApi/ResearchModeApi.h>
#include "OpenCVFrameProcessing.h"
#include <mutex>
class SlateArucoTracker{
public:
    SlateArucoTracker(std::shared_ptr<DX::DeviceResources> const& deviceResources);
    ~SlateArucoTracker();

    //DirectX::XMMATRIX GetModelRotation();

    void UpdateSlateTextureWithBitmap(const BYTE *pImage, UINT uWidth, UINT uHeight);
    void StartCVProcessing(BYTE bright);

    static void FrameReadyCallback(IResearchModeSensorFrame* pSensorFrame, PVOID frameCtx);

    //bool GetFirstCenter(float *px, float *py, ResearchModeSensorTimestamp *pTimeStamp);
    bool GetFirstTransformation(cv::Vec3d& rvec, cv::Vec3d& tvec);
protected:

    //void GetModelVertices(std::vector<VertexPositionColor> &returnedModelVertices);

    //void GetModelTriangleIndices(std::vector<unsigned short> &triangleIndices);

    virtual void UpdateSlateTexture();

    //void EnsureSlateTexture();

    void FrameProcessing();

    static void FrameProcessingThread(SlateArucoTracker* tracker);
    bool m_fExit = { false };
    HANDLE m_hFrameEvent;

    IResearchModeSensorFrame* m_pSensorFrame;
    IResearchModeSensorFrame* m_pSensorFrameIn;
    UINT m_Width, m_Height;

    cv::Mat m_cameraMatrix, m_distCoeffs;

    //std::vector<std::vector<cv::Point2f>> m_corners;
    //std::vector<cv::Point2f> m_centers;
    //std::vector<int> m_ids;

    std::vector<cv::Vec3d> rvecs, tvecs;

    ResearchModeSensorTimestamp m_timeStamp;

    //thread
    std::thread* m_pFrameUpdateThread;

    //mutex
    std::mutex m_frameMutex;
    std::mutex m_cornerMutex;
    std::mutex m_mutex;
    cv::Mat m_cvResultMat;
};
#endif
