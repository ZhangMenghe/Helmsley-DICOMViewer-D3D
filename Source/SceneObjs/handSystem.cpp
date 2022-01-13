#include "pch.h"
#include "handSystem.h"
//#include <Common/DirectXHelper.h>
//#include <Utils/TypeConvertUtils.h>
//#include <D3DPipeline/Primitive.h>
//#include "strsafe.h"

handSystem::handSystem(const std::shared_ptr<DX::OXRManager>& deviceResources)
:m_deviceResources(deviceResources){
	auto m_context = m_deviceResources->XrContext();
	auto m_actionContext = m_deviceResources->XrActionContext();

	auto xr_instance = m_context->Instance.Handle;
	auto xr_session = m_context->Session.Handle;

	const std::tuple<XrHandEXT, xr::XrHandData&> hands[] = {
		{XrHandEXT::XR_HAND_LEFT_EXT, m_leftHandData},
		{XrHandEXT::XR_HAND_RIGHT_EXT, m_rightHandData} };

	xr::ActionSet& actionSet = m_actionContext->CreateActionSet("dicom_viewer_actions", "DICOM Viewer Actions");

	//selection
	m_selectAction = actionSet.CreateAction("select_action", "Select Action", XR_ACTION_TYPE_BOOLEAN_INPUT, {});
	m_actionContext->SuggestInteractionProfileBindings("/interaction_profiles/khr/simple_controller",
		{
			{m_selectAction, "/user/hand/right/input/select/click"},
			{m_selectAction, "/user/hand/left/input/select/click"},
		});

	// For each hand, initialize the joint objects and corresponding space.
	for (const auto& [hand, handData] : hands) {
		XrHandTrackerCreateInfoEXT createInfo{ XR_TYPE_HAND_TRACKER_CREATE_INFO_EXT };
		createInfo.hand = hand;
		createInfo.handJointSet = XR_HAND_JOINT_SET_DEFAULT_EXT;
		CHECK_XRCMD(m_context->Extensions.xrCreateHandTrackerEXT(
			xr_session, &createInfo, handData.TrackerHandle.Put(m_context->Extensions.xrDestroyHandTrackerEXT)));


		XrHandMeshSpaceCreateInfoMSFT meshSpaceCreateInfo{ XR_TYPE_HAND_MESH_SPACE_CREATE_INFO_MSFT };
		meshSpaceCreateInfo.poseInHandMeshSpace = xr::math::Pose::Identity();
		meshSpaceCreateInfo.handPoseType = XR_HAND_POSE_TYPE_TRACKED_MSFT;
		CHECK_XRCMD(m_context->Extensions.xrCreateHandMeshSpaceMSFT(
			handData.TrackerHandle.Get(), &meshSpaceCreateInfo, handData.MeshSpace.Put()));

		meshSpaceCreateInfo.handPoseType = XR_HAND_POSE_TYPE_REFERENCE_OPEN_PALM_MSFT;
		CHECK_XRCMD(m_context->Extensions.xrCreateHandMeshSpaceMSFT(
			handData.TrackerHandle.Get(), &meshSpaceCreateInfo, handData.ReferenceMeshSpace.Put()));
	}
	std::vector<const xr::ActionContext*> actionContexts;
	actionContexts.push_back(m_actionContext);
	xr::AttachActionsToSession(m_context->Instance.Handle, m_context->Session.Handle, actionContexts);
}

void handSystem::Update(std::vector<xr::HAND_TOUCH_EVENT>& hand_events, std::vector<glm::vec3>& hand_poes){
    auto m_context = m_deviceResources->XrContext();
    auto m_actionContext = m_deviceResources->XrActionContext();

    //sync actions
    std::vector<const xr::ActionContext*> actionContexts;
    actionContexts.push_back(m_actionContext);
    xr::SyncActions(m_context->Session.Handle, actionContexts);

    const std::tuple<XrHandEXT, xr::XrHandData&> hands[] = {
    {XrHandEXT::XR_HAND_LEFT_EXT, m_leftHandData},
    {XrHandEXT::XR_HAND_RIGHT_EXT, m_rightHandData} };
    hand_events = std::vector<xr::HAND_TOUCH_EVENT>(2, xr::HAND_TOUCH_NO_EVENT);
    hand_poes = std::vector<glm::vec3>(2);

    for (const auto& [hand, handData] : hands) {
        XrHandJointsLocateInfoEXT locateInfo{ XR_TYPE_HAND_JOINTS_LOCATE_INFO_EXT };
        locateInfo.baseSpace = m_deviceResources->XrAppSpace().Get();
        locateInfo.time = m_deviceResources->getCurrentFrameState().predictedDisplayTime;

        XrHandJointLocationsEXT locations{ XR_TYPE_HAND_JOINT_LOCATIONS_EXT };
        locations.jointCount = (uint32_t)handData.JointLocations.size();
        locations.jointLocations = handData.JointLocations.data();
        CHECK_XRCMD(m_context->Extensions.xrLocateHandJointsEXT(handData.TrackerHandle.Get(), &locateInfo, &locations));
        handData.isActive = locations.isActive;

        if (locations.isActive) {
            const XrVector3f& index_tip = handData.JointLocations[XR_HAND_JOINT_INDEX_TIP_EXT].pose.position;
            const XrVector3f& thumb_tip = handData.JointLocations[XR_HAND_JOINT_THUMB_TIP_EXT].pose.position;
            m_middle_finger_pos = glm::vec3(index_tip.x + thumb_tip.x, index_tip.y + thumb_tip.y, index_tip.z + thumb_tip.z) * 0.5f;
            hand_poes[hand - 1] = m_middle_finger_pos;

            XrActionStateBoolean state{ XR_TYPE_ACTION_STATE_BOOLEAN };
            XrActionStateGetInfo getInfo{ XR_TYPE_ACTION_STATE_GET_INFO };
            getInfo.action = m_selectAction;
            CHECK_XRCMD(xrGetActionStateBoolean(m_context->Session.Handle, &getInfo, &state));
            const bool isSelectPressed = state.isActive && state.changedSinceLastSync && state.currentState;

            handData.handSelect = state.isActive && state.currentState && state.changedSinceLastSync;
            handData.handDeselect = state.isActive && !state.currentState && state.changedSinceLastSync;

            if (handData.handSelect)
                hand_events[hand-1] = xr::HAND_TOUCH_DOWN;
            else if (handData.handDeselect)
                hand_events[hand - 1] = xr::HAND_TOUCH_RELEASE;
            else 
                hand_events[hand - 1] = xr::HAND_TOUCH_MOVE;
        }
        else {
            hand_events[hand - 1] = xr::HAND_TOUCH_RELEASE;
        }
    }
}
