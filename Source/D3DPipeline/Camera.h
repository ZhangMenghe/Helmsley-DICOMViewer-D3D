#ifndef D3DPIPELINE_CAMERA_H
#define D3DPIPELINE_CAMERA_H
#include "pch.h"

class Camera {
    const char* name_;

    DirectX::XMMATRIX _viewMat, _projMat, pose_mat;
    DirectX::XMVECTOR _eyePos, _center, _up, _front, _right;

    float fov;

    const float NEAR_PLANE = 0.01f;//as close as possible
    const float FAR_PLANE = 100.0f;
    DirectX::XMVECTOR ORI_CAM_POS = { 0.0f, .0f, 1.5f, 0.0f};
    DirectX::XMVECTOR ORI_UP = { 0.0f, 1.0f, 0.0f, 0.0f };
    DirectX::XMVECTOR ORI_FRONT = { 0.0f, 0.0f, -1.0f };

    void updateCameraVector() {
        _viewMat = DirectX::XMMatrixTranspose(DirectX::XMMatrixLookAtRH(_eyePos, _center, _up));
        DirectX::XMFLOAT3 pos_dest;
        DirectX::XMStoreFloat3(&pos_dest, _eyePos);
        pose_mat = DirectX::XMMatrixTranslation(pos_dest.x, pos_dest.y, pos_dest.z);
    }
    void updateCameraVector(DirectX::XMMATRIX model) {
        _viewMat = DirectX::XMMatrixTranspose(DirectX::XMMatrixLookAtRH(_eyePos, _center, _up));
        pose_mat = model;
    }
    void reset(DirectX::XMVECTOR pos, DirectX::XMVECTOR up, DirectX::XMVECTOR center) {
        _up = up; _eyePos = pos;
        _center = center;
        _front = DirectX::XMVectorSubtract(_center, _eyePos);
        updateCameraVector();
    }
public:
    Camera() {
        auto mcenter = DirectX::XMVectorAdd(ORI_CAM_POS, ORI_FRONT);
        reset(ORI_CAM_POS, ORI_UP, mcenter);
    }
    Camera(const char* cam_name) :name_(cam_name) { 
        auto mcenter = DirectX::XMVectorAdd(ORI_CAM_POS, ORI_FRONT);
        reset(ORI_CAM_POS, ORI_UP, mcenter);
    }
    Camera(DirectX::XMVECTOR pos, DirectX::XMVECTOR up, DirectX::XMVECTOR center) {
        reset(pos, up, center);
    }

    //setters
    void setProjMat(int screen_width, int screen_height) {
        /*float screen_ratio = ((float)screen_width) / screen_height;
        fov = 70.0f * DirectX::XM_PI / 180.0f;
        _projMat = DirectX::XMMatrixPerspectiveFovRH(
            fov,
            screen_ratio,
            NEAR_PLANE,
            FAR_PLANE
        );*/

        float aspectRatio = ((float)screen_width) / screen_height;
        float fovAngleY = 70.0f * DirectX::XM_PI / 180.0f;
        if (aspectRatio < 1.0f)
        {
            fovAngleY *= 2.0f;
        }
        _projMat = DirectX::XMMatrixPerspectiveFovRH(
            fovAngleY,
            aspectRatio,
            0.01f,
            100.0f
        );
        const DirectX::XMFLOAT4X4 orientation(
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f
        );

        //XMFLOAT4X4 orientation = m_deviceResources->GetOrientationTransform3D();

        DirectX::XMMATRIX orientationMatrix = XMLoadFloat4x4(&orientation);
        _projMat = DirectX::XMMatrixTranspose(_projMat * orientationMatrix);
    }
    void setViewMat(DirectX::XMMATRIX viewmat) { _viewMat = viewmat; }
    //void setProjMat(DirectX::XMMATRIX projmat) {
    //    fov = 2.0 * atan(1.0f / projmat[1][1]);
    //    _projMat = projmat;
    //}

    //void updateCameraPose(DirectX::XMMATRIX pose) {
    //    //pose is in column major
    //    
    //    _eyePos = glm::vec3(pose[3][0], pose[3][1], pose[3][2]);
    //    _front = -glm::vec3(pose[2][0], pose[2][1], pose[2][2]);
    //    pose_mat = pose;
    //}

    //getters
    float getFOV() { return fov; }
    DirectX::XMMATRIX getProjMat() { return _projMat; }
    DirectX::XMMATRIX getViewMat() { return _viewMat; }
    DirectX::XMMATRIX getVPMat() { return DirectX::XMMatrixMultiply(_projMat, _viewMat); }
    DirectX::XMVECTOR getCameraPosition() { return _eyePos; }
    DirectX::XMVECTOR getViewCenter() { return _center; }
    DirectX::XMVECTOR getViewDirection() { return _front; }
    DirectX::XMVECTOR getViewUpDirection() { return _up; }
    DirectX::XMMATRIX getCameraPose() { return pose_mat; }
    //DirectX::XMVECTOR getRotationMatrixOfCameraDirection() {
    //    //a is the vector you want to translate to and b is where you are
    //    const glm::vec3 a = -_front;//src_dir;
    //    const glm::vec3 b = glm::vec3(0, 0, 1);//dest_dir;
    //    glm::vec3 v = glm::cross(b, a);
    //    float angle = acos(glm::dot(b, a) / (glm::length(b) * glm::length(a)));
    //    return glm::rotate(angle, v);
    //}
    //void rotateCamera(int axis, glm::vec4 center, float offset) {
    //    glm::vec3 rotateAxis = (axis == 3) ? glm::vec3(0, 1, 0) : glm::vec3(1, 0, 0);
    //    glm::mat4 modelMat = glm::mat4(1.0);

    //    modelMat = glm::translate(modelMat, glm::vec3(-center.x, -center.y, -center.z));
    //    modelMat = glm::rotate(modelMat, offset, rotateAxis);
    //    modelMat = glm::translate(modelMat, glm::vec3(center.x, center.y, center.z));
    //    _eyePos = glm::vec3(modelMat * glm::vec4(_eyePos, 1.0));
    //    _center = glm::vec3(center);
    //    _front = _center - _eyePos;
    //    //        _front = glm::vec3(modelMat * glm::vec4(_front,1.0));
    //    //        // Also re-calculate the Right and Up vector
    //    //        _right = glm::vec3(glm::cross(_front, ORI_UP));  // Normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
    //    //        _up = glm::normalize(glm::cross(_right, _front));
    //    //        _center = _eyePos + _front;

    //    updateCameraVector();
    //}
};

#endif