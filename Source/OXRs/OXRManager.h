#pragma once

#include <vector>
#include <Common/DeviceResources.h>
#include "XrSceneLib/ProjectionLayer.h"
#include "XrSceneLib/Scene.h"
#include "XrSceneLib/CompositionLayers.h"
#include "XrSceneLib/HLSensorManager.h"
#include "XrUtility/XrActionContext.h"
namespace DX {
	struct swapchain_surfdata_t {
		ID3D11DepthStencilView* depth_view;
		ID3D11RenderTargetView* target_view;
	};

	struct swapchain_t {
		XrSwapchain handle;
		int32_t     width;
		int32_t     height;
		std::vector<XrSwapchainImageD3D11KHR> surface_images;
		std::vector<swapchain_surfdata_t>     surface_data;
	};

	class OXRManager : public DeviceResources {
	public:
		OXRManager();
		static OXRManager* instance();

		int64_t GetSwapchainFmt()const { return d3d_swapchain_fmt; }
		bool InitOxrSession(const char* app_name);
		void InitHLSensors(const std::vector<ResearchModeSensorType>& kEnabledSensorTypes);

		bool BeforeUpdate();
		bool Update();
		void Render();
		//void ShutDown();

		void AddScene(xr::Scene* scene);
		//void AddSceneFinished();

		XrSpatialAnchorMSFT createAnchor(const XrPosef& poseInScene);

		XrSpace createAnchorSpace(const XrPosef& poseInScene);
		DirectX::XMMATRIX getSensorMatrixAtTime(uint64_t time);
		HLSensorManager* getHLSensorManager() { 
			if (m_sensor_manager) return m_sensor_manager.get();
			return nullptr;
		}

		winrt::Windows::Perception::Spatial::SpatialCoordinateSystem getReferenceFrame() {
			return m_referenceFrame;
		}
		xr::XrContext* XrContext(){
			return m_context.get();
		}
		xr::ActionContext* XrActionContext() {
			return m_actionContext.get();
		}
		xr::SpaceHandle& XrAppSpace() {
			return m_appSpace;
		}
		XrFrameState getCurrentFrameState() {
			return m_current_framestate;
		}
		//std::function<void(float, float, float, int)> onSingle3DTouchDown;
		//std::function<void(float, float, float, glm::mat4, int)> on3DTouchMove;
		//std::function<void(int)> on3DTouchReleased;

	private:
		static OXRManager* myPtr_;

		std::vector<xr::Scene* > m_scenes;
		std::unique_ptr<HLSensorManager> m_sensor_manager = nullptr;

		const int64_t d3d_swapchain_fmt = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
		const XrFormFactor app_config_form = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
		const XrViewConfigurationType app_config_view = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;


		std::unique_ptr<xr::XrContext> m_context;
		std::unique_ptr<xr::ActionContext> m_actionContext;

		xr::ProjectionLayers m_projectionLayers;
		xr::FrameTime m_currentFrameTime{};

		XrSessionState xr_session_state = XR_SESSION_STATE_UNKNOWN;
		bool           xr_running = false;
		bool		   xr_quit = false;

		xr::SpaceHandle m_appSpace;
		xr::SpaceHandle m_viewSpace;

		XrEnvironmentBlendMode   xr_blend = {};
		XrDebugUtilsMessengerEXT xr_debug = {};
		XrFrameState m_current_framestate, lastFrameState;
		
		// Function pointers for some OpenXR extension methods we'll use.
		PFN_xrGetD3D11GraphicsRequirementsKHR ext_xrGetD3D11GraphicsRequirementsKHR = nullptr;
		PFN_xrCreateDebugUtilsMessengerEXT    ext_xrCreateDebugUtilsMessengerEXT = nullptr;
		PFN_xrDestroyDebugUtilsMessengerEXT   ext_xrDestroyDebugUtilsMessengerEXT = nullptr;
		PFN_xrCreateSpatialAnchorMSFT		  ext_xrCreateSpatialAnchorMSFT = nullptr;
		PFN_xrCreateSpatialAnchorSpaceMSFT    ext_xrCreateSpatialAnchorSpaceMSFT = nullptr;

		std::mutex m_sceneMutex;
		std::mutex m_secondaryViewConfigActiveMutex;

		std::vector<XrSecondaryViewConfigurationStateMSFT> m_secondaryViewConfigurationsState;
		std::unordered_map<XrViewConfigurationType, xr::ViewConfigurationState> m_viewConfigStates;
		bool render_for_MRC = false;

		winrt::Windows::Perception::Spatial::SpatialCoordinateSystem m_referenceFrame = { nullptr };

		PFN_xrLocateHandJointsEXT   ext_xrLocateHandJointsEXT;

		void openxr_poll_events();
		void openxr_poll_hands_ext();

		void SetSecondaryViewConfigurationActive(xr::ViewConfigurationState& secondaryViewConfigState, bool active);

		DirectX::XMMATRIX d3d_xr_projection(XrFovf fov, float clip_near, float clip_far);

		void RenderViewConfiguration(const std::scoped_lock<std::mutex>& proofOfSceneLock,
			XrViewConfigurationType viewConfigurationType,
			xr::CompositionLayers& layers);
	};
}