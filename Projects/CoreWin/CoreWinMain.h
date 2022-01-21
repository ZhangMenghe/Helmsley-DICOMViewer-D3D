#ifndef CORE_WIN_MAIN_H
#define CORE_WIN_MAIN_H
//#include <Renderers/FpsTextRenderer.h>
#include <Utils/dataManager.h>
#include <SceneObjs/overUIBoard.h>
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

		void onMainButtonClicked(){}
	private:
		// Cached pointer to device resources.
		std::shared_ptr<DX::DeviceResources> m_deviceResources;

		std::shared_ptr<Manager> m_manager;
		std::unique_ptr<vrController> m_sceneRenderer;

		std::unique_ptr<overUIBoard> m_static_uiboard, m_popup_uiboard;

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
		bool m_local_initialized = false;
		bool m_waitfor_operation = true;
		bool m_pop_up_ui_visible = true;

		void setup_volume_server();
		void setup_volume_local();
		void setup_resource();
	};
}
#endif