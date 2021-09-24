#ifndef RM_CAMERA_READER_H
#define RM_CAMERA_READER_H

#include <functional>
#include <mutex>
#include <ResearchModeApi/ResearchModeApi.h>

class RMCameraReader
{
public:
	RMCameraReader(IResearchModeSensor* pLLSensor, HANDLE camConsentGiven, ResearchModeSensorConsent* camAccessConsent, const GUID& guid);
	~RMCameraReader();
	void SetFrameCallBack(std::function<void(IResearchModeSensorFrame*, PVOID frameCtx)> frameCallback, PVOID frameCtx);

private:
	std::thread* m_pCameraUpdateThread;
	bool m_fExit = false;

	std::function<void(IResearchModeSensorFrame*, PVOID frameCtx)> m_frameCallback;
	PVOID m_frameCtx;

	// Mutex to access sensor frame
	std::mutex m_sensorFrameMutex;
	IResearchModeSensor* m_pRMSensor = nullptr;
	IResearchModeSensorFrame* m_pSensorFrame = nullptr;

	//timestamp
	uint64_t m_refreshTimeInMilliseconds = 0;
	uint64_t m_sensorRefreshTime = 0;
	uint64_t m_lastHostTicks = 0;

	// Thread for retrieving frames
	static void CameraUpdateThread(RMCameraReader* pReader, HANDLE camConsentGiven, ResearchModeSensorConsent* camAccessConsent);
};
#endif
