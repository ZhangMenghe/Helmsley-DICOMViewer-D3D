#pragma once

#include "pch.h"
#include <vector>
#include <Common/DeviceResources.h>
#include <OXRs/OXRScenes.h>
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

		XrSpace createReferenceSpace(XrReferenceSpaceType referenceSpaceType, XrPosef poseInReferenceSpace);
		XrSpatialAnchorMSFT createAnchor(const XrPosef& poseInScene);

		XrSpace createAnchorSpace(const XrPosef& poseInScene);
		XrSpace * getAppSpace();

		std::function<void(float, float, float, int)> onSingle3DTouchDown;
		std::function<void(float, float, float, int)> on3DTouchMove;
		std::function<void(int)> on3DTouchReleased;

	private:
		const int64_t d3d_swapchain_fmt = DXGI_FORMAT_R8G8B8A8_UNORM;
		const XrFormFactor app_config_form = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
		const XrViewConfigurationType app_config_view = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;

		const XrPosef  xr_pose_identity = { {0,0,0,1}, {0,0,0} };

		XrInstance     xr_instance = {};
		XrSession      xr_session = {};
		XrSessionState xr_session_state = XR_SESSION_STATE_UNKNOWN;
		bool           xr_running = false;
		bool		   xr_quit = false;
		XrSpace        xr_app_space = {};
		XrSystemId     xr_system_id = XR_NULL_SYSTEM_ID;
		input_state_t  xr_input = { };
		XrEnvironmentBlendMode   xr_blend = {};
		XrDebugUtilsMessengerEXT xr_debug = {};

		std::vector<XrView>                  xr_views;
		std::vector<XrViewConfigurationView> xr_config_views;
		std::vector<swapchain_t>             xr_swapchains;

		// Function pointers for some OpenXR extension methods we'll use.
		PFN_xrGetD3D11GraphicsRequirementsKHR ext_xrGetD3D11GraphicsRequirementsKHR = nullptr;
		PFN_xrCreateDebugUtilsMessengerEXT    ext_xrCreateDebugUtilsMessengerEXT = nullptr;
		PFN_xrDestroyDebugUtilsMessengerEXT   ext_xrDestroyDebugUtilsMessengerEXT = nullptr;
		PFN_xrCreateSpatialAnchorMSFT    ext_xrCreateSpatialAnchorMSFT = nullptr;
		PFN_xrCreateSpatialAnchorSpaceMSFT    ext_xrCreateSpatialAnchorSpaceMSFT = nullptr;

		void openxr_poll_events();
		void openxr_poll_actions();
		void openxr_poll_predicted(XrTime predicted_time);

		

		DirectX::XMMATRIX d3d_xr_projection(XrFovf fov, float clip_near, float clip_far);
		IDXGIAdapter1* d3d_get_adapter(LUID& adapter_luid);
		swapchain_surfdata_t d3d_make_surface_data(XrBaseInStructure& swapchainImage);
		bool openxr_render_layer(XrTime predictedTime, 
			std::vector<XrCompositionLayerProjectionView>& projectionViews,
			XrCompositionLayerProjection& layer,
			OXRScenes* scene);
		void d3d_render_layer(XrCompositionLayerProjectionView& layerView, swapchain_surfdata_t& surface);
	};
}