#include "pch.h"
#include "OXRManager.h"
#include <thread> // sleep_for
#include <Common/Manager.h>
#include <Common/DirectXHelper.h>
#include <OXRs/XrUtility/XrMath.h>
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
    DXGI_FORMAT_B8G8R8A8_UNORM_SRGB,
    DXGI_FORMAT_R8G8B8A8_UNORM,
    DXGI_FORMAT_B8G8R8A8_UNORM,
    DXGI_FORMAT_R8G8B8A8_UNORM_SRGB
};

const std::vector<DXGI_FORMAT> SupportedDepthSwapchainFormats = {
    DXGI_FORMAT_D16_UNORM,
    DXGI_FORMAT_D32_FLOAT,
    DXGI_FORMAT_D32_FLOAT_S8X24_UINT,
    DXGI_FORMAT_D24_UNORM_S8_UINT
};
const char* RequestedExtensions[] = {
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
    XR_MSFT_SPATIAL_GRAPH_BRIDGE_EXTENSION_NAME
};

OXRManager* OXRManager::myPtr_ = nullptr;

OXRManager* OXRManager::instance()
{
    return myPtr_;
}

OXRManager::OXRManager()
    :DeviceResources(true) {
    myPtr_ = this;
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
    //setup extension
    xr::ExtensionContext extensions = xr::CreateExtensionContext(std::vector<const char*>(std::begin(RequestedExtensions), std::end(RequestedExtensions)));
    if (!extensions.SupportsD3D11) {
        throw std::logic_error("This sample currently only supports D3D11.");
        return false;
    }

    //setup instanceContex
    xr::InstanceContext instance =
        xr::CreateInstanceContext({ app_name, 1 },//xr::AppInfo
            { "XrSceneLib", 1 }, //xr::engineInfo
            extensions.EnabledExtensions);

    extensions.PopulateDispatchTable(instance.Handle);
    auto xr_instance = instance.Handle;
    if (xr_instance == nullptr)
        return false;

    //setup system
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

    uint32_t blend_count = 0;
    xrEnumerateEnvironmentBlendModes(xr_instance, system.Id, app_config_view, 1, &blend_count, &xr_blend);

    //setup binding
    auto [d3d11Binding, device, deviceContext] = DX::CreateD3D11Binding(
        xr_instance,
        system.Id,
        extensions,
        false, //m_appConfiguration.SingleThreadedD3D11Device, 
        SupportedFeatureLevels);

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

    //setup session
    xr::SessionHandle sessionHandle;
    XrSessionCreateInfo sessionCreateInfo{ XR_TYPE_SESSION_CREATE_INFO, nullptr, 0, system.Id };

    xr::InsertExtensionStruct(sessionCreateInfo, d3d11Binding);
    CHECK_XRCMD(xrCreateSession(instance.Handle, &sessionCreateInfo, sessionHandle.Put()));

    xr::SessionContext session(std::move(sessionHandle),
        system,
        extensions,
        PrimaryViewConfigurationType,
        SupportedViewConfigurationTypes, // enable all supported secondary view config
        SupportedColorSwapchainFormats,
        SupportedDepthSwapchainFormats);
    auto xr_session = session.Handle;

    // Initialize XrViewConfigurationView and XrView buffers
    for (const auto& viewConfigurationType : xr::GetAllViewConfigurationTypes(session)) {
        m_viewConfigStates.emplace(viewConfigurationType,
            xr::CreateViewConfigurationState(viewConfigurationType, instance.Handle, system.Id));
    }

    // Enum reference space type
    //uint32_t spaceCount = 0;
    //CHECK_XRCMD(xrEnumerateReferenceSpaces(session.Handle, 0, &spaceCount, NULL));

    //std::vector<XrReferenceSpaceType> supportedReferenceSpaceType;
    //supportedReferenceSpaceType.resize(spaceCount);

    //CHECK_XRCMD(xrEnumerateReferenceSpaces(session.Handle, spaceCount, &spaceCount, supportedReferenceSpaceType.data()));

    // Create view app space
    XrReferenceSpaceCreateInfo spaceCreateInfo{ XR_TYPE_REFERENCE_SPACE_CREATE_INFO };
    spaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_VIEW;
    spaceCreateInfo.poseInReferenceSpace = xr::math::Pose::Identity();
    CHECK_XRCMD(xrCreateReferenceSpace(session.Handle, &spaceCreateInfo, m_viewSpace.Put()));

    //Create main app space
    spaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL; // extensions.SupportsUnboundedSpace ? XR_REFERENCE_SPACE_TYPE_UNBOUNDED_MSFT :
    CHECK_XRCMD(xrCreateReferenceSpace(session.Handle, &spaceCreateInfo, m_appSpace.Put()));

    //setup reference frame
    auto locator = SpatialLocator::GetDefault();
    m_referenceFrame = locator.CreateStationaryFrameOfReferenceAtCurrentLocation().CoordinateSystem();

    m_context = std::make_unique<xr::XrContext>(
        std::move(instance),
        std::move(extensions),
        std::move(system),
        std::move(session),
        m_appSpace.Get(),
        //std::move(pbrResources),
        std::move(device),
        std::move(deviceContext)
        );
    m_projectionLayers.Resize(1, XrContext(), true /*forceReset*/);

    InitOxrActions();
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
        action_space_info.poseInActionSpace = xr::math::Pose::Identity();
        action_space_info.subactionPath = xr_input.handSubactionPath[i];
        xrCreateActionSpace(xr_session, &action_space_info, &xr_input.handSpace[i]);
    }

    // Attach the action set we just made to the session
    XrSessionActionSetsAttachInfo attach_info = { XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO };
    attach_info.countActionSets = 1;
    attach_info.actionSets = &xr_input.actionSet;
    xrAttachSessionActionSets(xr_session, &attach_info);
    lastFrameState = { XR_TYPE_FRAME_STATE };

    const std::tuple<XrHandEXT, xr::XrHandData&> hands[] = { {XrHandEXT::XR_HAND_LEFT_EXT, m_leftHandData},
                                                  {XrHandEXT::XR_HAND_RIGHT_EXT, m_rightHandData} };

    /*PFN_xrCreateHandTrackerEXT ext_xrCreateHandTrackerEXT;
    xrGetInstanceProcAddr(m_context->Instance.Handle, "xrCreateHandTrackerEXT",
        reinterpret_cast<PFN_xrVoidFunction*>(&ext_xrCreateHandTrackerEXT));

    PFN_xrDestroyHandTrackerEXT ext_xrDestroyHandTrackerEXT;
    xrGetInstanceProcAddr(m_context->Instance.Handle, "xrDestroyHandTrackerEXT",
        reinterpret_cast<PFN_xrVoidFunction*>(&ext_xrDestroyHandTrackerEXT));*/

    // For each hand, initialize the joint objects and corresponding space.
    for (const auto& [hand, handData] : hands) {
        XrHandTrackerCreateInfoEXT createInfo{ XR_TYPE_HAND_TRACKER_CREATE_INFO_EXT };
        createInfo.hand = hand;
        createInfo.handJointSet = XR_HAND_JOINT_SET_DEFAULT_EXT;
        CHECK_XRCMD(m_context->Extensions.xrCreateHandTrackerEXT(
            xr_session, &createInfo, handData.TrackerHandle.Put(m_context->Extensions.xrDestroyHandTrackerEXT)));

        //CHECK_XRCMD(ext_xrCreateHandTrackerEXT(
        //    xr_session, 
        //    &createInfo, 
        //    handData.TrackerHandle.Put(ext_xrDestroyHandTrackerEXT)));
        //xrCreateHandTrackerEXT(xr_session, &createInfo, handData.TrackerHandle.Put(xrDestroyHandTrackerEXT));
        //createJointObjects(handData);


        // Initialize buffers to receive hand mesh indices and vertices
        //const XrSystemHandTrackingMeshPropertiesMSFT& handMeshSystemProperties = context.System.HandMeshProperties;
        //handData.IndexBuffer = std::make_unique<uint32_t[]>(handMeshSystemProperties.maxHandMeshIndexCount);
        //handData.VertexBuffer = std::make_unique<XrHandMeshVertexMSFT[]>(handMeshSystemProperties.maxHandMeshVertexCount);

        //handData.meshState.indexBuffer.indexCapacityInput = handMeshSystemProperties.maxHandMeshIndexCount;
        //handData.meshState.indexBuffer.indices = handData.IndexBuffer.get();
        //handData.meshState.vertexBuffer.vertexCapacityInput = handMeshSystemProperties.maxHandMeshVertexCount;
        //handData.meshState.vertexBuffer.vertices = handData.VertexBuffer.get();

        XrHandMeshSpaceCreateInfoMSFT meshSpaceCreateInfo{ XR_TYPE_HAND_MESH_SPACE_CREATE_INFO_MSFT };
        meshSpaceCreateInfo.poseInHandMeshSpace = xr::math::Pose::Identity();
        meshSpaceCreateInfo.handPoseType = XR_HAND_POSE_TYPE_TRACKED_MSFT;
        CHECK_XRCMD(m_context->Extensions.xrCreateHandMeshSpaceMSFT(
            handData.TrackerHandle.Get(), &meshSpaceCreateInfo, handData.MeshSpace.Put()));

        meshSpaceCreateInfo.handPoseType = XR_HAND_POSE_TYPE_REFERENCE_OPEN_PALM_MSFT;
        CHECK_XRCMD(m_context->Extensions.xrCreateHandMeshSpaceMSFT(
            handData.TrackerHandle.Get(), &meshSpaceCreateInfo, handData.ReferenceMeshSpace.Put()));
        //PFN_xrCreateHandMeshSpaceMSFT   ext_xrCreateHandMeshSpaceMSFT;
        //xrGetInstanceProcAddr(m_context->Instance.Handle, "xrCreateHandMeshSpaceMSFT",
        //    reinterpret_cast<PFN_xrVoidFunction*>(&ext_xrCreateHandMeshSpaceMSFT));
        //CHECK_XRCMD(ext_xrCreateHandMeshSpaceMSFT(handData.TrackerHandle.Get(), &meshSpaceCreateInfo, handData.MeshSpace.Put()));

        //xrGetInstanceProcAddr(m_context->Instance.Handle, "xrLocateHandJointsEXT",
        //    reinterpret_cast<PFN_xrVoidFunction*>(&ext_xrLocateHandJointsEXT));

        //meshSpaceCreateInfo.handPoseType = XR_HAND_POSE_TYPE_REFERENCE_OPEN_PALM_MSFT;
        //CHECK_XRCMD(ext_xrCreateHandMeshSpaceMSFT(handData.TrackerHandle.Get(), &meshSpaceCreateInfo, handData.ReferenceMeshSpace.Put()));
    
    }
}
void OXRManager::InitHLSensors(const std::vector<ResearchModeSensorType>& kEnabledSensorTypes) {
    m_sensor_manager = std::make_unique<HLSensorManager>(kEnabledSensorTypes);
    m_sensor_manager->InitializeXRSpaces(m_context->Instance.Handle, m_context->Session.Handle);
}
bool OXRManager::Update() {
    openxr_poll_events();
    if (xr_running) {
        //
        openxr_poll_hands_ext();
        openxr_poll_actions();

        if (xr_session_state != XR_SESSION_STATE_VISIBLE &&
            xr_session_state != XR_SESSION_STATE_FOCUSED) {
            std::this_thread::sleep_for(std::chrono::milliseconds(250));
        }

        XrFrameState frame_state = { XR_TYPE_FRAME_STATE };
        auto xr_session = XrContext().Session.Handle;

        // secondaryViewConfigFrameState needs to have the same lifetime as frameState
        XrSecondaryViewConfigurationFrameStateMSFT secondaryViewConfigFrameState{ XR_TYPE_SECONDARY_VIEW_CONFIGURATION_FRAME_STATE_MSFT };

        const size_t enabledSecondaryViewConfigCount = XrContext().Session.EnabledSecondaryViewConfigurationTypes.size();
        std::vector<XrSecondaryViewConfigurationStateMSFT> secondaryViewConfigStates(enabledSecondaryViewConfigCount,
            { XR_TYPE_SECONDARY_VIEW_CONFIGURATION_STATE_MSFT });

        if (XrContext().Extensions.SupportsSecondaryViewConfiguration && enabledSecondaryViewConfigCount > 0) {
            secondaryViewConfigFrameState.viewConfigurationCount = (uint32_t)secondaryViewConfigStates.size();
            secondaryViewConfigFrameState.viewConfigurationStates = secondaryViewConfigStates.data();
            secondaryViewConfigFrameState.next = frame_state.next;
            frame_state.next = &secondaryViewConfigFrameState;
        }

        XrFrameWaitInfo waitFrameInfo{ XR_TYPE_FRAME_WAIT_INFO };
        xrWaitFrame(xr_session, &waitFrameInfo, &frame_state);

        if (XrContext().Extensions.SupportsSecondaryViewConfiguration) {
            std::scoped_lock lock(m_secondaryViewConfigActiveMutex);
            m_secondaryViewConfigurationsState = std::move(secondaryViewConfigStates);
        }
        m_current_framestate = frame_state;

        m_currentFrameTime.Update(frame_state, xr_session_state);

        for (auto& scene : m_scenes) {
            if (scene->IsActive()) {
                scene->Update(m_currentFrameTime);
            }
        }
    }
    return !xr_quit;
}
void OXRManager::Render() {
    const xr::FrameTime renderFrameTime = m_currentFrameTime;

    auto xr_session = XrContext().Session.Handle;

    XrFrameBeginInfo beginFrameDescription{ XR_TYPE_FRAME_BEGIN_INFO };
    CHECK_XRCMD(xrBeginFrame(xr_session, &beginFrameDescription));

    if (XrContext().Extensions.SupportsSecondaryViewConfiguration) {
        std::scoped_lock lock(m_secondaryViewConfigActiveMutex);
        for (auto& state : m_secondaryViewConfigurationsState) {
            SetSecondaryViewConfigurationActive(m_viewConfigStates.at(state.viewConfigurationType), state.active);
        }
    }
    m_projectionLayers.ForEachLayerWithLock([this](auto&& layer) {
        for (auto& [viewConfigType, state] : m_viewConfigStates) {
            if (xr::IsPrimaryViewConfigurationType(viewConfigType) || state.Active) {
                layer.PrepareRendering(XrContext(), viewConfigType, state.ViewConfigViews);
            }
        }
    });

    XrFrameEndInfo end_info{ XR_TYPE_FRAME_END_INFO };
    end_info.environmentBlendMode = XrContext().System.ViewProperties.at(app_config_view).BlendMode;
    end_info.displayTime = renderFrameTime.PredictedDisplayTime;//actually current frame, just updated

    // Secondary view config frame info need to have same lifetime as XrFrameEndInfo;
    XrSecondaryViewConfigurationFrameEndInfoMSFT frameEndSecondaryViewConfigInfo{
        XR_TYPE_SECONDARY_VIEW_CONFIGURATION_FRAME_END_INFO_MSFT };
    std::vector<XrSecondaryViewConfigurationLayerInfoMSFT> activeSecondaryViewConfigLayerInfos;

    // Chain secondary view configuration layers data to endFrameInfo
    if (XrContext().Extensions.SupportsSecondaryViewConfiguration &&
        XrContext().Session.EnabledSecondaryViewConfigurationTypes.size() > 0) {
        for (auto& secondaryViewConfigType : XrContext().Session.EnabledSecondaryViewConfigurationTypes) {
            auto& secondaryViewConfig = m_viewConfigStates.at(secondaryViewConfigType);
            if (secondaryViewConfig.Active) {
                activeSecondaryViewConfigLayerInfos.emplace_back(
                    XrSecondaryViewConfigurationLayerInfoMSFT{ XR_TYPE_SECONDARY_VIEW_CONFIGURATION_LAYER_INFO_MSFT,
                                                              nullptr,
                                                              secondaryViewConfigType,
                                                              XrContext().System.ViewProperties.at(secondaryViewConfigType).BlendMode });
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


    // Prepare array of layer data for each active view configurations.
    std::vector<xr::CompositionLayers> layersForAllViewConfigs(1 + activeSecondaryViewConfigLayerInfos.size());
    if (renderFrameTime.ShouldRender) {
        std::scoped_lock sceneLock(m_sceneMutex);

        for (const std::unique_ptr<xr::Scene>& scene : m_scenes) {
            if (scene->IsActive()) {
                scene->BeforeRender(m_currentFrameTime);
            }
        }

        // Render for the primary view configuration.
        xr::CompositionLayers& primaryViewConfigLayers = layersForAllViewConfigs[0];
        RenderViewConfiguration(sceneLock, PrimaryViewConfigurationType, primaryViewConfigLayers);
        end_info.layerCount = primaryViewConfigLayers.LayerCount();
        end_info.layers = primaryViewConfigLayers.LayerData();

        // Render layers for any active secondary view configurations too.
        if (XrContext().Extensions.SupportsSecondaryViewConfiguration && activeSecondaryViewConfigLayerInfos.size() > 0) {
            for (size_t i = 0; i < activeSecondaryViewConfigLayerInfos.size(); i++) {
                XrSecondaryViewConfigurationLayerInfoMSFT& secondaryViewConfigLayerInfo = activeSecondaryViewConfigLayerInfos.at(i);
                xr::CompositionLayers& secondaryViewConfigLayers = layersForAllViewConfigs.at(i + 1);
                RenderViewConfiguration(sceneLock, secondaryViewConfigLayerInfo.viewConfigurationType, secondaryViewConfigLayers);
                secondaryViewConfigLayerInfo.layerCount = secondaryViewConfigLayers.LayerCount();
                secondaryViewConfigLayerInfo.layers = secondaryViewConfigLayers.LayerData();
            }
        }
    }



    /*
    // If the session is active, lets render our layer in the compositor!
    // Primary render
    XrCompositionLayerBaseHeader* layer = nullptr;
    XrCompositionLayerProjection             layer_proj = { XR_TYPE_COMPOSITION_LAYER_PROJECTION };
    std::vector<XrCompositionLayerProjectionView> views;
    std::vector<XrCompositionLayerDepthInfoKHR> depthInfo;
    bool session_active = xr_session_state == XR_SESSION_STATE_VISIBLE || xr_session_state == XR_SESSION_STATE_FOCUSED;

    if (session_active && openxr_render_layer(m_current_framestate.predictedDisplayTime, views, depthInfo, layer_proj)) {
        layer = (XrCompositionLayerBaseHeader*)&layer_proj;
    }

    std::vector<std::vector<XrCompositionLayerProjectionView>> views_array;
    std::vector<std::vector<XrCompositionLayerDepthInfoKHR>> depthInfo_array;
    std::vector<XrCompositionLayerProjection> layer_proj_array;
    std::vector<XrCompositionLayerBaseHeader*> layer_array;
    // Secondary render
    if (XrContext().Extensions.SupportsSecondaryViewConfiguration && activeSecondaryViewConfigLayerInfos.size() > 0) {
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
            if (openxr_render_layer(m_current_framestate.predictedDisplayTime, views, depthInfo, layer_proj, true)) {
                layer_array[i] = (XrCompositionLayerBaseHeader*)&layer_proj;
            }
            //RenderViewConfiguration(sceneLock, secondaryViewConfigLayerInfo.viewConfigurationType, secondaryViewConfigLayers);
            secondaryViewConfigLayerInfo.layerCount = layer_array[i] == nullptr ? 0 : 1;
            secondaryViewConfigLayerInfo.layers = &(layer_array[i]);
        }
    }

    // We're finished with rendering our layer, so send it off for display!
    end_info.displayTime = m_current_framestate.predictedDisplayTime;
    end_info.environmentBlendMode = xr_blend;
    end_info.layerCount = layer == nullptr ? 0 : 1;
    end_info.layers = &layer;*/

    CHECK_XRCMD(xrEndFrame(xr_session, &end_info));

    lastFrameState = m_current_framestate;
}

void OXRManager::AddScene(std::unique_ptr<xr::Scene> scene) {
    if (!scene) {
        return; // Some scenes might skip creation due to extension unavailability.
    }
    scene->SetupDeviceResource(std::unique_ptr<DX::DeviceResources>(this));
    scene->SetupReferenceFrame(m_referenceFrame);

    std::scoped_lock lock(m_sceneMutex);
    m_scenes.push_back(std::move(scene));


}
void OXRManager::AddSceneFinished() {
    onSingle3DTouchDown = [&](float x, float y, float z, int side) {
        for (const std::unique_ptr<xr::Scene>& scene : m_scenes)
            scene->onSingle3DTouchDown(x, y, z, side);
    };
    on3DTouchMove = [&](float x, float y, float z, glm::mat4 rot, int side) {
        for (const std::unique_ptr<xr::Scene>& scene : m_scenes)
            scene->on3DTouchMove(x, y, z, rot, side);
    };
    on3DTouchReleased = [&](int side) {
        for (const std::unique_ptr<xr::Scene>& scene : m_scenes)
            scene->on3DTouchReleased(side);
    };
}

XrSpatialAnchorMSFT DX::OXRManager::createAnchor(const XrPosef& poseInScene)
{
    auto xr_session = XrContext().Session.Handle;

    XrSpatialAnchorMSFT anchor;
    // Anchors provide the best stability when moving beyond 5 meters, so if the extension is enabled,
    // create an anchor at given location and place the hologram at the resulting anchor space.
    XrSpatialAnchorCreateInfoMSFT createInfo{ XR_TYPE_SPATIAL_ANCHOR_CREATE_INFO_MSFT };
    createInfo.space = m_appSpace.Get();
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

DirectX::XMMATRIX OXRManager::getSensorMatrixAtTime(uint64_t time) {
    if (m_sensor_manager) return m_sensor_manager->getSensorMatrixAtTime(m_appSpace, time);
    return DirectX::XMMatrixIdentity();
}
void OXRManager::openxr_poll_events() {
    auto xr_session = XrContext().Session.Handle;
    XrEventDataBuffer event_buffer = { XR_TYPE_EVENT_DATA_BUFFER };

    while (xrPollEvent(XrContext().Instance.Handle, &event_buffer) == XR_SUCCESS) {
        switch (event_buffer.type) {

        case XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED:
        {
            XrEventDataSessionStateChanged* changed = (XrEventDataSessionStateChanged*)&event_buffer;
            xr_session_state = changed->state;

            // Session state change is where we can begin and end sessions, as well as find quit messages!
            switch (xr_session_state)
            {
            case XR_SESSION_STATE_READY: {
                XrSessionBeginInfo begin_info = { XR_TYPE_SESSION_BEGIN_INFO };
                begin_info.primaryViewConfigurationType = app_config_view;

                XrSecondaryViewConfigurationSessionBeginInfoMSFT secondaryViewConfigInfo{
                    XR_TYPE_SECONDARY_VIEW_CONFIGURATION_SESSION_BEGIN_INFO_MSFT };
                if (XrContext().Extensions.SupportsSecondaryViewConfiguration &&
                    XrContext().Session.EnabledSecondaryViewConfigurationTypes.size() > 0) {
                    secondaryViewConfigInfo.viewConfigurationCount = (uint32_t)XrContext().Session.EnabledSecondaryViewConfigurationTypes.size();
                    secondaryViewConfigInfo.enabledViewConfigurationTypes = XrContext().Session.EnabledSecondaryViewConfigurationTypes.data();

                    secondaryViewConfigInfo.next = begin_info.next;
                    begin_info.next = &secondaryViewConfigInfo;
                }

                xrBeginSession(xr_session, &begin_info);
                xr_running = true;
            }
                                       break;
            case XR_SESSION_STATE_STOPPING:
            {
                xr_running = false;
                xrEndSession(xr_session);
            }
            break;
            case XR_SESSION_STATE_EXITING:
                xr_quit = true; break;
            case XR_SESSION_STATE_LOSS_PENDING:
                xr_quit = true; break;
            }
        }
        break;
        case XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING:
            xr_quit = true;
            return;
        }
        event_buffer = { XR_TYPE_EVENT_DATA_BUFFER };
    }
}
void OXRManager::openxr_poll_hands_ext() {
    if (xr_session_state != XR_SESSION_STATE_FOCUSED)
        return;
    xr::XrHandData& handData = std::ref(m_rightHandData);
    //for (xr::XrHandData& handData : { std::ref(m_leftHandData), std::ref(m_rightHandData) }) {
    XrHandJointsLocateInfoEXT locateInfo{ XR_TYPE_HAND_JOINTS_LOCATE_INFO_EXT };
    locateInfo.baseSpace = m_appSpace.Get();
    locateInfo.time = m_current_framestate.predictedDisplayTime;

    XrHandJointLocationsEXT locations{ XR_TYPE_HAND_JOINT_LOCATIONS_EXT };
    locations.jointCount = (uint32_t)handData.JointLocations.size();
    locations.jointLocations = handData.JointLocations.data();
    CHECK_XRCMD(m_context->Extensions.xrLocateHandJointsEXT(handData.TrackerHandle.Get(), &locateInfo, &locations));
    if (locations.isActive) {
        const XrVector3f& index_tip = handData.JointLocations[XR_HAND_JOINT_INDEX_TIP_EXT].pose.position;
        const XrVector3f& thumb_tip = handData.JointLocations[XR_HAND_JOINT_THUMB_TIP_EXT].pose.position;
        m_middle_finger_pos = glm::vec3(index_tip.x + thumb_tip.x, index_tip.y + thumb_tip.y, index_tip.z + thumb_tip.z) * 0.5f;
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
            XrResult        res = xrLocateSpace(xr_input.handSpace[hand], m_appSpace.Get(), lastFrameState.predictedDisplayTime, &space_location);
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
                    //onSingle3DTouchDown(x, y, z, hand);
                    onSingle3DTouchDown(m_middle_finger_pos.x, m_middle_finger_pos.y, m_middle_finger_pos.z, hand);
                }
                else if (xr_input.handDeselect[hand]) {
                    // If we have a deselect event, send on3DTouchReleased
                    on3DTouchReleased(hand);
                }
                else {
                    // Send on3DTouchMove
                    //on3DTouchMove(x, y, z, rotMat, hand);
                    on3DTouchMove(m_middle_finger_pos.x, m_middle_finger_pos.y, m_middle_finger_pos.z, rotMat, hand);

                }

            }
        }
        else {

            // lose tracking = release
            on3DTouchReleased(hand);
        }


    }
}

void OXRManager::RenderViewConfiguration(const std::scoped_lock<std::mutex>& proofOfSceneLock,
    XrViewConfigurationType viewConfigurationType,
    xr::CompositionLayers& layers) {
    // Locate the views in VIEW space to get the per-view offset from the VIEW "camera"
    XrViewState viewState{ XR_TYPE_VIEW_STATE };
    std::vector<XrView>& views = m_viewConfigStates.at(viewConfigurationType).Views;
    {
        XrViewLocateInfo viewLocateInfo{ XR_TYPE_VIEW_LOCATE_INFO };
        viewLocateInfo.viewConfigurationType = viewConfigurationType;
        viewLocateInfo.displayTime = m_currentFrameTime.PredictedDisplayTime;
        viewLocateInfo.space = m_viewSpace.Get();

        uint32_t viewCount = 0;
        CHECK_XRCMD(
            xrLocateViews(XrContext().Session.Handle, &viewLocateInfo, &viewState, (uint32_t)views.size(), &viewCount, views.data()));
        assert(viewCount == views.size());
        if (!xr::math::Pose::IsPoseValid(viewState)) {
            return;
        }
    }

    // Locate the VIEW space in the app space to get the "camera" pose and combine the per-view offsets with the camera pose.
    XrSpaceLocation viewLocation{ XR_TYPE_SPACE_LOCATION };
    CHECK_XRCMD(xrLocateSpace(m_viewSpace.Get(), m_appSpace.Get(), m_currentFrameTime.PredictedDisplayTime, &viewLocation));
    if (!xr::math::Pose::IsPoseValid(viewLocation)) {
        return;
    }

    for (XrView& view : views) {
        view.pose = xr::math::Pose::Multiply(view.pose, viewLocation.pose);
    }

    m_projectionLayers.ForEachLayerWithLock([this, &layers, &views, viewConfigurationType](xr::ProjectionLayer& projectionLayer) {
        bool opaqueClearColor = (layers.LayerCount() == 0); // Only the first projection layer need opaque background
        opaqueClearColor &= (XrContext().Session.PrimaryViewConfigurationBlendMode == XR_ENVIRONMENT_BLEND_MODE_OPAQUE);
        DirectX::XMStoreFloat4(&projectionLayer.Config().ClearColor,
            opaqueClearColor ? DirectX::XMColorSRGBToRGB(DirectX::Colors::CornflowerBlue)
            : DirectX::Colors::Transparent);
        const bool shouldSubmitProjectionLayer =
            projectionLayer.Render(XrContext(), m_currentFrameTime, XrContext().AppSpace, views, m_scenes, viewConfigurationType);

        // Create the multi projection layer
        if (shouldSubmitProjectionLayer) {
            AppendProjectionLayer(layers, &projectionLayer, viewConfigurationType);
        }
    });
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
        XrResult        res = xrLocateSpace(xr_input.handSpace[i], m_appSpace.Get(), predicted_time, &spaceRelation);
        if (XR_UNQUALIFIED_SUCCESS(res) &&
            (spaceRelation.locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT) != 0 &&
            (spaceRelation.locationFlags & XR_SPACE_LOCATION_ORIENTATION_VALID_BIT) != 0) {
            xr_input.handPose[i] = spaceRelation.pose;
        }
    }
}

void OXRManager::SetSecondaryViewConfigurationActive(xr::ViewConfigurationState& secondaryViewConfigState, bool active) {
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
                m_projectionLayers.ForEachLayerWithLock([secondaryViewConfigType = secondaryViewConfigState.Type](auto&& layer) {
                    layer.Config(secondaryViewConfigType).ForceReset = true;
                });
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
