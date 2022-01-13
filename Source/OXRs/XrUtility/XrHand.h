#pragma once

#include "XrHandle.h"
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

        XrHandData() = default;
        XrHandData(XrHandData&&) = delete;
        XrHandData(const XrHandData&) = delete;
    };
}