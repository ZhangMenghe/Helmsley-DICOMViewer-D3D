#pragma once

#include "Common\StepTimer.h"
#include "Common\DeviceResources.h"
#include <vrController.h>
#include <Utils/dicomLoader.h>
#include <Common/Manager.h>
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
		std::string m_ds_path = "dicom-data/IRB01/2100_FATPOSTCORLAVAFLEX20secs/";
		DirectX::XMINT3 vol_dims = DirectX::XMINT3(512, 512, 164);

		// Cached pointer to device resources.
		std::shared_ptr<DX::DeviceResources> m_deviceResources;

		// TODO: Replace with your own content renderers.
		std::unique_ptr<vrController> m_sceneRenderer;
		std::unique_ptr<Manager> m_manager;

		//std::unique_ptr<SampleFpsTextRenderer> m_fpsTextRenderer;
		dicomLoader m_dicom_loader;
		// Rendering loop timer.
		DX::StepTimer m_timer;
	};
}