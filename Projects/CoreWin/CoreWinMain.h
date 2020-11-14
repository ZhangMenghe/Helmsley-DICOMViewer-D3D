#pragma once

#include "Common\StepTimer.h"
#include "Common\DeviceResources.h"
#include <vrController.h>
#include <Utils/dicomLoader.h>
#include <Common/Manager.h>
#include <Renderers/FpsTextRenderer.h>
#include <grpc/rpcHandler.h>

namespace CoreWin{
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

		// TODO: Replace with your own content renderers.
		std::unique_ptr<vrController> m_sceneRenderer;
		std::unique_ptr<Manager> m_manager;

		std::unique_ptr<FpsTextRenderer> m_fpsTextRenderer;
		
		dicomLoader m_dicom_loader;
		// Rendering loop timer.
		DX::StepTimer m_timer;

		// RPC instance
		rpcHandler * m_rpcHandler;

		// RPC thread
		std::thread * m_rpcThread;

		///////debug data//////
		std::string m_ds_path = "dicom-data/IRB01/2100_FATPOSTCORLAVAFLEX20secs/";
		//std::string m_ds_path = "dicom-data/Larry_Smarr_2016/series_23_Cor_LAVA_PRE-Amira/";
		//height, width, depth
		DirectX::XMINT3 vol_dims = DirectX::XMINT3(512, 512, 164);
		//DirectX::XMINT3 vol_dims = DirectX::XMINT3(512, 512, 144);

		void setup_volume_server();
		void setup_volume_local();
	};
}