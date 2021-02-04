﻿#ifndef OXR_SCENES_H
#define OXR_SCENES_H
#include <Common/Manager.h>
#include <vrController.h>
#include <Renderers/FpsTextRenderer.h>
#include <Utils/dicomLoader.h>
#include <Utils/uiController.h>
#include <Utils/dataManager.h>
#include <grpc/rpcHandler.h>

class OXRScenes {
public:
	OXRScenes(const std::shared_ptr<DX::DeviceResources>& deviceResources);
	void Update();
	void Update(XrTime time);
	bool Render();
	void onViewChanged();
private:
	std::shared_ptr<DX::DeviceResources> m_deviceResources;

	std::shared_ptr<Manager> m_manager;
	std::unique_ptr<vrController> m_sceneRenderer;

	std::unique_ptr<FpsTextRenderer> m_fpsTextRenderer;

	uiController m_uiController;

	dataManager* m_data_manager;

	std::shared_ptr<dicomLoader> m_dicom_loader;

	// RPC instance
	std::shared_ptr<rpcHandler> m_rpcHandler = nullptr;

	// RPC thread
	std::thread* m_rpcThread;

	//XR
	XrSpace* space;
	XrSpace* app_space;

	// Rendering loop timer.
	DX::StepTimer m_timer;

	const bool m_overwrite_index_file = false;

	void setup_volume_server();
	void setup_volume_local();
	void setup_resource();
};
#endif