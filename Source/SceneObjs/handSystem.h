﻿#ifndef HAND_SYSTEM_H
#define HAND_SYSTEM_H

#include <Common/DeviceResources.h>
#include <OXRs/XrUtility/XrHandle.h>
#include <OXRs/OXRManager.h>

namespace xr {
    struct XrHandData {
        xr::HandTrackerHandle TrackerHandle;

        // Data to display hand joints tracking
        //std::shared_ptr<engine::PbrModelObject> JointModel;
        //std::array<Pbr::NodeIndex_t, XR_HAND_JOINT_COUNT_EXT> PbrNodeIndices{};
        std::array<XrHandJointLocationEXT, XR_HAND_JOINT_COUNT_EXT> JointLocations{};

        // Data to display hand mesh tracking
        xr::SpaceHandle MeshSpace;
        xr::SpaceHandle ReferenceMeshSpace;
        //std::vector<DirectX::XMFLOAT4> VertexColors;
        //std::shared_ptr<engine::PbrModelObject> MeshObject;

        // Data to process open-palm reference hand.
        XrHandMeshMSFT meshState{ XR_TYPE_HAND_MESH_MSFT };
        std::unique_ptr<uint32_t[]> IndexBuffer{};
        std::unique_ptr<XrHandMeshVertexMSFT[]> VertexBuffer{};

        //Selection Action
        //XrAction selectAction;
        XrBool32 handSelect;
        XrBool32 handDeselect;

        XrBool32 isActive;

        XrHandData() = default;
        XrHandData(XrHandData&&) = delete;
        XrHandData(const XrHandData&) = delete;
    };
    enum HAND_TOUCH_EVENT {
        HAND_TOUCH_NO_EVENT=0,
        HAND_TOUCH_DOWN,
        HAND_TOUCH_MOVE,
        HAND_TOUCH_RELEASE
    };
}
// Renders the current FPS value in the bottom right corner of the screen using Direct2D and DirectWrite.
class handSystem{
public:
	handSystem(const std::shared_ptr<DX::OXRManager>& deviceResources);

    void Update(std::vector<xr::HAND_TOUCH_EVENT>& hand_events, std::vector<glm::vec3>& hand_poes);

private:
	std::shared_ptr<DX::OXRManager> m_deviceResources;

    //Hand Extensions:
    XrAction m_selectAction;


    xr::XrHandData m_leftHandData;
    xr::XrHandData m_rightHandData;

    glm::vec3 m_middle_finger_pos;
};
#endif