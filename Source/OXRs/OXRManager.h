#pragma once

#include "pch.h"
#include <vector>
#include <Common/DeviceResources.h>
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
	};

	class OXRManager : public DeviceResources {
	public:
		OXRManager();
	private:
		// Function pointers for some OpenXR extension methods we'll use.
		PFN_xrGetD3D11GraphicsRequirementsKHR ext_xrGetD3D11GraphicsRequirementsKHR = nullptr;
		PFN_xrCreateDebugUtilsMessengerEXT    ext_xrCreateDebugUtilsMessengerEXT = nullptr;
		PFN_xrDestroyDebugUtilsMessengerEXT   ext_xrDestroyDebugUtilsMessengerEXT = nullptr;
	};
}