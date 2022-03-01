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
    m_actionContext = std::make_unique<xr::ActionContext>(m_context->Instance.Handle);

    m_projectionLayers.Resize(1, *(m_context), true /*forceReset*/);

    lastFrameState = { XR_TYPE_FRAME_STATE };

    return true;
}

void OXRManager::InitHLSensors(const std::vector<ResearchModeSensorType>& kEnabledSensorTypes) {
    m_sensor_manager = std::make_unique<HLSensorManager>(kEnabledSensorTypes);
    m_sensor_manager->InitializeXRSpaces(m_context->Instance.Handle, m_context->Session.Handle);
}
bool OXRManager::BeforeUpdate() {
    openxr_poll_events();
    return xr_running && xr_session_state == XR_SESSION_STATE_FOCUSED;
}

bool OXRManager::Update() {
    if (xr_running) {
        //openxr_poll_hands_ext();
        //openxr_poll_actions();

        if (xr_session_state != XR_SESSION_STATE_VISIBLE &&
            xr_session_state != XR_SESSION_STATE_FOCUSED) {
            std::this_thread::sleep_for(std::chrono::milliseconds(250));
        }

        XrFrameState frame_state = { XR_TYPE_FRAME_STATE };
        auto xr_session = m_context->Session.Handle;

        // secondaryViewConfigFrameState needs to have the same lifetime as frameState
        XrSecondaryViewConfigurationFrameStateMSFT secondaryViewConfigFrameState{ XR_TYPE_SECONDARY_VIEW_CONFIGURATION_FRAME_STATE_MSFT };

        const size_t enabledSecondaryViewConfigCount = m_context->Session.EnabledSecondaryViewConfigurationTypes.size();
        std::vector<XrSecondaryViewConfigurationStateMSFT> secondaryViewConfigStates(enabledSecondaryViewConfigCount,
            { XR_TYPE_SECONDARY_VIEW_CONFIGURATION_STATE_MSFT });

        if (m_context->Extensions.SupportsSecondaryViewConfiguration && enabledSecondaryViewConfigCount > 0) {
            secondaryViewConfigFrameState.viewConfigurationCount = (uint32_t)secondaryViewConfigStates.size();
            secondaryViewConfigFrameState.viewConfigurationStates = secondaryViewConfigStates.data();
            secondaryViewConfigFrameState.next = frame_state.next;
            frame_state.next = &secondaryViewConfigFrameState;
        }

        XrFrameWaitInfo waitFrameInfo{ XR_TYPE_FRAME_WAIT_INFO };
        xrWaitFrame(xr_session, &waitFrameInfo, &frame_state);

        if (m_context->Extensions.SupportsSecondaryViewConfiguration) {
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

    auto xr_session = m_context->Session.Handle;

    XrFrameBeginInfo beginFrameDescription{ XR_TYPE_FRAME_BEGIN_INFO };
    CHECK_XRCMD(xrBeginFrame(xr_session, &beginFrameDescription));

    if (m_context->Extensions.SupportsSecondaryViewConfiguration) {
        std::scoped_lock lock(m_secondaryViewConfigActiveMutex);
        for (auto& state : m_secondaryViewConfigurationsState) {
            SetSecondaryViewConfigurationActive(m_viewConfigStates.at(state.viewConfigurationType), state.active);
        }
    }
    m_projectionLayers.ForEachLayerWithLock([this](auto&& layer) {
        for (auto& [viewConfigType, state] : m_viewConfigStates) {
            if (xr::IsPrimaryViewConfigurationType(viewConfigType) || state.Active) {
                layer.PrepareRendering(*m_context, viewConfigType, state.ViewConfigViews);
            }
        }
    });

    XrFrameEndInfo end_info{ XR_TYPE_FRAME_END_INFO };
    end_info.environmentBlendMode = m_context->System.ViewProperties.at(app_config_view).BlendMode;
    end_info.displayTime = renderFrameTime.PredictedDisplayTime;//actually current frame, just updated

    // Secondary view config frame info need to have same lifetime as XrFrameEndInfo;
    XrSecondaryViewConfigurationFrameEndInfoMSFT frameEndSecondaryViewConfigInfo{
        XR_TYPE_SECONDARY_VIEW_CONFIGURATION_FRAME_END_INFO_MSFT };
    std::vector<XrSecondaryViewConfigurationLayerInfoMSFT> activeSecondaryViewConfigLayerInfos;

    // Chain secondary view configuration layers data to endFrameInfo
    if (m_context->Extensions.SupportsSecondaryViewConfiguration &&
        m_context->Session.EnabledSecondaryViewConfigurationTypes.size() > 0) {
        for (auto& secondaryViewConfigType : m_context->Session.EnabledSecondaryViewConfigurationTypes) {
            auto& secondaryViewConfig = m_viewConfigStates.at(secondaryViewConfigType);
            if (secondaryViewConfig.Active) {
                activeSecondaryViewConfigLayerInfos.emplace_back(
                    XrSecondaryViewConfigurationLayerInfoMSFT{ XR_TYPE_SECONDARY_VIEW_CONFIGURATION_LAYER_INFO_MSFT,
                                                              nullptr,
                                                              secondaryViewConfigType,
                                                              m_context->System.ViewProperties.at(secondaryViewConfigType).BlendMode });
            }
        }
        m_render_primary = (activeSecondaryViewConfigLayerInfos.size() == 0);

        if (!m_render_primary) {
            frameEndSecondaryViewConfigInfo.viewConfigurationCount = (uint32_t)activeSecondaryViewConfigLayerInfos.size();
            frameEndSecondaryViewConfigInfo.viewConfigurationLayersInfo = activeSecondaryViewConfigLayerInfos.data();
            frameEndSecondaryViewConfigInfo.next = end_info.next;
            end_info.next = &frameEndSecondaryViewConfigInfo;
        }
    }


    // Prepare array of layer data for each active view configurations.
    std::vector<xr::CompositionLayers> layersForAllViewConfigs(1 + activeSecondaryViewConfigLayerInfos.size());
    if (renderFrameTime.ShouldRender) {
        std::scoped_lock sceneLock(m_sceneMutex);

        for (auto scene : m_scenes) {
            if (scene->IsActive()) {
                scene->BeforeRender(m_currentFrameTime);
            }
        }
        if( m_keep_primary_while_mrc || m_render_primary){
            // Render for the primary view configuration.
            xr::CompositionLayers& primaryViewConfigLayers = layersForAllViewConfigs[0];
            RenderViewConfiguration(sceneLock, PrimaryViewConfigurationType, primaryViewConfigLayers);
            end_info.layerCount = primaryViewConfigLayers.LayerCount();
            end_info.layers = primaryViewConfigLayers.LayerData();
        }
        // Render layers for any active secondary view configurations too.
        if (m_context->Extensions.SupportsSecondaryViewConfiguration && activeSecondaryViewConfigLayerInfos.size() > 0) {
            for (size_t i = 0; i < activeSecondaryViewConfigLayerInfos.size(); i++) {
                XrSecondaryViewConfigurationLayerInfoMSFT& secondaryViewConfigLayerInfo = activeSecondaryViewConfigLayerInfos.at(i);
                xr::CompositionLayers& secondaryViewConfigLayers = layersForAllViewConfigs.at(i + 1);
                RenderViewConfiguration(sceneLock, secondaryViewConfigLayerInfo.viewConfigurationType, secondaryViewConfigLayers);
                secondaryViewConfigLayerInfo.layerCount = secondaryViewConfigLayers.LayerCount();
                secondaryViewConfigLayerInfo.layers = secondaryViewConfigLayers.LayerData();
            }
        }
    }

    CHECK_XRCMD(xrEndFrame(xr_session, &end_info));

    lastFrameState = m_current_framestate;
}

void OXRManager::AddScene(xr::Scene* scene) {
    if (!scene) {
        return; // Some scenes might skip creation due to extension unavailability.
    }
    scene->SetupDeviceResource(std::unique_ptr<DX::DeviceResources>(this));
    scene->SetupReferenceFrame(m_referenceFrame);

    std::scoped_lock lock(m_sceneMutex);
    m_scenes.push_back(std::move(scene));


}
//void OXRManager::AddSceneFinished() {
//    onSingle3DTouchDown = [&](float x, float y, float z, int side) {
//        for (const std::unique_ptr<xr::Scene>& scene : m_scenes)
//            scene->onSingle3DTouchDown(x, y, z, side);
//    };
//    on3DTouchMove = [&](float x, float y, float z, glm::mat4 rot, int side) {
//        for (const std::unique_ptr<xr::Scene>& scene : m_scenes)
//            scene->on3DTouchMove(x, y, z, rot, side);
//    };
//    on3DTouchReleased = [&](int side) {
//        for (const std::unique_ptr<xr::Scene>& scene : m_scenes)
//            scene->on3DTouchReleased(side);
//    };
//}

XrSpatialAnchorMSFT DX::OXRManager::createAnchor(const XrPosef& poseInScene)
{
    auto xr_session = m_context->Session.Handle;

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
    auto xr_session = m_context->Session.Handle;
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
    auto xr_session = m_context->Session.Handle;
    XrEventDataBuffer event_buffer = { XR_TYPE_EVENT_DATA_BUFFER };

    while (xrPollEvent(m_context->Instance.Handle, &event_buffer) == XR_SUCCESS) {
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
                if (m_context->Extensions.SupportsSecondaryViewConfiguration &&
                    m_context->Session.EnabledSecondaryViewConfigurationTypes.size() > 0) {
                    secondaryViewConfigInfo.viewConfigurationCount = (uint32_t)m_context->Session.EnabledSecondaryViewConfigurationTypes.size();
                    secondaryViewConfigInfo.enabledViewConfigurationTypes = m_context->Session.EnabledSecondaryViewConfigurationTypes.data();

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
            xrLocateViews(m_context->Session.Handle, &viewLocateInfo, &viewState, (uint32_t)views.size(), &viewCount, views.data()));
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
        opaqueClearColor &= (m_context->Session.PrimaryViewConfigurationBlendMode == XR_ENVIRONMENT_BLEND_MODE_OPAQUE);
        DirectX::XMStoreFloat4(&projectionLayer.Config().ClearColor,
            opaqueClearColor ? DirectX::XMColorSRGBToRGB(DirectX::Colors::CornflowerBlue)
            : DirectX::Colors::Transparent);
        const bool shouldSubmitProjectionLayer =
            projectionLayer.Render(*m_context, m_currentFrameTime, m_context->AppSpace, views, m_scenes, viewConfigurationType);

        // Create the multi projection layer
        if (shouldSubmitProjectionLayer) {
            AppendProjectionLayer(layers, &projectionLayer, viewConfigurationType);
        }
    });
}

void OXRManager::SetSecondaryViewConfigurationActive(xr::ViewConfigurationState& secondaryViewConfigState, bool active) {
    if (secondaryViewConfigState.Active != active) {
        secondaryViewConfigState.Active = active;

        // When a returned secondary view configuration is changed to active and recommended swapchain size is changed,
        // reset resources in layers related to this secondary view configuration.
        if (active) {
            uint32_t viewCount;
            auto xr_instance = m_context->Instance.Handle;

            xrEnumerateViewConfigurationViews(xr_instance, m_context->System.Id, secondaryViewConfigState.Type, 0, &viewCount, nullptr);

            std::vector<XrViewConfigurationView> newViewConfigViews(viewCount, { XR_TYPE_VIEW_CONFIGURATION_VIEW });
            xrEnumerateViewConfigurationViews(xr_instance, m_context->System.Id, secondaryViewConfigState.Type, (uint32_t)newViewConfigViews.size(), &viewCount, newViewConfigViews.data());

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
