#ifndef OXR_MAIN_SCENE_H
#define OXR_MAIN_SCENE_H

#include <Common/Manager.h>
#include <vrController.h>
#include <SceneObjs/overUIBoard.h>
#include <Utils/dicomLoader.h>
#include <Utils/uiController.h>
#include <Utils/dataManager.h>
#include <grpc/rpcHandler.h>
#include <OXRs/XrSceneLib/Scene.h>
#include <OXRs/XrScenarios/MarkerBasedScenario.h>
#include <SceneObjs/handSystem.h>

class OXRMainScene : public xr::Scene{
public:
	OXRMainScene(const std::shared_ptr<xr::XrContext>& context);
	void SetupReferenceFrame(winrt::Windows::Perception::Spatial::SpatialCoordinateSystem referenceFrame) {
		//m_scenario->SetupReferenceFrame(referenceFrame);
	}

	//override
	void Update(const xr::FrameTime& frameTime);
	void BeforeRender(const xr::FrameTime& frameTime);
	void Render(const xr::FrameTime& frameTime, uint32_t view_id);
	void SetupDeviceResource(const std::shared_ptr<DX::DeviceResources>& deviceResources);

	void onViewChanged();

	void onSingle3DTouchDown(float x, float y, float z, int side);
	void on3DTouchMove(float x, float y, float z, glm::mat4 rot, int side);
	void on3DTouchReleased(int side);
	
	void saveCurrentTargetViews(winrt::com_ptr <ID3D11RenderTargetView> render_target,
		winrt::com_ptr<ID3D11DepthStencilView> depth_target, float depth_value) {
		m_deviceResources->saveCurrentTargetViews(render_target, depth_target, depth_value);
	}
private:
	std::shared_ptr<Manager> m_manager;
	std::unique_ptr<vrController> m_sceneRenderer;
	MarkerBasedScenario* m_scenario;
	std::shared_ptr<DX::DeviceResources> m_deviceResources = nullptr;

	std::unique_ptr<overUIBoard> m_ui_board;
	
	uiController m_uiController;

	dataManager* m_data_manager;

	std::shared_ptr<dicomLoader> m_dicom_loader;

	// RPC instance
	std::shared_ptr<rpcHandler> m_rpcHandler = nullptr;

	// RPC thread
	std::thread *m_rpcThread;

	// Rendering loop timer.
	DX::StepTimer m_timer;

	bool m_overwrite_index_file = false;
	bool m_local_initialized = false;

	void setup_volume_server();
	void setup_volume_local();
	void setup_resource();
};
#endif