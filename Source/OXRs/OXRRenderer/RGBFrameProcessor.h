#ifndef RGB_FRAME_PROCESSOR_H
#define RGB_FRAME_PROCESSOR_H

#include <MemoryBuffer.h>
#include <winrt/Windows.Media.Devices.Core.h>
#include <winrt/Windows.Media.Capture.Frames.h>
#include <winrt/Windows.Perception.Spatial.h>
#include <winrt/Windows.Graphics.Imaging.h>

#include <mutex>
#include <shared_mutex>
#include <thread>
#include <OXRs/TimeConverter.h>
#include <opencv2/core.hpp>
#include <D3DPipeline/Texture.h>
#include <Common/DeviceResources.h>
// Struct to store per-frame PV information:
// timestamp, PV2world transform, focal length
struct PVFrame{
    long long timestamp;
    winrt::Windows::Foundation::Numerics::float4x4 PVtoWorldtransform;
    float fx;
    float fy;    
};


class RGBFrameProcessor{
public:
    RGBFrameProcessor(std::shared_ptr<DX::DeviceResources> const& deviceResources)
        :m_deviceResources(deviceResources){}
    virtual ~RGBFrameProcessor()
    {
        m_fExit = true;
        m_pWriteThread->join();
    }

    void Clear();
    void AddLogFrame();
    bool DumpDataToDisk(const winrt::Windows::Storage::StorageFolder& folder, const std::wstring& datetime_path);
    void StartRecording(const winrt::Windows::Storage::StorageFolder& storageFolder, const winrt::Windows::Perception::Spatial::SpatialCoordinateSystem& worldCoordSystem);
    void StopRecording();
    void setTargetTexture(std::shared_ptr<Texture> tex) { m_targetTex = tex; }
    winrt::Windows::Foundation::IAsyncAction InitializeAsync();

protected:
    void OnFrameArrived(const winrt::Windows::Media::Capture::Frames::MediaFrameReader& sender,        
                        const winrt::Windows::Media::Capture::Frames::MediaFrameArrivedEventArgs& args);

private:
    void DumpFrame(const winrt::Windows::Graphics::Imaging::SoftwareBitmap& softwareBitmap, long long timestamp);

    winrt::Windows::Media::Capture::Frames::MediaFrameReader m_mediaFrameReader = nullptr;
    winrt::event_token m_OnFrameArrivedRegistration;

    std::shared_mutex m_frameMutex;
    long long m_latestTimestamp = 0;
    winrt::Windows::Media::Capture::Frames::MediaFrameReference m_latestFrame = nullptr;
    std::vector<PVFrame> m_PVFrameLog;
    
    std::mutex m_storageMutex;
    winrt::Windows::Storage::StorageFolder m_storageFolder = nullptr;

    winrt::Windows::Perception::Spatial::SpatialCoordinateSystem m_worldCoordSystem = nullptr;

    // writing thread
    static void CameraWriteThread(RGBFrameProcessor* pProcessor);
    std::thread* m_pWriteThread = nullptr;
    bool m_fExit = false;
    TimeConverter m_converter;
    std::shared_ptr<Texture> m_targetTex;
    std::shared_ptr<DX::DeviceResources> m_deviceResources;
    
    cv::Mat m_FrameMat;
    static int frame_count;
    static const int kImageWidth;
    static const wchar_t kSensorName[3];
};
#endif