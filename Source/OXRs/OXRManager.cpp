#include "pch.h"
#include "OXRManager.h"
#include <thread> // sleep_for
#include <Common/Manager.h>
#include <Common/DirectXHelper.h>
#include <Utils/XrMath.h>
#include <glm/gtc/quaternion.hpp>
#include <OXRs/DxCommon/DxUtility.h>

#include <winrt/Windows.Perception.Spatial.h>
#include <winrt/Windows.Perception.Spatial.Preview.h>
using namespace winrt::Windows::Perception;
using namespace winrt::Windows::Perception::Spatial;
using namespace winrt::Windows::Perception::Spatial::Preview;

using namespace DX;

const XrViewConfigurationType PrimaryViewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;

const std::vector<XrViewConfigurationType> SupportedViewConfigurationTypes = {
    XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,
    XR_VIEW_CONFIGURATION_TYPE_SECONDARY_MONO_FIRST_PERSON_OBSERVER_MSFT,
};

const std::vector<XrEnvironmentBlendMode> SupportedEnvironmentBlendModes = {
      XR_ENVIRONMENT_BLEND_MODE_ADDITIVE,
      XR_ENVIRONMENT_BLEND_MODE_OPAQUE,
      XR_ENVIRONMENT_BLEND_MODE_ALPHA_BLEND,
};

const std::vector<D3D_FEATURE_LEVEL> SupportedFeatureLevels = {
    D3D_FEATURE_LEVEL_12_1,
    D3D_FEATURE_LEVEL_12_0,
    D3D_FEATURE_LEVEL_11_1,
    D3D_FEATURE_LEVEL_11_0,
    D3D_FEATURE_LEVEL_10_1,
    D3D_FEATURE_LEVEL_10_0,
};
const std::vector<DXGI_FORMAT> SupportedColorSwapchainFormats = {
    DXGI_FORMAT_R8G8B8A8_UNORM,
    DXGI_FORMAT_B8G8R8A8_UNORM,
    DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
    DXGI_FORMAT_B8G8R8A8_UNORM_SRGB,
};

const std::vector<DXGI_FORMAT> SupportedDepthSwapchainFormats = {
    DXGI_FORMAT_D32_FLOAT,
    DXGI_FORMAT_D32_FLOAT_S8X24_UINT,
    DXGI_FORMAT_D24_UNORM_S8_UINT,
    DXGI_FORMAT_D16_UNORM,
};

OXRManager::OXRManager()
    :DeviceResources(true) {
    m_d3dFeatureLevel = D3D_FEATURE_LEVEL_11_0;
}

template <typename TArray, typename TValue>
inline bool Contains(const TArray& array, const TValue& value) {
    return std::end(array) != std::find(std::begin(array), std::end(array), value);
}

inline bool IsRecommendedSwapchainSizeChanged(const std::vector<XrViewConfigurationView>& oldConfigs,
    const std::vector<XrViewConfigurationView>& newConfigs) {
    assert(oldConfigs.size() == newConfigs.size());
    size_t end = (std::min)(oldConfigs.size(), newConfigs.size());
    for (size_t i = 0; i < end; i++) {
        if ((oldConfigs[i].recommendedImageRectWidth != newConfigs[i].recommendedImageRectWidth) ||
            (oldConfigs[i].recommendedImageRectHeight != newConfigs[i].recommendedImageRectHeight)) {
            return true;
        }
    }
    return false;
}

bool OXRManager::InitOxrSession(const char* app_name) {
    //  // OpenXR will fail to initialize if we ask for an extension that OpenXR
    //// can't provide! So we need to check our all extensions before 
    //// initializing OpenXR with them. Note that even if the extension is 
    //// present, it's still possible you may not be able to use it. For 
    //// example: the hand tracking extension may be present, but the hand
    //// sensor might not be plugged in or turned on. There are often 
    //// additional checks that should be made before using certain features!
    //  std::vector<const char*> use_extensions;
    //  const char* ask_extensions[] = {
    //    XR_KHR_D3D11_ENABLE_EXTENSION_NAME, // Use Direct3D11 for rendering
    //    XR_EXT_DEBUG_UTILS_EXTENSION_NAME,  // Debug utils for extra info
    //    XR_MSFT_SPATIAL_ANCHOR_EXTENSION_NAME,  // For spatial anchor
    //    XR_MSFT_UNBOUNDED_REFERENCE_SPACE_EXTENSION_NAME,
    //    XR_MSFT_SECONDARY_VIEW_CONFIGURATION_EXTENSION_NAME, // MRC
    //    XR_MSFT_FIRST_PERSON_OBSERVER_EXTENSION_NAME,
    //  };
    //
    //  // We'll get a list of extensions that OpenXR provides using this 
    //  // enumerate pattern. OpenXR often uses a two-call enumeration pattern 
    //  // where the first call will tell you how much memory to allocate, and
    //  // the second call will provide you with the actual data!
    //  uint32_t ext_count = 0;
    //  xrEnumerateInstanceExtensionProperties(nullptr, 0, &ext_count, nullptr);
    //  std::vector<XrExtensionProperties> xr_exts(ext_count, { XR_TYPE_EXTENSION_PROPERTIES });
    //  xrEnumerateInstanceExtensionProperties(nullptr, ext_count, &ext_count, xr_exts.data());
    //
    //  printf("OpenXR extensions available:\n");
    //  for (size_t i = 0; i < xr_exts.size(); i++) {
    //    printf("- %s\n", xr_exts[i].extensionName);
    //
    //    // Check if we're asking for this extensions, and add it to our use 
    //    // list!
    //    for (int32_t ask = 0; ask < _countof(ask_extensions); ask++) {
    //      if (strcmp(ask_extensions[ask], xr_exts[i].extensionName) == 0) {
    //        use_extensions.push_back(ask_extensions[ask]);
    //        break;
    //      }
    //    }
    //  }
    //  // If a required extension isn't present, you want to ditch out here!
    //  // It's possible something like your rendering API might not be provided
    //  // by the active runtime. APIs like OpenGL don't have universal support.
    //  if (!std::any_of(use_extensions.begin(), use_extensions.end(),
    //    [](const char* ext) {
    //    return strcmp(ext, XR_KHR_D3D11_ENABLE_EXTENSION_NAME) == 0;
    //  }))
    //    return false;
    const char* requestedExtensions[] = {
    XR_KHR_D3D11_ENABLE_EXTENSION_NAME,//Rendering Tool: D3D
    XR_EXT_DEBUG_UTILS_EXTENSION_NAME,  // Debug utils for extra info
    XR_MSFT_SPATIAL_ANCHOR_EXTENSION_NAME,  // For spatial anchor
    XR_MSFT_UNBOUNDED_REFERENCE_SPACE_EXTENSION_NAME,
    XR_MSFT_SECONDARY_VIEW_CONFIGURATION_EXTENSION_NAME,
    XR_MSFT_FIRST_PERSON_OBSERVER_EXTENSION_NAME,
    XR_EXT_HP_MIXED_REALITY_CONTROLLER_EXTENSION_NAME,
    //////
    XR_EXT_WIN32_APPCONTAINER_COMPATIBLE_EXTENSION_NAME,
    XR_KHR_COMPOSITION_LAYER_DEPTH_EXTENSION_NAME,
    XR_EXT_HAND_TRACKING_EXTENSION_NAME,
    XR_MSFT_HAND_INTERACTION_EXTENSION_NAME,
    XR_MSFT_HAND_TRACKING_MESH_EXTENSION_NAME,
    };
    xr::ExtensionContext extensions = xr::CreateExtensionContext(std::vector<const char*>(std::begin(requestedExtensions), std::end(requestedExtensions)));
    if (!extensions.SupportsD3D11) {
        throw std::logic_error("This sample currently only supports D3D11.");
        return false;
    }

    xr::InstanceContext instance =
        xr::CreateInstanceContext({ app_name, 1 },//xr::AppInfo
            { "XrSceneLib", 1 }, //xr::engineInfo
            extensions.EnabledExtensions);

    extensions.PopulateDispatchTable(instance.Handle);
    auto xr_instance = instance.Handle;

    //// Initialize OpenXR with the extensions we've found!
    //XrInstanceCreateInfo createInfo = { XR_TYPE_INSTANCE_CREATE_INFO };
    //createInfo.enabledExtensionCount = extension_results.EnabledExtensions.size();
    //createInfo.enabledExtensionNames = extension_results.EnabledExtensions.data();
    //createInfo.applicationInfo.apiVersion = XR_CURRENT_API_VERSION;
    //strcpy_s(createInfo.applicationInfo.applicationName, app_name);
    //xrCreateInstance(&createInfo, &xr_instance);

    // Check if OpenXR is on this system, if this is null here, the user 
    // needs to install an OpenXR runtime and ensure it's active!
    if (xr_instance == nullptr)
        return false;

    // Load extension methods that we'll need for this application! There's a
    // couple ways to do this, and this is a fairly manual one. Chek out this
    // file for another way to do it:
    // https://github.com/maluoi/StereoKit/blob/master/StereoKitC/systems/platform/openxr_extensions.h
    //xrGetInstanceProcAddr(xr_instance, "xrCreateDebugUtilsMessengerEXT", (PFN_xrVoidFunction*)(&ext_xrCreateDebugUtilsMessengerEXT));
    //xrGetInstanceProcAddr(xr_instance, "xrDestroyDebugUtilsMessengerEXT", (PFN_xrVoidFunction*)(&ext_xrDestroyDebugUtilsMessengerEXT));
    //xrGetInstanceProcAddr(xr_instance, "xrGetD3D11GraphicsRequirementsKHR", (PFN_xrVoidFunction*)(&ext_xrGetD3D11GraphicsRequirementsKHR));
    //xrGetInstanceProcAddr(xr_instance, "xrCreateSpatialAnchorMSFT", (PFN_xrVoidFunction*)(&ext_xrCreateSpatialAnchorMSFT));
    //xrGetInstanceProcAddr(xr_instance, "xrCreateSpatialAnchorSpaceMSFT", (PFN_xrVoidFunction*)(&ext_xrCreateSpatialAnchorSpaceMSFT));

    // Set up a really verbose debug log! Great for dev, but turn this off or
    // down for final builds. WMR doesn't produce much output here, but it
    // may be more useful for other runtimes?
    // Here's some extra information about the message types and severities:
    // https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#debug-message-categorization
    //XrDebugUtilsMessengerCreateInfoEXT debug_info = { XR_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };
    //debug_info.messageTypes =
    //    XR_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
    //    XR_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
    //    XR_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT |
    //    XR_DEBUG_UTILS_MESSAGE_TYPE_CONFORMANCE_BIT_EXT;
    //debug_info.messageSeverities =
    //    XR_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
    //    XR_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
    //    XR_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
    //    XR_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    //debug_info.userCallback = [](XrDebugUtilsMessageSeverityFlagsEXT severity, XrDebugUtilsMessageTypeFlagsEXT types, const XrDebugUtilsMessengerCallbackDataEXT* msg, void* user_data) {
    //    // Print the debug message we got! There's a bunch more info we could
    //    // add here too, but this is a pretty good start, and you can always
    //    // add a breakpoint this line!
    //    printf("%s: %s\n", msg->functionName, msg->message);

    //    // Output to debug window
    //    char text[512];
    //    sprintf_s(text, "%s: %s", msg->functionName, msg->message);
    //    OutputDebugStringA(text);

    //    // Returning XR_TRUE here will force the calling function to fail
    //    return (XrBool32)XR_FALSE;
    //};
    // Start up the debug utils!
    //if (ext_xrCreateDebugUtilsMessengerEXT)
        //ext_xrCreateDebugUtilsMessengerEXT(xr_instance, &debug_info, &xr_debug);


            // Then get the active system with required form factor.
        // If no system is plugged in, wait until the device is plugged in.
    xr::SystemContext system = [&instance, &extensions] {
        std::optional<xr::SystemContext> systemOpt;
        while (!(systemOpt = xr::CreateSystemContext(instance.Handle,
            extensions,
            XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY,//app_config_form
            SupportedViewConfigurationTypes,
            SupportedEnvironmentBlendModes))) {
            //sample::Trace("Waiting for system plugin ...");
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
        return systemOpt.value();
    }();
    if (!xr::Contains(system.SupportedPrimaryViewConfigurationTypes, PrimaryViewConfigurationType)) {
        throw std::logic_error("The system doesn't support required primary view configuration.");
    }


    // Request a form factor from the device (HMD, Handheld, etc.)
    //XrSystemGetInfo systemInfo = { XR_TYPE_SYSTEM_GET_INFO };
    //systemInfo.formFactor = app_config_form;
    //xrGetSystem(xr_instance, &systemInfo, &system.Id);

    // Check what blend mode is valid for this device (opaque vs transparent displays)
    // We'll just take the first one available!
    uint32_t blend_count = 0;
    xrEnumerateEnvironmentBlendModes(xr_instance, system.Id, app_config_view, 1, &blend_count, &xr_blend);

    // OpenXR wants to ensure apps are using the correct graphics card, so this MUST be called 
    // before xrCreateSession. This is crucial on devices that have multiple graphics cards, 
    // like laptops with integrated graphics chips in addition to dedicated graphics cards.


    auto [d3d11Binding, device, deviceContext] = DX::CreateD3D11Binding(
        xr_instance, 
        system.Id, 
        extensions,
        false, //m_appConfiguration.SingleThreadedD3D11Device, 
        SupportedFeatureLevels);



    //XrGraphicsRequirementsD3D11KHR graphicsRequirements{ XR_TYPE_GRAPHICS_REQUIREMENTS_D3D11_KHR };
    //CHECK_XRCMD(extensions.xrGetD3D11GraphicsRequirementsKHR(xr_instance, system.Id, &graphicsRequirements));

    //const winrt::com_ptr<IDXGIAdapter1> adapter = DX::GetAdapter(graphicsRequirements.adapterLuid);

    //if (adapter == nullptr)
        //return false;
    //support d2d
    //UINT creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

    //D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_0 };
    //winrt::com_ptr<ID3D11Device> device{ nullptr };
    //winrt::com_ptr<ID3D11DeviceContext> context{ nullptr };
    //if (FAILED(D3D11CreateDevice(
    //    adapter.get(),
    //    D3D_DRIVER_TYPE_UNKNOWN,
    //    0,
    //    creationFlags,
    //    featureLevels,
    //    _countof(featureLevels),
    //    D3D11_SDK_VERSION,
    //    device.put(),
    //    &m_d3dFeatureLevel,
    //    context.put())))
    //    return false;
    device.try_as(m_d3dDevice);
    deviceContext.try_as(m_d3dContext);
    //// Create the Direct2D device object and a corresponding context.
    winrt::com_ptr<IDXGIDevice3> dxgiDevice{ nullptr };
    device.try_as(dxgiDevice);
    auto hr = m_d2dFactory->CreateDevice(dxgiDevice.get(), m_d2dDevice.put());
    if (FAILED(hr)) {
        throw Platform::Exception::CreateException(hr);
    }
    m_d2dDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, m_d2dContext.put());

    //adapter->Release();

    xr::SessionHandle sessionHandle;
    XrSessionCreateInfo sessionCreateInfo{ XR_TYPE_SESSION_CREATE_INFO, nullptr, 0, system.Id };

    xr::InsertExtensionStruct(sessionCreateInfo, d3d11Binding);

    //XrHolographicWindowAttachmentMSFT holographicWindowAttachment;
    //if (m_appConfiguration.HolographicWindowAttachment.has_value() && extensions.SupportsHolographicWindowAttachment) {
    //    holographicWindowAttachment = m_appConfiguration.HolographicWindowAttachment.value();
    //    xr::InsertExtensionStruct(sessionCreateInfo, holographicWindowAttachment);
    //}

    CHECK_XRCMD(xrCreateSession(instance.Handle, &sessionCreateInfo, sessionHandle.Put()));

    xr::SessionContext session(std::move(sessionHandle),
        system,
        extensions,
        PrimaryViewConfigurationType,
        SupportedViewConfigurationTypes, // enable all supported secondary view config
        SupportedColorSwapchainFormats,
        SupportedDepthSwapchainFormats);

    m_context = std::make_unique<xr::XrContext>(
        std::move(instance),
        std::move(extensions),
        std::move(system),
        std::move(session)
        //m_appSpace.Get(),
        //std::move(pbrResources),
        //device,
        //deviceContext
        );

    auto xr_session = session.Handle;
    // A session represents this application's desire to display things! This is where we hook up our graphics API.
    // This does not start the session, for that, you'll need a call to xrBeginSession, which we do in openxr_poll_events
    //XrGraphicsBindingD3D11KHR binding = { XR_TYPE_GRAPHICS_BINDING_D3D11_KHR };
    //binding.device = device.get();
    //XrSessionCreateInfo sessionInfo = { XR_TYPE_SESSION_CREATE_INFO };
    //sessionInfo.next = &binding;
    //sessionInfo.systemId = system.Id;
    //xrCreateSession(xr_instance, &sessionInfo, &xr_session);

    // Initialize view configuration properties and environment blend modes

    uint32_t viewConfigurationCount = 0;
    xrEnumerateViewConfigurations(xr_instance, system.Id, 0, &viewConfigurationCount, nullptr);

    std::vector<XrViewConfigurationType> systemSupportedViewConfigurationTypes(viewConfigurationCount);
    xrEnumerateViewConfigurations(xr_instance, system.Id, viewConfigurationCount, &viewConfigurationCount, systemSupportedViewConfigurationTypes.data());

    for (const auto viewConfigType : SupportedViewConfigurationTypes) {
        if (!Contains(systemSupportedViewConfigurationTypes, viewConfigType)) {
            continue; // The system doesn't support this view configuration
        }

        //auto viewProperties = xr::CreateViewProperties(xr_instance, system.Id, viewConfigType, SupportedEnvironmentBlendModes);
        XrViewConfigurationProperties viewConfigProperties{ XR_TYPE_VIEW_CONFIGURATION_PROPERTIES };
        xrGetViewConfigurationProperties(xr_instance, system.Id, viewConfigType, &viewConfigProperties);

        ViewProperties viewProperties{};
        viewProperties.Type = viewConfigType;
        viewProperties.FovMutable = viewConfigProperties.fovMutable;

        uint32_t blendModeCount;
        xrEnumerateEnvironmentBlendModes(xr_instance, system.Id, viewConfigType, 0, &blendModeCount, nullptr);
        std::vector<XrEnvironmentBlendMode> blendModes(blendModeCount);
        xrEnumerateEnvironmentBlendModes(xr_instance, system.Id, viewConfigType, blendModeCount, &blendModeCount, blendModes.data());
        viewProperties.SupportedBlendModes = blendModes;

        auto blendModeIt = std::find_first_of(viewProperties.SupportedBlendModes.begin(),
            viewProperties.SupportedBlendModes.end(),
            SupportedEnvironmentBlendModes.begin(),
            SupportedEnvironmentBlendModes.end());
        if (blendModeIt == std::end(viewProperties.SupportedBlendModes)) {
            throw std::runtime_error("No blend modes supported");
        }
        viewProperties.BlendMode = *blendModeIt;

        if (viewProperties.SupportedBlendModes.size() > 0) {
            m_viewProperties.emplace(viewConfigType, viewProperties);

            if (viewConfigType == XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO ||
                viewConfigType == XR_VIEW_CONFIGURATION_TYPE_PRIMARY_MONO ||
                viewConfigType == XR_VIEW_CONFIGURATION_TYPE_PRIMARY_QUAD_VARJO) {
                SupportedPrimaryViewConfigurationTypes.push_back(viewConfigType);
            }
            else {
                SupportedSecondaryViewConfigurationTypes.push_back(viewConfigType);
            }
        }
    }

    for (const auto secondaryViewConfigurationType : SupportedViewConfigurationTypes) {
        if (!Contains(SupportedSecondaryViewConfigurationTypes, secondaryViewConfigurationType)) {
            continue; // Not supported by the system
        }
        EnabledSecondaryViewConfigurationTypes.push_back(secondaryViewConfigurationType);
    }

    // Unable to start a session, may not have an MR device attached or ready
    if (xr_session == nullptr)
        return false;

    std::vector<XrViewConfigurationType> AllViewConfigurationType;
    AllViewConfigurationType.push_back(PrimaryViewConfigurationType);
    AllViewConfigurationType.insert(AllViewConfigurationType.end(),
        EnabledSecondaryViewConfigurationTypes.begin(),
        EnabledSecondaryViewConfigurationTypes.end());

    for (const auto& viewConfigurationType : AllViewConfigurationType) {
        uint32_t viewCount;
        xrEnumerateViewConfigurationViews(xr_instance, system.Id, viewConfigurationType, 0, &viewCount, nullptr);

        std::vector<XrViewConfigurationView> newViewConfigViews(viewCount, { XR_TYPE_VIEW_CONFIGURATION_VIEW });
        xrEnumerateViewConfigurationViews(xr_instance, system.Id, viewConfigurationType, (uint32_t)newViewConfigViews.size(), &viewCount, newViewConfigViews.data());

        ViewConfigurationState state{};
        state.Type = viewConfigurationType;
        state.ViewConfigViews = newViewConfigViews;
        state.Views.resize(state.ViewConfigViews.size(), { XR_TYPE_VIEW });

        m_viewConfigStates.emplace(viewConfigurationType, state);
    }

    // OpenXR uses a couple different types of reference frames for positioning content, we need to choose one for
    // displaying our content! STAGE would be relative to the center of your guardian system's bounds, and LOCAL
    // would be relative to your device's starting location. HoloLens doesn't have a STAGE, so we'll use LOCAL.
    XrReferenceSpaceCreateInfo ref_space = { XR_TYPE_REFERENCE_SPACE_CREATE_INFO };
    ref_space.poseInReferenceSpace = xr_pose_identity;
    ref_space.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_UNBOUNDED_MSFT;//XR_REFERENCE_SPACE_TYPE_LOCAL;
    xrCreateReferenceSpace(xr_session, &ref_space, &xr_app_space);

    auto locator = SpatialLocator::GetDefault();
    referenceFrame = locator.CreateStationaryFrameOfReferenceAtCurrentLocation().CoordinateSystem();

    {
        // Now we need to find all the viewpoints we need to take care of! For a stereo headset, this should be 2.
        // Similarly, for an AR phone, we'll need 1, and a VR cave could have 6, or even 12!
        uint32_t view_count = 0;
        xrEnumerateViewConfigurationViews(xr_instance, system.Id, app_config_view, 0, &view_count, nullptr);
        xr_config_views.resize(view_count, { XR_TYPE_VIEW_CONFIGURATION_VIEW });
        xr_views.resize(view_count, { XR_TYPE_VIEW });
        xrEnumerateViewConfigurationViews(xr_instance, system.Id, app_config_view, view_count, &view_count, xr_config_views.data());
        for (uint32_t i = 0; i < view_count; i++) {
            // Create a swapchain for this viewpoint! A swapchain is a set of texture buffers used for displaying to screen,
            // typically this is a backbuffer and a front buffer, one for rendering data to, and one for displaying on-screen.
            // A note about swapchain image format here! OpenXR doesn't create a concrete image format for the texture, like 
            // DXGI_FORMAT_R8G8B8A8_UNORM. Instead, it switches to the TYPELESS variant of the provided texture format, like 
            // DXGI_FORMAT_R8G8B8A8_TYPELESS. When creating an ID3D11RenderTargetView for the swapchain texture, we must specify
            // a concrete type like DXGI_FORMAT_R8G8B8A8_UNORM, as attempting to create a TYPELESS view will throw errors, so 
            // we do need to store the format separately and remember it later.
            XrViewConfigurationView& view = xr_config_views[i];
            {
                XrSwapchainCreateInfo    swapchain_info = { XR_TYPE_SWAPCHAIN_CREATE_INFO };
                XrSwapchain              handle;
                swapchain_info.arraySize = 1;
                swapchain_info.mipCount = 1;
                swapchain_info.faceCount = 1;
                swapchain_info.format = d3d_swapchain_fmt;
                swapchain_info.width = view.recommendedImageRectWidth;
                swapchain_info.height = view.recommendedImageRectHeight;
                swapchain_info.sampleCount = view.recommendedSwapchainSampleCount;
                swapchain_info.usageFlags = XR_SWAPCHAIN_USAGE_SAMPLED_BIT | XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;

                XrSecondaryViewConfigurationSwapchainCreateInfoMSFT secondaryViewConfigCreateInfo{ XR_TYPE_SECONDARY_VIEW_CONFIGURATION_SWAPCHAIN_CREATE_INFO_MSFT };
                secondaryViewConfigCreateInfo.viewConfigurationType = app_config_view;
                swapchain_info.next = &secondaryViewConfigCreateInfo;

                xrCreateSwapchain(xr_session, &swapchain_info, &handle);

                // Find out how many textures were generated for the swapchain
                uint32_t surface_count = 0;
                xrEnumerateSwapchainImages(handle, 0, &surface_count, nullptr);

                // We'll want to track our own information about the swapchain, so we can draw stuff onto it! We'll also create
                // a depth buffer for each generated texture here as well with make_surfacedata.
                DX::swapchain_t swapchain = {};
                swapchain.width = swapchain_info.width;
                swapchain.height = swapchain_info.height;
                swapchain.handle = handle;
                swapchain.surface_images.resize(surface_count, { XR_TYPE_SWAPCHAIN_IMAGE_D3D11_KHR });
                //swapchain.surface_data.resize(surface_count);
                xrEnumerateSwapchainImages(swapchain.handle, surface_count, &surface_count, (XrSwapchainImageBaseHeader*)swapchain.surface_images.data());

                xr_swapchains.push_back(swapchain);
            }

            //for (uint32_t i = 0; i < surface_count; i++) {
            //  swapchain.surface_data[i] = d3d_make_surface_data((XrBaseInStructure&)swapchain.surface_images[i]);
            //}
            {
                XrSwapchainCreateInfo    swapchain_info = { XR_TYPE_SWAPCHAIN_CREATE_INFO };
                XrSwapchain              handle;
                swapchain_info.arraySize = 1;
                swapchain_info.mipCount = 1;
                swapchain_info.faceCount = 1;
                swapchain_info.format = DXGI_FORMAT_D16_UNORM;
                swapchain_info.width = view.recommendedImageRectWidth;
                swapchain_info.height = view.recommendedImageRectHeight;
                swapchain_info.sampleCount = view.recommendedSwapchainSampleCount;
                swapchain_info.usageFlags = XR_SWAPCHAIN_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

                XrSecondaryViewConfigurationSwapchainCreateInfoMSFT secondaryViewConfigCreateInfo{ XR_TYPE_SECONDARY_VIEW_CONFIGURATION_SWAPCHAIN_CREATE_INFO_MSFT };
                secondaryViewConfigCreateInfo.viewConfigurationType = app_config_view;
                swapchain_info.next = &secondaryViewConfigCreateInfo;

                xrCreateSwapchain(xr_session, &swapchain_info, &handle);

                // Find out how many textures were generated for the swapchain
                uint32_t surface_count = 0;
                xrEnumerateSwapchainImages(handle, 0, &surface_count, nullptr);

                // We'll want to track our own information about the swapchain, so we can draw stuff onto it! We'll also create
                // a depth buffer for each generated texture here as well with make_surfacedata.
                DX::swapchain_t swapchain = {};
                swapchain.width = swapchain_info.width;
                swapchain.height = swapchain_info.height;
                swapchain.handle = handle;
                swapchain.surface_images.resize(surface_count, { XR_TYPE_SWAPCHAIN_IMAGE_D3D11_KHR });
                //swapchain.surface_data.resize(surface_count);
                xrEnumerateSwapchainImages(swapchain.handle, surface_count, &surface_count, (XrSwapchainImageBaseHeader*)swapchain.surface_images.data());

                xr_depth_swapchains.push_back(swapchain);
            }



            {
                CD3D11_DEPTH_STENCIL_DESC depthStencilDesc(CD3D11_DEFAULT{});
                depthStencilDesc.StencilEnable = false;
                depthStencilDesc.DepthEnable = true;
                depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
                depthStencilDesc.DepthFunc = D3D11_COMPARISON_GREATER;
                winrt::com_ptr<ID3D11DepthStencilState> m_reversedZDepthNoStencilTest;
                m_reversedZDepthNoStencilTest = nullptr;
                device->CreateDepthStencilState(&depthStencilDesc, m_reversedZDepthNoStencilTest.put());
            }
        }
    }

    {
        // Secondary 
        uint32_t view_count = 0;
        xrEnumerateViewConfigurationViews(xr_instance, system.Id, EnabledSecondaryViewConfigurationTypes[0], 0, &view_count, nullptr);
        xr_secondary_config_views.resize(view_count, { XR_TYPE_VIEW_CONFIGURATION_VIEW });
        xr_secondary_views.resize(view_count, { XR_TYPE_VIEW });
        xrEnumerateViewConfigurationViews(xr_instance, system.Id, EnabledSecondaryViewConfigurationTypes[0], view_count, &view_count, xr_secondary_config_views.data());
        for (uint32_t i = 0; i < view_count; i++) {
            // Create a swapchain for this viewpoint! A swapchain is a set of texture buffers used for displaying to screen,
            // typically this is a backbuffer and a front buffer, one for rendering data to, and one for displaying on-screen.
            // A note about swapchain image format here! OpenXR doesn't create a concrete image format for the texture, like 
            // DXGI_FORMAT_R8G8B8A8_UNORM. Instead, it switches to the TYPELESS variant of the provided texture format, like 
            // DXGI_FORMAT_R8G8B8A8_TYPELESS. When creating an ID3D11RenderTargetView for the swapchain texture, we must specify
            // a concrete type like DXGI_FORMAT_R8G8B8A8_UNORM, as attempting to create a TYPELESS view will throw errors, so 
            // we do need to store the format separately and remember it later.
            XrViewConfigurationView& view = xr_secondary_config_views[i];
            {
                XrSwapchainCreateInfo    swapchain_info = { XR_TYPE_SWAPCHAIN_CREATE_INFO };
                XrSwapchain              handle;
                swapchain_info.arraySize = 1;
                swapchain_info.mipCount = 1;
                swapchain_info.faceCount = 1;
                swapchain_info.format = d3d_swapchain_fmt;
                swapchain_info.width = view.recommendedImageRectWidth;
                swapchain_info.height = view.recommendedImageRectHeight;
                swapchain_info.sampleCount = view.recommendedSwapchainSampleCount;
                swapchain_info.usageFlags = XR_SWAPCHAIN_USAGE_SAMPLED_BIT | XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;
                XrSecondaryViewConfigurationSwapchainCreateInfoMSFT secondaryViewConfigCreateInfo{ XR_TYPE_SECONDARY_VIEW_CONFIGURATION_SWAPCHAIN_CREATE_INFO_MSFT };
                secondaryViewConfigCreateInfo.viewConfigurationType = EnabledSecondaryViewConfigurationTypes[0];
                swapchain_info.next = &secondaryViewConfigCreateInfo;


                xrCreateSwapchain(xr_session, &swapchain_info, &handle);

                // Find out how many textures were generated for the swapchain
                uint32_t surface_count = 0;
                xrEnumerateSwapchainImages(handle, 0, &surface_count, nullptr);

                // We'll want to track our own information about the swapchain, so we can draw stuff onto it! We'll also create
                // a depth buffer for each generated texture here as well with make_surfacedata.
                DX::swapchain_t swapchain = {};
                swapchain.width = swapchain_info.width;
                swapchain.height = swapchain_info.height;
                swapchain.handle = handle;
                swapchain.surface_images.resize(surface_count, { XR_TYPE_SWAPCHAIN_IMAGE_D3D11_KHR });
                //swapchain.surface_data.resize(surface_count);
                xrEnumerateSwapchainImages(swapchain.handle, surface_count, &surface_count, (XrSwapchainImageBaseHeader*)swapchain.surface_images.data());

                xr_secondary_swapchains.push_back(swapchain);
            }

            {
                XrSwapchainCreateInfo    swapchain_info = { XR_TYPE_SWAPCHAIN_CREATE_INFO };
                XrSwapchain              handle;
                swapchain_info.arraySize = 1;
                swapchain_info.mipCount = 1;
                swapchain_info.faceCount = 1;
                swapchain_info.format = DXGI_FORMAT_D16_UNORM;
                swapchain_info.width = view.recommendedImageRectWidth;
                swapchain_info.height = view.recommendedImageRectHeight;
                swapchain_info.sampleCount = view.recommendedSwapchainSampleCount;
                swapchain_info.usageFlags = XR_SWAPCHAIN_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
                XrSecondaryViewConfigurationSwapchainCreateInfoMSFT secondaryViewConfigCreateInfo{ XR_TYPE_SECONDARY_VIEW_CONFIGURATION_SWAPCHAIN_CREATE_INFO_MSFT };
                secondaryViewConfigCreateInfo.viewConfigurationType = EnabledSecondaryViewConfigurationTypes[0];
                swapchain_info.next = &secondaryViewConfigCreateInfo;

                xrCreateSwapchain(xr_session, &swapchain_info, &handle);

                // Find out how many textures were generated for the swapchain
                uint32_t surface_count = 0;
                xrEnumerateSwapchainImages(handle, 0, &surface_count, nullptr);

                // We'll want to track our own information about the swapchain, so we can draw stuff onto it! We'll also create
                // a depth buffer for each generated texture here as well with make_surfacedata.
                DX::swapchain_t swapchain = {};
                swapchain.width = swapchain_info.width;
                swapchain.height = swapchain_info.height;
                swapchain.handle = handle;
                swapchain.surface_images.resize(surface_count, { XR_TYPE_SWAPCHAIN_IMAGE_D3D11_KHR });
                //swapchain.surface_data.resize(surface_count);
                xrEnumerateSwapchainImages(swapchain.handle, surface_count, &surface_count, (XrSwapchainImageBaseHeader*)swapchain.surface_images.data());

                xr_secondary_depth_swapchains.push_back(swapchain);
            }
            //for (uint32_t i = 0; i < surface_count; i++) {
            //	swapchain.surface_data[i] = d3d_make_surface_data((XrBaseInStructure&)swapchain.surface_images[i]);
            //}


            {
                CD3D11_DEPTH_STENCIL_DESC depthStencilDesc(CD3D11_DEFAULT{});
                depthStencilDesc.StencilEnable = false;
                depthStencilDesc.DepthEnable = true;
                depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
                depthStencilDesc.DepthFunc = D3D11_COMPARISON_GREATER;
                winrt::com_ptr<ID3D11DepthStencilState> m_reversedZDepthNoStencilTest;
                m_reversedZDepthNoStencilTest = nullptr;
                device->CreateDepthStencilState(&depthStencilDesc, m_reversedZDepthNoStencilTest.put());
            }
        }
    }

    return true;
}
void OXRManager::InitOxrActions() {
    auto xr_instance = XrContext().Instance.Handle;
    auto xr_session = XrContext().Session.Handle;

    XrActionSetCreateInfo actionset_info = { XR_TYPE_ACTION_SET_CREATE_INFO };
    strcpy_s(actionset_info.actionSetName, "gameplay");
    strcpy_s(actionset_info.localizedActionSetName, "Gameplay");
    xrCreateActionSet(xr_instance, &actionset_info, &xr_input.actionSet);
    xrStringToPath(xr_instance, "/user/hand/left", &xr_input.handSubactionPath[0]);
    xrStringToPath(xr_instance, "/user/hand/right", &xr_input.handSubactionPath[1]);

    // Create an action to track the position and orientation of the hands! This is
    // the controller location, or the center of the palms for actual hands.
    XrActionCreateInfo action_info = { XR_TYPE_ACTION_CREATE_INFO };
    action_info.countSubactionPaths = _countof(xr_input.handSubactionPath);
    action_info.subactionPaths = xr_input.handSubactionPath;
    action_info.actionType = XR_ACTION_TYPE_POSE_INPUT;
    strcpy_s(action_info.actionName, "hand_pose");
    strcpy_s(action_info.localizedActionName, "Hand Pose");
    xrCreateAction(xr_input.actionSet, &action_info, &xr_input.poseAction);

    // Create an action for listening to the select action! This is primary trigger
    // on controllers, and an airtap on HoloLens
    action_info.actionType = XR_ACTION_TYPE_BOOLEAN_INPUT;
    strcpy_s(action_info.actionName, "select");
    strcpy_s(action_info.localizedActionName, "Select");
    xrCreateAction(xr_input.actionSet, &action_info, &xr_input.selectAction);

    // Bind the actions we just created to specific locations on the Khronos simple_controller
    // definition! These are labeled as 'suggested' because they may be overridden by the runtime
    // preferences. For example, if the runtime allows you to remap buttons, or provides input
    // accessibility settings.
    XrPath profile_path;
    XrPath pose_path[2];
    XrPath select_path[2];
    xrStringToPath(xr_instance, "/user/hand/left/input/grip/pose", &pose_path[0]);
    xrStringToPath(xr_instance, "/user/hand/right/input/grip/pose", &pose_path[1]);
    xrStringToPath(xr_instance, "/user/hand/left/input/select/click", &select_path[0]);
    xrStringToPath(xr_instance, "/user/hand/right/input/select/click", &select_path[1]);
    xrStringToPath(xr_instance, "/interaction_profiles/khr/simple_controller", &profile_path);
    XrActionSuggestedBinding bindings[] = {
      { xr_input.poseAction,   pose_path[0]   },
      { xr_input.poseAction,   pose_path[1]   },
      { xr_input.selectAction, select_path[0] },
      { xr_input.selectAction, select_path[1] }, };
    XrInteractionProfileSuggestedBinding suggested_binds = { XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING };
    suggested_binds.interactionProfile = profile_path;
    suggested_binds.suggestedBindings = &bindings[0];
    suggested_binds.countSuggestedBindings = _countof(bindings);
    xrSuggestInteractionProfileBindings(xr_instance, &suggested_binds);

    // Create frames of reference for the pose actions
    for (int32_t i = 0; i < 2; i++) {
        XrActionSpaceCreateInfo action_space_info = { XR_TYPE_ACTION_SPACE_CREATE_INFO };
        action_space_info.action = xr_input.poseAction;
        action_space_info.poseInActionSpace = xr_pose_identity;
        action_space_info.subactionPath = xr_input.handSubactionPath[i];
        xrCreateActionSpace(xr_session, &action_space_info, &xr_input.handSpace[i]);
    }

    // Attach the action set we just made to the session
    XrSessionActionSetsAttachInfo attach_info = { XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO };
    attach_info.countActionSets = 1;
    attach_info.actionSets = &xr_input.actionSet;
    xrAttachSessionActionSets(xr_session, &attach_info);
    lastFrameState = { XR_TYPE_FRAME_STATE };

}
bool OXRManager::Update() {
    openxr_poll_events();
    if (xr_running) {
        openxr_poll_actions();
        if (xr_session_state != XR_SESSION_STATE_VISIBLE &&
            xr_session_state != XR_SESSION_STATE_FOCUSED) {
            std::this_thread::sleep_for(std::chrono::milliseconds(250));
        }
    }
    return !xr_quit;
}
void OXRManager::Render(OXRScenes* scene) {
    // Block until the previous frame is finished displaying, and is ready for another one.
    // Also returns a prediction of when the next frame will be displayed, for use with predicting
    // locations of controllers, viewpoints, etc.
    XrFrameState frame_state = { XR_TYPE_FRAME_STATE };
    auto xr_session = XrContext().Session.Handle;

    // secondaryViewConfigFrameState needs to have the same lifetime as frameState
    XrSecondaryViewConfigurationFrameStateMSFT secondaryViewConfigFrameState{ XR_TYPE_SECONDARY_VIEW_CONFIGURATION_FRAME_STATE_MSFT };

    const size_t enabledSecondaryViewConfigCount = EnabledSecondaryViewConfigurationTypes.size();
    std::vector<XrSecondaryViewConfigurationStateMSFT> secondaryViewConfigStates(enabledSecondaryViewConfigCount,
        { XR_TYPE_SECONDARY_VIEW_CONFIGURATION_STATE_MSFT });

    if (SupportsSecondaryViewConfiguration && enabledSecondaryViewConfigCount > 0) {
        secondaryViewConfigFrameState.viewConfigurationCount = (uint32_t)secondaryViewConfigStates.size();
        secondaryViewConfigFrameState.viewConfigurationStates = secondaryViewConfigStates.data();
        secondaryViewConfigFrameState.next = frame_state.next;
        frame_state.next = &secondaryViewConfigFrameState;
    }

    XrFrameWaitInfo waitFrameInfo{ XR_TYPE_FRAME_WAIT_INFO };
    xrWaitFrame(xr_session, &waitFrameInfo, &frame_state);

    if (SupportsSecondaryViewConfiguration) {
        std::scoped_lock lock(m_secondaryViewConfigActiveMutex);
        m_secondaryViewConfigurationsState = std::move(secondaryViewConfigStates);
    }
    // Must be called before any rendering is done! This can return some interesting flags, like 
    // XR_SESSION_VISIBILITY_UNAVAILABLE, which means we could skip rendering this frame and call
    // xrEndFrame right away.
    lastFrameState = frame_state;
    XrFrameBeginInfo beginFrameDescription{ XR_TYPE_FRAME_BEGIN_INFO };
    XrFrameEndInfo end_info{ XR_TYPE_FRAME_END_INFO };
    xrBeginFrame(xr_session, &beginFrameDescription);

    end_info.environmentBlendMode = m_viewProperties.at(app_config_view).BlendMode;
    end_info.displayTime = frame_state.predictedDisplayTime;

    if (SupportsSecondaryViewConfiguration) {
        std::scoped_lock lock(m_secondaryViewConfigActiveMutex);
        for (auto& state : m_secondaryViewConfigurationsState) {
            SetSecondaryViewConfigurationActive(m_viewConfigStates.at(state.viewConfigurationType), state.active);
        }
    }

    // Secondary view config frame info need to have same lifetime as XrFrameEndInfo;
    XrSecondaryViewConfigurationFrameEndInfoMSFT frameEndSecondaryViewConfigInfo{
        XR_TYPE_SECONDARY_VIEW_CONFIGURATION_FRAME_END_INFO_MSFT };
    std::vector<XrSecondaryViewConfigurationLayerInfoMSFT> activeSecondaryViewConfigLayerInfos;

    // Chain secondary view configuration layers data to endFrameInfo
    if (SupportsSecondaryViewConfiguration &&
        EnabledSecondaryViewConfigurationTypes.size() > 0) {
        for (auto& secondaryViewConfigType : EnabledSecondaryViewConfigurationTypes) {
            auto& secondaryViewConfig = m_viewConfigStates.at(secondaryViewConfigType);
            if (secondaryViewConfig.Active) {
                activeSecondaryViewConfigLayerInfos.emplace_back(
                    XrSecondaryViewConfigurationLayerInfoMSFT{ XR_TYPE_SECONDARY_VIEW_CONFIGURATION_LAYER_INFO_MSFT,
                                                              nullptr,
                                                              secondaryViewConfigType,
                                                              m_viewProperties.at(secondaryViewConfigType).BlendMode });
            }
        }

        if (activeSecondaryViewConfigLayerInfos.size() > 0) {
            frameEndSecondaryViewConfigInfo.viewConfigurationCount = (uint32_t)activeSecondaryViewConfigLayerInfos.size();
            frameEndSecondaryViewConfigInfo.viewConfigurationLayersInfo = activeSecondaryViewConfigLayerInfos.data();
            frameEndSecondaryViewConfigInfo.next = end_info.next;
            end_info.next = &frameEndSecondaryViewConfigInfo;
            render_for_MRC = true;
        }
        else {
            render_for_MRC = false;
        }
    }

    // Execute any code that's dependant on the predicted time, such as updating the location of
    // controller models.
    //openxr_poll_predicted(frame_state.predictedDisplayTime);
    //app_update_predicted();

    // If the session is active, lets render our layer in the compositor!
    // Primary render
    XrCompositionLayerBaseHeader* layer = nullptr;
    XrCompositionLayerProjection             layer_proj = { XR_TYPE_COMPOSITION_LAYER_PROJECTION };
    std::vector<XrCompositionLayerProjectionView> views;
    std::vector<XrCompositionLayerDepthInfoKHR> depthInfo;
    bool session_active = xr_session_state == XR_SESSION_STATE_VISIBLE || xr_session_state == XR_SESSION_STATE_FOCUSED;

    if (session_active && openxr_render_layer(frame_state.predictedDisplayTime, views, depthInfo, layer_proj, scene)) {
        layer = (XrCompositionLayerBaseHeader*)&layer_proj;
    }

    //{
    //	XrCompositionLayerBaseHeader* layer = nullptr;
    //	XrCompositionLayerProjection             layer_proj = { XR_TYPE_COMPOSITION_LAYER_PROJECTION };
    //	std::vector<XrCompositionLayerProjectionView> views;
    //  if (openxr_render_layer(frame_state.predictedDisplayTime, views, layer_proj, scene, true)) {
    //	  layer = (XrCompositionLayerBaseHeader*)&layer_proj;
    //  }
   // }

    std::vector<std::vector<XrCompositionLayerProjectionView>> views_array;
    std::vector<std::vector<XrCompositionLayerDepthInfoKHR>> depthInfo_array;
    std::vector<XrCompositionLayerProjection> layer_proj_array;
    std::vector<XrCompositionLayerBaseHeader*> layer_array;
    // Secondary render
    if (SupportsSecondaryViewConfiguration && activeSecondaryViewConfigLayerInfos.size() > 0) {
        views_array.resize(activeSecondaryViewConfigLayerInfos.size());
        depthInfo_array.resize(activeSecondaryViewConfigLayerInfos.size());
        layer_proj_array.resize(activeSecondaryViewConfigLayerInfos.size());
        layer_array.resize(activeSecondaryViewConfigLayerInfos.size());
        for (size_t i = 0; i < activeSecondaryViewConfigLayerInfos.size(); i++) {
            layer_array[i] = nullptr;
            XrCompositionLayerProjection& layer_proj = layer_proj_array[i];
            layer_proj = { XR_TYPE_COMPOSITION_LAYER_PROJECTION };

            auto& views = views_array[i];
            auto& depthInfo = depthInfo_array[i];

            XrSecondaryViewConfigurationLayerInfoMSFT& secondaryViewConfigLayerInfo = activeSecondaryViewConfigLayerInfos.at(i);
            //engine::CompositionLayers& secondaryViewConfigLayers = layersForAllViewConfigs.at(i + 1);
            if (openxr_render_layer(frame_state.predictedDisplayTime, views, depthInfo, layer_proj, scene, true)) {
                layer_array[i] = (XrCompositionLayerBaseHeader*)&layer_proj;
            }
            //RenderViewConfiguration(sceneLock, secondaryViewConfigLayerInfo.viewConfigurationType, secondaryViewConfigLayers);
            secondaryViewConfigLayerInfo.layerCount = layer_array[i] == nullptr ? 0 : 1;
            secondaryViewConfigLayerInfo.layers = &(layer_array[i]);
        }
    }

    // We're finished with rendering our layer, so send it off for display!
    end_info.displayTime = frame_state.predictedDisplayTime;
    end_info.environmentBlendMode = xr_blend;
    end_info.layerCount = layer == nullptr ? 0 : 1;
    end_info.layers = &layer;
    xrEndFrame(xr_session, &end_info);
}
void OXRManager::ShutDown() {
    auto xr_session = XrContext().Session.Handle;
    auto m_d3dContext = XrContext().DeviceContext;
    auto m_d3dDevice = XrContext().Device;
    // We used a graphics API to initialize the swapchain data, so we'll
    // give it a chance to release anythig here!
    for (int32_t i = 0; i < xr_swapchains.size(); i++) {
        xrDestroySwapchain(xr_swapchains[i].handle);
        for (uint32_t j = 0; j < xr_swapchains[i].surface_data.size(); j++) {
            xr_swapchains[i].surface_data[j].depth_view->Release();
            xr_swapchains[i].surface_data[j].target_view->Release();
        }
    }
    xr_swapchains.clear();

    // Release all the other OpenXR resources that we've created!
    // What gets allocated, must get deallocated!
    if (xr_input.actionSet != XR_NULL_HANDLE) {
        if (xr_input.handSpace[0] != XR_NULL_HANDLE) xrDestroySpace(xr_input.handSpace[0]);
        if (xr_input.handSpace[1] != XR_NULL_HANDLE) xrDestroySpace(xr_input.handSpace[1]);
        xrDestroyActionSet(xr_input.actionSet);
    }
    if (xr_app_space != XR_NULL_HANDLE) xrDestroySpace(xr_app_space);
    if (xr_session != XR_NULL_HANDLE) xrDestroySession(xr_session);
    if (xr_debug != XR_NULL_HANDLE) ext_xrDestroyDebugUtilsMessengerEXT(xr_debug);
    auto xr_instance = XrContext().Instance.Handle;

    if (xr_instance != XR_NULL_HANDLE) xrDestroyInstance(xr_instance);

    if (m_d3dContext.get()) { m_d3dContext.get()->Release(); m_d3dContext = nullptr; }
    if (m_d3dDevice.get()) { m_d3dDevice.get()->Release();  m_d3dDevice = nullptr; }
}

XrSpace DX::OXRManager::createReferenceSpace(XrReferenceSpaceType referenceSpaceType, XrPosef poseInReferenceSpace) {
    XrSpace space;
    auto xr_session = XrContext().Session.Handle;

    XrReferenceSpaceCreateInfo createInfo{ XR_TYPE_REFERENCE_SPACE_CREATE_INFO };
    createInfo.referenceSpaceType = referenceSpaceType;
    createInfo.poseInReferenceSpace = poseInReferenceSpace;
    xrCreateReferenceSpace(xr_session, &createInfo, &space);
    return space;
}

XrSpatialAnchorMSFT DX::OXRManager::createAnchor(const XrPosef& poseInScene)
{
    auto xr_session = XrContext().Session.Handle;

    XrSpatialAnchorMSFT anchor;
    // Anchors provide the best stability when moving beyond 5 meters, so if the extension is enabled,
    // create an anchor at given location and place the hologram at the resulting anchor space.
    XrSpatialAnchorCreateInfoMSFT createInfo{ XR_TYPE_SPATIAL_ANCHOR_CREATE_INFO_MSFT };
    createInfo.space = xr_app_space;
    createInfo.pose = poseInScene;
    XrResult result = ext_xrCreateSpatialAnchorMSFT(
        xr_session, &createInfo, &anchor);
    return anchor;
}

XrSpace DX::OXRManager::createAnchorSpace(const XrPosef& poseInScene)
{
    auto xr_session = XrContext().Session.Handle;
    XrSpatialAnchorMSFT anchor = createAnchor(poseInScene);
    XrSpace space;
    XrSpatialAnchorSpaceCreateInfoMSFT createSpaceInfo{ XR_TYPE_SPATIAL_ANCHOR_SPACE_CREATE_INFO_MSFT };
    createSpaceInfo.anchor = anchor;
    createSpaceInfo.poseInAnchorSpace = xr::math::Pose::Identity();
    ext_xrCreateSpatialAnchorSpaceMSFT(xr_session, &createSpaceInfo, &space);
    return space;
}

XrSpace* DX::OXRManager::getAppSpace() { return &xr_app_space; }

void OXRManager::openxr_poll_events() {
    auto xr_session = XrContext().Session.Handle;

    XrEventDataBuffer event_buffer = { XR_TYPE_EVENT_DATA_BUFFER };
    auto xr_instance = XrContext().Instance.Handle;

    while (xrPollEvent(xr_instance, &event_buffer) == XR_SUCCESS) {
        switch (event_buffer.type) {
        case XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED: {
            XrEventDataSessionStateChanged* changed = (XrEventDataSessionStateChanged*)&event_buffer;
            xr_session_state = changed->state;

            // Session state change is where we can begin and end sessions, as well as find quit messages!
            switch (xr_session_state) {
            case XR_SESSION_STATE_READY: {
                XrSessionBeginInfo begin_info = { XR_TYPE_SESSION_BEGIN_INFO };
                begin_info.primaryViewConfigurationType = app_config_view;

                XrSecondaryViewConfigurationSessionBeginInfoMSFT secondaryViewConfigInfo{
                    XR_TYPE_SECONDARY_VIEW_CONFIGURATION_SESSION_BEGIN_INFO_MSFT };
                if (SupportsSecondaryViewConfiguration &&
                    EnabledSecondaryViewConfigurationTypes.size() > 0) {
                    secondaryViewConfigInfo.viewConfigurationCount = (uint32_t)EnabledSecondaryViewConfigurationTypes.size();
                    secondaryViewConfigInfo.enabledViewConfigurationTypes = EnabledSecondaryViewConfigurationTypes.data();

                    secondaryViewConfigInfo.next = begin_info.next;
                    begin_info.next = &secondaryViewConfigInfo;
                }

                xrBeginSession(xr_session, &begin_info);
                xr_running = true;
            } break;
            case XR_SESSION_STATE_STOPPING: {
                xr_running = false;
                xrEndSession(xr_session);
            } break;
            case XR_SESSION_STATE_EXITING:      xr_quit = true; break;
            case XR_SESSION_STATE_LOSS_PENDING: xr_quit = true; break;
            }
        } break;
        case XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING: xr_quit = true; return;
        }
        event_buffer = { XR_TYPE_EVENT_DATA_BUFFER };
    }
}
void OXRManager::openxr_poll_actions() {
    if (xr_session_state != XR_SESSION_STATE_FOCUSED)
        return;

    // Update our action set with up-to-date input data!
    XrActiveActionSet action_set = { };
    action_set.actionSet = xr_input.actionSet;
    action_set.subactionPath = XR_NULL_PATH;

    XrActionsSyncInfo sync_info = { XR_TYPE_ACTIONS_SYNC_INFO };
    sync_info.countActiveActionSets = 1;
    sync_info.activeActionSets = &action_set;
    auto xr_session = XrContext().Session.Handle;

    xrSyncActions(xr_session, &sync_info);

    // Now we'll get the current states of our actions, and store them for later use
    for (uint32_t hand = 0; hand < 2; hand++) {
        XrActionStateGetInfo get_info = { XR_TYPE_ACTION_STATE_GET_INFO };
        get_info.subactionPath = xr_input.handSubactionPath[hand];

        XrActionStatePose pose_state = { XR_TYPE_ACTION_STATE_POSE };
        get_info.action = xr_input.poseAction;
        xrGetActionStatePose(xr_session, &get_info, &pose_state);
        xr_input.renderHand[hand] = pose_state.isActive;

        // Events come with a timestamp
        XrActionStateBoolean select_state = { XR_TYPE_ACTION_STATE_BOOLEAN };
        get_info.action = xr_input.selectAction;
        xrGetActionStateBoolean(xr_session, &get_info, &select_state);
        xr_input.handSelect[hand] = select_state.isActive && select_state.currentState && select_state.changedSinceLastSync;
        xr_input.handDeselect[hand] = select_state.isActive && !select_state.currentState && select_state.changedSinceLastSync;

        // Constantly update pose if isActive
        if (xr_input.renderHand[hand]) {
            XrSpaceLocation space_location = { XR_TYPE_SPACE_LOCATION };
            XrResult        res = xrLocateSpace(xr_input.handSpace[hand], xr_app_space, lastFrameState.predictedDisplayTime, &space_location);
            if (XR_UNQUALIFIED_SUCCESS(res) &&
                (space_location.locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT) != 0 &&
                (space_location.locationFlags & XR_SPACE_LOCATION_ORIENTATION_VALID_BIT) != 0) {
                xr_input.handPose[hand] = space_location.pose;
                float x = space_location.pose.position.x;
                float y = space_location.pose.position.y;
                float z = space_location.pose.position.z;

                glm::quat rot;
                rot.x = space_location.pose.orientation.x;
                rot.y = space_location.pose.orientation.y;
                rot.z = space_location.pose.orientation.z;
                rot.w = space_location.pose.orientation.w;
                glm::mat4 rotMat = glm::mat4_cast(rot);

                if (xr_input.handSelect[hand]) {
                    // If we have a select event, send onSingle3DTouchDown
                    onSingle3DTouchDown(x, y, z, hand);
                }
                else if (xr_input.handDeselect[hand]) {
                    // If we have a deselect event, send on3DTouchReleased
                    on3DTouchReleased(hand);
                }
                else {
                    // Send on3DTouchMove
                    on3DTouchMove(x, y, z, rotMat, hand);
                }

            }
        }
        else {

            // lose tracking = release
            on3DTouchReleased(hand);
        }


    }
}

swapchain_surfdata_t OXRManager::d3d_make_surface_data(XrBaseInStructure& swapchain_img) {
    DX::swapchain_surfdata_t result = {};

    // Get information about the swapchain image that OpenXR made for us!
    XrSwapchainImageD3D11KHR& d3d_swapchain_img = (XrSwapchainImageD3D11KHR&)swapchain_img;
    D3D11_TEXTURE2D_DESC      color_desc;
    d3d_swapchain_img.texture->GetDesc(&color_desc);

    // Create a view resource for the swapchain image target that we can use to set up rendering.
    D3D11_RENDER_TARGET_VIEW_DESC target_desc = {};
    target_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
    // NOTE: Why not use color_desc.Format? Check the notes over near the xrCreateSwapchain call!
    // Basically, the color_desc.Format of the OpenXR created swapchain is TYPELESS, but in order to
    // create a View for the texture, we need a concrete variant of the texture format like UNORM.
    target_desc.Format = (DXGI_FORMAT)d3d_swapchain_fmt;
    m_d3dDevice.get()->CreateRenderTargetView(d3d_swapchain_img.texture, &target_desc, &result.target_view);

    // Create a depth buffer that matches 
    ID3D11Texture2D* depth_texture;
    D3D11_TEXTURE2D_DESC depth_desc = {};
    depth_desc.SampleDesc.Count = 1;
    depth_desc.MipLevels = 1;
    depth_desc.Width = color_desc.Width;
    depth_desc.Height = color_desc.Height;
    depth_desc.ArraySize = color_desc.ArraySize;
    depth_desc.Format = DXGI_FORMAT_R32_TYPELESS;
    depth_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_DEPTH_STENCIL;
    m_d3dDevice.get()->CreateTexture2D(&depth_desc, nullptr, &depth_texture);

    // And create a view resource for the depth buffer, so we can set that up for rendering to as well!
    D3D11_DEPTH_STENCIL_VIEW_DESC stencil_desc = {};
    stencil_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    stencil_desc.Format = DXGI_FORMAT_D16_UNORM;
    m_d3dDevice.get()->CreateDepthStencilView(depth_texture, &stencil_desc, &result.depth_view);

    // We don't need direct access to the ID3D11Texture2D object anymore, we only need the view
    depth_texture->Release();

    return result;
}

bool OXRManager::openxr_render_layer(XrTime predictedTime,
    std::vector<XrCompositionLayerProjectionView>& views,
    std::vector<XrCompositionLayerDepthInfoKHR>& depthInfo,
    XrCompositionLayerProjection& layer,
    OXRScenes* scene, bool is_secondary) {
    auto xr_session = XrContext().Session.Handle;
    auto m_d3dContext = XrContext().DeviceContext;
    auto m_d3dDevice = XrContext().Device;
    // Find the state and location of each viewpoint at the predicted time

    uint32_t         view_count = 0;
    if (is_secondary)
    {
        view_count = 0;
        XrViewState      view_state = { XR_TYPE_VIEW_STATE };
        XrViewLocateInfo locate_info = { XR_TYPE_VIEW_LOCATE_INFO };
        locate_info.viewConfigurationType = SupportedSecondaryViewConfigurationTypes[0];
        locate_info.displayTime = predictedTime;
        locate_info.space = xr_app_space;

        xrLocateViews(xr_session, &locate_info, &view_state, (uint32_t)xr_secondary_views.size(), &view_count, xr_secondary_views.data());
        views.resize(view_count);
        depthInfo.resize(view_count);
    }
    else
    {
        view_count = 0;
        XrViewState      view_state = { XR_TYPE_VIEW_STATE };
        XrViewLocateInfo locate_info = { XR_TYPE_VIEW_LOCATE_INFO };
        locate_info.viewConfigurationType = app_config_view;
        locate_info.displayTime = predictedTime;
        locate_info.space = xr_app_space;
        xrLocateViews(xr_session, &locate_info, &view_state, (uint32_t)xr_views.size(), &view_count, xr_views.data());
        views.resize(view_count);
        depthInfo.resize(view_count);
    }



    // And now we'll iterate through each viewpoint, and render it!
    for (uint32_t i = 0; i < view_count; i++) {

        // We need to ask which swapchain image to use for rendering! Which one will we get?
        // Who knows! It's up to the runtime to decide.
        uint32_t                    img_id;
        uint32_t depth_img_id;
        XrSwapchainImageAcquireInfo acquire_info = { XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO };
        if (is_secondary) {
            xrAcquireSwapchainImage(xr_secondary_swapchains[i].handle, &acquire_info, &img_id);
            xrAcquireSwapchainImage(xr_secondary_depth_swapchains[i].handle, &acquire_info, &depth_img_id);
        }
        else {
            xrAcquireSwapchainImage(xr_swapchains[i].handle, &acquire_info, &img_id);
            xrAcquireSwapchainImage(xr_depth_swapchains[i].handle, &acquire_info, &depth_img_id);
        }

        // Wait until the image is available to render to. The compositor could still be
        // reading from it.
        XrSwapchainImageWaitInfo wait_info = { XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO };
        wait_info.timeout = XR_INFINITE_DURATION;
        XrResult colorSwapchainWait;
        XrResult depthSwapchainWait;
        if (is_secondary) {
            colorSwapchainWait = xrWaitSwapchainImage(xr_secondary_swapchains[i].handle, &wait_info);
            depthSwapchainWait = xrWaitSwapchainImage(xr_secondary_depth_swapchains[i].handle, &wait_info);
        }
        else {
            colorSwapchainWait = xrWaitSwapchainImage(xr_swapchains[i].handle, &wait_info);
            depthSwapchainWait = xrWaitSwapchainImage(xr_depth_swapchains[i].handle, &wait_info);
        }

        if ((colorSwapchainWait != XR_SUCCESS) || (depthSwapchainWait != XR_SUCCESS)) {
            return false;
        }

        // Set up our rendering information for the viewpoint we're using right now!
        views[i] = { XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW };

        if (is_secondary) {
            views[i].pose = xr_secondary_views[i].pose;
            views[i].fov = xr_secondary_views[i].fov;
            views[i].subImage.swapchain = xr_secondary_swapchains[i].handle;
            views[i].subImage.imageRect.offset = { 0, 0 };
            views[i].subImage.imageRect.extent = { xr_secondary_swapchains[i].width, xr_secondary_swapchains[i].height };
        }
        else {
            /*if (i == 1 && xr_secondary_views[0].fov.angleLeft != 0) {
              views[i].pose = xr_secondary_views[0].pose;
              views[i].fov = xr_secondary_views[0].fov;
            }
            else {*/
            views[i].pose = xr_views[i].pose;
            views[i].fov = xr_views[i].fov;
            //}
            views[i].subImage.swapchain = xr_swapchains[i].handle;
            views[i].subImage.imageRect.offset = { 0, 0 };
            views[i].subImage.imageRect.extent = { xr_swapchains[i].width, xr_swapchains[i].height };
        }
        views[i].next = nullptr; //&depthInfo[i];

        xr::math::NearFar nearFar = { 0.01f, 100.0f };
        depthInfo[i] = { XR_TYPE_COMPOSITION_LAYER_DEPTH_INFO_KHR };
        depthInfo[i].minDepth = 0;
        depthInfo[i].maxDepth = 1;
        depthInfo[i].nearZ = nearFar.Near;
        depthInfo[i].farZ = nearFar.Far;
        depthInfo[i].subImage.imageArrayIndex = i;
        if (is_secondary) {
            depthInfo[i].subImage.swapchain = xr_secondary_depth_swapchains[i].handle;
            depthInfo[i].subImage.imageRect.offset = { 0, 0 };
            depthInfo[i].subImage.imageRect.extent = { xr_secondary_depth_swapchains[i].width, xr_secondary_depth_swapchains[i].height };
        }
        else {
            depthInfo[i].subImage.swapchain = xr_depth_swapchains[i].handle;
            depthInfo[i].subImage.imageRect.offset = { 0, 0 };
            depthInfo[i].subImage.imageRect.extent = { xr_depth_swapchains[i].width, xr_depth_swapchains[i].height };
        }
        //xr_secondary_depth_swapchains[i].
        // Call the rendering callback with our view and swapchain info
        XrRect2Di& rect = views[i].subImage.imageRect;
        D3D11_VIEWPORT viewport = CD3D11_VIEWPORT((float)rect.offset.x, (float)rect.offset.y, (float)rect.extent.width, (float)rect.extent.height);
        if (render_for_MRC && !is_secondary) {
            viewport = CD3D11_VIEWPORT((float)rect.offset.x, (float)rect.offset.y, (float)xr_secondary_swapchains[0].width, (float)xr_secondary_swapchains[0].height);
        }
        if (is_secondary) {
            //d3d_render_layer(views[i], xr_secondary_swapchains[i].surface_data[img_id]);


            {// Set the Viewport.
                m_d3dContext.get()->RSSetViewports(1, &viewport);

                const uint32_t firstArraySliceForColor = views[i].subImage.imageArrayIndex;

                // Create a render target view into the appropriate slice of the color texture from this swapchain image.
                // This is a lightweight operation which can be done for each viewport projection.
                winrt::com_ptr<ID3D11RenderTargetView> renderTargetView;
                const CD3D11_RENDER_TARGET_VIEW_DESC renderTargetViewDesc(
                    D3D11_RTV_DIMENSION_TEXTURE2D,
                    DXGI_FORMAT_R8G8B8A8_UNORM,
                    0 /* mipSlice */,
                    firstArraySliceForColor,
                    1 /* arraySize */);

                m_d3dDevice.get()->CreateRenderTargetView(
                    xr_secondary_swapchains[i].surface_images[img_id].texture, &renderTargetViewDesc, renderTargetView.put());

                const uint32_t firstArraySliceForDepth = depthInfo[i].subImage.imageArrayIndex;

                // Create a depth stencil view into the slice of the depth stencil texture array for this swapchain image.
                // This is a lightweight operation which can be done for each viewport projection.
                winrt::com_ptr<ID3D11DepthStencilView> depthStencilView;
                CD3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc(
                    D3D11_DSV_DIMENSION_TEXTURE2D,
                    DXGI_FORMAT_D16_UNORM,
                    0 /* mipSlice */,
                    firstArraySliceForDepth,
                    1 /* arraySize */);
                m_d3dDevice.get()->CreateDepthStencilView(
                    xr_secondary_depth_swapchains[i].surface_images[depth_img_id].texture, &depthStencilViewDesc, depthStencilView.put());

                // Clear and render to the render target.
                ID3D11RenderTargetView* const renderTargets[] = { renderTargetView.get() };
                m_d3dContext.get()->OMSetRenderTargets(1, renderTargets, depthStencilView.get());

                // In double wide mode, the first projection clears the whole RTV and DSV.
                float clear[] = { 0, 0, 0, 0 };

                m_d3dContext.get()->ClearRenderTargetView(renderTargets[0],
                    reinterpret_cast<const float*>(clear));

                const float clearDepthValue = 1.f;
                m_d3dContext.get()->ClearDepthStencilView(
                    depthStencilView.get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, clearDepthValue, 0);
                m_d3dContext.get()->OMSetDepthStencilState(nullptr, 0);

                saveCurrentTargetViews(renderTargets[0], depthStencilView.get());

            }

            if (m_outputSize.Width != xr_secondary_swapchains[i].width) {
                m_outputSize.Width = xr_secondary_swapchains[i].width;
                m_outputSize.Height = xr_secondary_swapchains[i].height;
                //scene->onViewChanged();
                Manager::instance()->onViewChange(m_outputSize.Width, m_outputSize.Height);
            }

            Manager::instance()->updateCamera(xr::math::LoadInvertedXrPose(views[i].pose), xr::math::ComposeProjectionMatrix(views[i].fov, nearFar));

            scene->Update(predictedTime);
            scene->Render(9);

            // And tell OpenXR we're done with rendering to this one!
            XrSwapchainImageReleaseInfo release_info = { XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO };
            xrReleaseSwapchainImage(xr_secondary_swapchains[i].handle, &release_info);
            xrReleaseSwapchainImage(xr_secondary_depth_swapchains[i].handle, &release_info);
        }
        else {
            //d3d_render_layer(views[i], xr_swapchains[i].surface_data[img_id]);

            {// Set the Viewport.
                m_d3dContext.get()->RSSetViewports(1, &viewport);

                const uint32_t firstArraySliceForColor = views[i].subImage.imageArrayIndex;

                // Create a render target view into the appropriate slice of the color texture from this swapchain image.
                // This is a lightweight operation which can be done for each viewport projection.
                winrt::com_ptr<ID3D11RenderTargetView> renderTargetView;
                const CD3D11_RENDER_TARGET_VIEW_DESC renderTargetViewDesc(
                    D3D11_RTV_DIMENSION_TEXTURE2D,
                    DXGI_FORMAT_R8G8B8A8_UNORM,
                    0 /* mipSlice */,
                    firstArraySliceForColor,
                    1 /* arraySize */);

                m_d3dDevice.get()->CreateRenderTargetView(
                    xr_swapchains[i].surface_images[img_id].texture, &renderTargetViewDesc, renderTargetView.put());

                const uint32_t firstArraySliceForDepth = depthInfo[i].subImage.imageArrayIndex;

                // Create a depth stencil view into the slice of the depth stencil texture array for this swapchain image.
                // This is a lightweight operation which can be done for each viewport projection.
                ID3D11DepthStencilView* depthStencilView;
                CD3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc(
                    D3D11_DSV_DIMENSION_TEXTURE2D,
                    DXGI_FORMAT_D16_UNORM,
                    0 /* mipSlice */,
                    firstArraySliceForDepth,
                    1 /* arraySize */);
                m_d3dDevice.get()->CreateDepthStencilView(
                    xr_depth_swapchains[i].surface_images[depth_img_id].texture, &depthStencilViewDesc, &depthStencilView);

                // Clear and render to the render target.
                ID3D11RenderTargetView* const renderTargets[] = { renderTargetView.get() };
                m_d3dContext.get()->OMSetRenderTargets(1, renderTargets, depthStencilView);

                // In double wide mode, the first projection clears the whole RTV and DSV.
                float clear[] = { 0, 0, 0, 0 };

                m_d3dContext.get()->ClearRenderTargetView(renderTargets[0],
                    reinterpret_cast<const float*>(clear));

                const float clearDepthValue = 1.f;
                m_d3dContext.get()->ClearDepthStencilView(
                    depthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, clearDepthValue, 0);
                m_d3dContext.get()->OMSetDepthStencilState(nullptr, 0);

                saveCurrentTargetViews(renderTargets[0], depthStencilView);

            }

            if (m_outputSize.Width != xr_swapchains[i].width) {
                m_outputSize.Width = xr_swapchains[i].width;
                m_outputSize.Height = xr_swapchains[i].height;
                //scene->onViewChanged();
                Manager::instance()->onViewChange(m_outputSize.Width, m_outputSize.Height);
            }
            Manager::instance()->updateCamera(xr::math::LoadInvertedXrPose(views[i].pose), xr::math::ComposeProjectionMatrix(views[i].fov, nearFar));

            scene->Update(predictedTime);
            scene->Render(i);

            // And tell OpenXR we're done with rendering to this one!
            XrSwapchainImageReleaseInfo release_info = { XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO };
            xrReleaseSwapchainImage(xr_swapchains[i].handle, &release_info);
            xrReleaseSwapchainImage(xr_depth_swapchains[i].handle, &release_info);
        }

        //remove saved views
        removeCurrentTargetViews();
    }

    layer.space = xr_app_space;
    layer.viewCount = (uint32_t)views.size();
    layer.views = views.data();
    return true;
}


void OXRManager::d3d_render_layer(XrCompositionLayerProjectionView& view, swapchain_surfdata_t& surface) {
    // Set up where on the render target we want to draw, the view has a 
    XrRect2Di& rect = view.subImage.imageRect;
    D3D11_VIEWPORT viewport = CD3D11_VIEWPORT((float)rect.offset.x, (float)rect.offset.y, (float)rect.extent.width, (float)rect.extent.height);
    m_d3dContext.get()->RSSetViewports(1, &viewport);

    m_d3dContext.get()->OMSetRenderTargets(1, &surface.target_view, surface.depth_view);

    // Wipe our swapchain color and depth target clean, and then set them up for rendering!
    float clear[] = { 0, 0, 0, 0 };
    m_d3dContext.get()->ClearRenderTargetView(surface.target_view, clear);
    m_d3dContext.get()->ClearDepthStencilView(surface.depth_view, D3D11_CLEAR_DEPTH, 1.0f, 0);// | D3D11_CLEAR_STENCIL

    saveCurrentTargetViews(surface.target_view, surface.depth_view);
}
void OXRManager::openxr_poll_predicted(XrTime predicted_time) {
    if (xr_session_state != XR_SESSION_STATE_FOCUSED)
        return;

    // Update hand position based on the predicted time of when the frame will be rendered! This 
    // should result in a more accurate location, and reduce perceived lag.
    for (size_t i = 0; i < 2; i++) {
        if (!xr_input.renderHand[i])
            continue;
        XrSpaceLocation spaceRelation = { XR_TYPE_SPACE_LOCATION };
        XrResult        res = xrLocateSpace(xr_input.handSpace[i], xr_app_space, predicted_time, &spaceRelation);
        if (XR_UNQUALIFIED_SUCCESS(res) &&
            (spaceRelation.locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT) != 0 &&
            (spaceRelation.locationFlags & XR_SPACE_LOCATION_ORIENTATION_VALID_BIT) != 0) {
            xr_input.handPose[i] = spaceRelation.pose;
        }
    }
}

void OXRManager::SetSecondaryViewConfigurationActive(ViewConfigurationState& secondaryViewConfigState, bool active) {
    if (secondaryViewConfigState.Active != active) {
        secondaryViewConfigState.Active = active;

        // When a returned secondary view configuration is changed to active and recommended swapchain size is changed,
        // reset resources in layers related to this secondary view configuration.
        if (active) {
            uint32_t viewCount;
            auto xr_instance = XrContext().Instance.Handle;

            xrEnumerateViewConfigurationViews(xr_instance, XrContext().System.Id, secondaryViewConfigState.Type, 0, &viewCount, nullptr);

            std::vector<XrViewConfigurationView> newViewConfigViews(viewCount, { XR_TYPE_VIEW_CONFIGURATION_VIEW });
            xrEnumerateViewConfigurationViews(xr_instance, XrContext().System.Id, secondaryViewConfigState.Type, (uint32_t)newViewConfigViews.size(), &viewCount, newViewConfigViews.data());

            if (IsRecommendedSwapchainSizeChanged(secondaryViewConfigState.ViewConfigViews, newViewConfigViews)) {
                secondaryViewConfigState.ViewConfigViews = std::move(newViewConfigViews);
                //m_projectionLayers.ForEachLayerWithLock([secondaryViewConfigType = secondaryViewConfigState.Type](auto&& layer) {
                //  layer.Config(secondaryViewConfigType).ForceReset = true;
                //});
            }
        }
    }
}

DirectX::XMMATRIX OXRManager::d3d_xr_projection(XrFovf fov, float clip_near, float clip_far) {
    const float left = clip_near * tanf(fov.angleLeft);
    const float right = clip_near * tanf(fov.angleRight);
    const float down = clip_near * tanf(fov.angleDown);
    const float up = clip_near * tanf(fov.angleUp);

    return DirectX::XMMatrixPerspectiveOffCenterRH(left, right, down, up, clip_near, clip_far);
}
