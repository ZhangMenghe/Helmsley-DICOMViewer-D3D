#ifndef OXR_SCENES_H
#define OXR_SCENES_H
#include <Common/Manager.h>
#include <vrController.h>
#include <Renderers/FpsTextRenderer.h>
#include <Utils/dicomLoader.h>
#include <Utils/uiController.h>
#include <Utils/dataManager.h>
#include <grpc/rpcHandler.h>
#include <OXRs/ArucoMarkerTrackingScenario.h>
#include <OXRs/SensorVizScenario.h>
#include <OXRs/XrSceneLib/Scene.h>
class OXRScenes : public xr::Scene{
public:
	OXRScenes(const std::shared_ptr<xr::XrContext>& context);
	void SetupReferenceFrame(winrt::Windows::Perception::Spatial::SpatialCoordinateSystem referenceFrame) {
		//m_scenario->SetupReferenceFrame(referenceFrame);
	}

	//override
	void Update(const xr::FrameTime& frameTime);
	void BeforeRender(const xr::FrameTime& frameTime);
	void Render(const xr::FrameTime& frameTime, uint32_t view_id);
	void SetupDeviceResource(const std::shared_ptr<DX::DeviceResources>& deviceResources);

	void onViewChanged();
	void setSpaces(XrSpace *space, XrSpace *app_space);

	void onSingle3DTouchDown(float x, float y, float z, int side) { 
		m_sceneRenderer->onSingle3DTouchDown(x, y, z, side); 
		//m_scenario->onSingle3DTouchDown(x, y, z, side);
	};
	void on3DTouchMove(float x, float y, float z, glm::mat4 rot, int side) { m_sceneRenderer->on3DTouchMove(x, y, z, rot, side); };
	void on3DTouchReleased(int side) { m_sceneRenderer->on3DTouchReleased(side); };
	//void saveCurrentTargetViews(ID3D11RenderTargetView* render_target, ID3D11DepthStencilView* depth_target) {
	//	m_deviceResources->saveCurrentTargetViews(render_target, depth_target);
	//}
	void saveCurrentTargetViews(winrt::com_ptr <ID3D11RenderTargetView> render_target,
		winrt::com_ptr<ID3D11DepthStencilView> depth_target, float depth_value) {
		m_deviceResources->saveCurrentTargetViews(render_target, depth_target, depth_value);
	}
private:
	std::shared_ptr<Manager> m_manager;
	std::unique_ptr<vrController> m_sceneRenderer;
	//std::unique_ptr<SensorVizScenario> m_scenario;
	std::shared_ptr<DX::DeviceResources> m_deviceResources = nullptr;

	std::unique_ptr<FpsTextRenderer> m_fpsTextRenderer;
	
	uiController m_uiController;

	dataManager* m_data_manager;

	std::shared_ptr<dicomLoader> m_dicom_loader;

	// RPC instance
	std::shared_ptr<rpcHandler> m_rpcHandler = nullptr;

	// RPC thread
	std::thread *m_rpcThread;

	//XR
	XrSpace* space;
	XrSpace* app_space;

	// Rendering loop timer.
	DX::StepTimer m_timer;

	bool m_overwrite_index_file = false;
	bool m_render_scene = true;
	bool m_local_initialized = false;

	void setup_volume_server();
	void setup_volume_local();
	void setup_resource();
};
#endif