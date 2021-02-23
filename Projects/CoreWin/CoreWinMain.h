#ifndef CORE_WIN_MAIN_H
#define CORE_WIN_MAIN_H
#include "Common\StepTimer.h"
#include "Common\DeviceResources.h"
#include <vrController.h>
#include <Utils/dicomLoader.h>
#include <Utils/uiController.h>
#include <Common/Manager.h>
#include <Renderers/FpsTextRenderer.h>
#include <grpc/rpcHandler.h>
#include <Utils/dataManager.h>

namespace CoreWin {
	class CoreWinMain : public DX::IDeviceNotify
	{
	public:
		CoreWinMain(const std::shared_ptr<DX::DeviceResources>& deviceResources);
		~CoreWinMain();
		void CreateWindowSizeDependentResources();
		void Update();
		bool Render();

		// IDeviceNotify
		virtual void OnDeviceLost();
		virtual void OnDeviceRestored();

		void OnPointerPressed(float x, float y);
		void OnPointerMoved(float x, float y);
		void OnPointerReleased();
	private:
		// Cached pointer to device resources.
		std::shared_ptr<DX::DeviceResources> m_deviceResources;

		std::shared_ptr<Manager> m_manager;
		std::unique_ptr<vrController> m_sceneRenderer;

		std::unique_ptr<FpsTextRenderer> m_fpsTextRenderer;

		uiController m_uiController;

		dataManager* m_data_manager;

		// Rendering loop timer.
		DX::StepTimer m_timer;

		std::shared_ptr<dicomLoader> m_dicom_loader;

		// RPC instance
		std::shared_ptr<rpcHandler> m_rpcHandler = nullptr;

		// RPC thread
		std::thread* m_rpcThread;

		const bool m_overwrite_index_file = false;
		void setup_volume_server();
		void setup_volume_local();
		void setup_resource();
	};
}
#endif