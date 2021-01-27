#ifndef CORE_WIN_MAIN_H
#define CORE_WIN_MAIN_H
#include <Common/Manager.h>
#include <vrController.h>
#include <Renderers/FpsTextRenderer.h>
#include <Utils/dicomLoader.h>
#include <Utils/uiController.h>
#include <Utils/dataManager.h>
#include <grpc/rpcHandler.h>

class CoreWinMain : public DX::IDeviceNotify{
public:
	CoreWinMain(const std::shared_ptr<DX::DeviceResources> &deviceResources);
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

	dataManager *m_data_manager;

	std::shared_ptr<dicomLoader> m_dicom_loader;

	// RPC instance
	std::shared_ptr<rpcHandler> m_rpcHandler = nullptr;

	// RPC thread
	std::thread *m_rpcThread;

	// Rendering loop timer.
	DX::StepTimer m_timer;

	///////debug data//////
	std::string m_ds_path = "helmsley_cached/Larry_Smarr_2016/series_23_Cor_LAVA_PRE-Amira/";
	DirectX::XMINT3 vol_dims = DirectX::XMINT3(512, 512, 144);

	void setup_volume_server();
	void setup_volume_local();
	void setup_resource();
};

#endif