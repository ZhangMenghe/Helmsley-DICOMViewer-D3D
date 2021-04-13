#pragma once

#include "pch.h"
#include <vector>
#include <Common/DeviceResources.h>
#include <OXRs/OXRScenes.h>
#include <OXRs/XrUtility/XrContext.h>
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

	struct input_state_t {
		XrActionSet actionSet;
		XrAction    poseAction;
		XrAction    selectAction;
		XrPath   handSubactionPath[2];
		XrSpace  handSpace[2];
		XrPosef  handPose[2];
		XrBool32 renderHand[2];
		XrBool32 handSelect[2];
		XrBool32 handDeselect[2];
	};



	class OXRManager : public DeviceResources {
	public:
		OXRManager();
		int64_t GetSwapchainFmt()const { return d3d_swapchain_fmt; }
		bool InitOxrSession(const char* app_name);
		void InitOxrActions();
		bool Update();
		void Render(OXRScenes* scene);
		void ShutDown();

		XrSpatialAnchorMSFT createAnchor(const XrPosef& poseInScene);

		XrSpace createAnchorSpace(const XrPosef& poseInScene);
		XrSpace * getAppSpace();

		winrt::Windows::Perception::Spatial::SpatialCoordinateSystem getReferenceFrame() {
			return m_referenceFrame;
		}
		xr::XrContext& XrContext(){
			return *m_context;
		}

		std::function<void(float, float, float, int)> onSingle3DTouchDown;
		std::function<void(float, float, float, glm::mat4, int)> on3DTouchMove;
		std::function<void(int)> on3DTouchReleased;

	private:
		const int64_t d3d_swapchain_fmt = DXGI_FORMAT_R8G8B8A8_UNORM;
		const XrFormFactor app_config_form = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
		const XrViewConfigurationType app_config_view = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;


		std::unique_ptr<xr::XrContext> m_context;


		//XrInstance     xr_instance = {};
		//XrSession      xr_session = {};
		XrSessionState xr_session_state = XR_SESSION_STATE_UNKNOWN;
		bool           xr_running = false;
		bool		   xr_quit = false;

		xr::SpaceHandle m_appSpace;
		xr::SpaceHandle m_viewSpace;

		//XrSpace        xr_app_space = {};
		//XrSpace				xr_view_space = {};
		//XrSystemId     xr_system_id = XR_NULL_SYSTEM_ID;
		input_state_t  xr_input = { };
		XrEnvironmentBlendMode   xr_blend = {};
		XrDebugUtilsMessengerEXT xr_debug = {};
		XrFrameState lastFrameState;

		std::vector<XrView>                  xr_views;
		std::vector<XrViewConfigurationView> xr_config_views;
		std::vector<swapchain_t>             xr_swapchains;
		std::vector<swapchain_t>             xr_depth_swapchains;

		// Function pointers for some OpenXR extension methods we'll use.
		PFN_xrGetD3D11GraphicsRequirementsKHR ext_xrGetD3D11GraphicsRequirementsKHR = nullptr;
		PFN_xrCreateDebugUtilsMessengerEXT    ext_xrCreateDebugUtilsMessengerEXT = nullptr;
		PFN_xrDestroyDebugUtilsMessengerEXT   ext_xrDestroyDebugUtilsMessengerEXT = nullptr;
		PFN_xrCreateSpatialAnchorMSFT    ext_xrCreateSpatialAnchorMSFT = nullptr;
		PFN_xrCreateSpatialAnchorSpaceMSFT    ext_xrCreateSpatialAnchorSpaceMSFT = nullptr;

		std::mutex m_secondaryViewConfigActiveMutex;
		std::vector<XrSecondaryViewConfigurationStateMSFT> m_secondaryViewConfigurationsState;
		std::vector<XrViewConfigurationType> EnabledSecondaryViewConfigurationTypes;
		bool SupportsSecondaryViewConfiguration = true;
		std::unordered_map<XrViewConfigurationType, xr::ViewConfigurationState> m_viewConfigStates;
		//std::unordered_map<XrViewConfigurationType, ViewProperties> m_viewProperties;
		//std::vector<XrViewConfigurationType> SupportedPrimaryViewConfigurationTypes;
		//std::vector<XrViewConfigurationType> SupportedSecondaryViewConfigurationTypes;
		std::vector<XrView>                  xr_secondary_views;
		std::vector<XrViewConfigurationView> xr_secondary_config_views;
		std::vector<swapchain_t>             xr_secondary_swapchains;
		std::vector<swapchain_t>             xr_secondary_depth_swapchains;
		winrt::Windows::Foundation::Size		 m_secondary_outputSizes;
		bool render_for_MRC = false;

		winrt::Windows::Perception::Spatial::SpatialCoordinateSystem m_referenceFrame = { nullptr };

		void openxr_poll_events();
		void openxr_poll_actions();
		void openxr_poll_predicted(XrTime predicted_time);

		void SetSecondaryViewConfigurationActive(xr::ViewConfigurationState& secondaryViewConfigState, bool active);

		DirectX::XMMATRIX d3d_xr_projection(XrFovf fov, float clip_near, float clip_far);

		swapchain_surfdata_t d3d_make_surface_data(XrBaseInStructure& swapchainImage);
		bool openxr_render_layer(XrTime predictedTime, 
			std::vector<XrCompositionLayerProjectionView>& projectionViews, std::vector<XrCompositionLayerDepthInfoKHR>& depthInfo,
			XrCompositionLayerProjection& layer,
			OXRScenes* scene, bool is_secondary = false);
		void d3d_render_layer(XrCompositionLayerProjectionView& layerView, swapchain_surfdata_t& surface);
	};
}