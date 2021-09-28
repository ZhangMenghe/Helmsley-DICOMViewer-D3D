#ifndef D3DPIPELINE_CAMERA_H
#define D3DPIPELINE_CAMERA_H
#include "strsafe.h"
#include <d3d11_3.h>

#include "Utils/TypeConvertUtils.h"
class Camera {
private:
    const char* name_;

    DirectX::XMMATRIX _viewMat, _projMat, pose_mat;
    DirectX::XMFLOAT3 _eyePos, _center, _up, _front, _right;

    float fov;

    const float NEAR_PLANE = 0.01f;//as close as possible
    const float FAR_PLANE = 100.0f;
    DirectX::XMFLOAT3 ORI_CAM_POS = { 0.0f, .0f, .0f };
    DirectX::XMFLOAT3 ORI_UP = { 0.0f, 1.0f, 0.0f };
    DirectX::XMFLOAT3 ORI_FRONT = { 0.0f, 0.0f, -1.0f };

public:
    Camera() {
        DirectX::XMFLOAT3 mcenter = { ORI_CAM_POS.x + ORI_FRONT.x, ORI_CAM_POS.y + ORI_FRONT.y, ORI_CAM_POS.z + ORI_FRONT.z }; //DirectX::XMVectorAdd(ORI_CAM_POS, ORI_FRONT);
        Reset(ORI_CAM_POS, ORI_UP, mcenter);
    }
    Camera(const char* cam_name) :name_(cam_name) {
        DirectX::XMFLOAT3 mcenter = { ORI_CAM_POS.x + ORI_FRONT.x, ORI_CAM_POS.y + ORI_FRONT.y, ORI_CAM_POS.z + ORI_FRONT.z }; //DirectX::XMVectorAdd(ORI_CAM_POS, ORI_FRONT);
        Reset(ORI_CAM_POS, ORI_UP, mcenter);
    }
    Camera(DirectX::XMFLOAT3 pos, DirectX::XMFLOAT3 up, DirectX::XMFLOAT3 center) {
        Reset(pos, up, center);
    }
    void Reset(Camera* cam) {
        Reset(cam->_eyePos, cam->_up, cam->_center);
    }
    void Reset(DirectX::XMFLOAT3 pos, DirectX::XMFLOAT3 up, DirectX::XMFLOAT3 center) {
        _up = up;
        _eyePos = pos;
        _center = center;
        _front = { _center.x - _eyePos.x, _center.y - _eyePos.y, _center.z - _eyePos.z };

        DirectX::XMVECTOR veye = DirectX::XMLoadFloat3(&_eyePos);
        DirectX::XMVECTOR vc = DirectX::XMLoadFloat3(&_center);
        DirectX::XMVECTOR vu = DirectX::XMLoadFloat3(&_up);

        _viewMat = DirectX::XMMatrixTranspose(DirectX::XMMatrixLookAtRH(veye, vc, vu));

        pose_mat = DirectX::XMMatrixTranslation(_eyePos.x, _eyePos.y, _eyePos.z);
    }

    //setters
    void setProjMat(int screen_width, int screen_height) {
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

        DirectX::XMMATRIX orientationMatrix = DirectX::XMLoadFloat4x4(&orientation);
        _projMat = DirectX::XMMatrixTranspose(_projMat * orientationMatrix);
    }

    void update(const DirectX::XMMATRIX pose, const DirectX::XMMATRIX proj) {
      _viewMat = DirectX::XMMatrixTranspose(pose);
      auto mat = glm::inverse(xmmatrix2mat4(pose));
      if (dvr::PRINT_CAMERA_MATRIX){
        TCHAR buf[1024];
        size_t cbDest = 1024 * sizeof(TCHAR);
        StringCbPrintf(buf, cbDest, TEXT("Camera:(%f,%f,%f)\n"), (float)mat[3][0], (float)mat[3][1], (float)mat[3][2]);
        OutputDebugString(buf);
      }

      _projMat = DirectX::XMMatrixTranspose(proj);
      DirectX::XMVECTOR temp = DirectX::XMVector3Transform(XMLoadFloat3(&ORI_FRONT), DirectX::XMMatrixRotationQuaternion(DirectX::XMQuaternionRotationMatrix(pose)));
      DirectX::XMStoreFloat3(
        &_front,
        temp);
    }

    void setViewMat(DirectX::XMMATRIX viewmat) { _viewMat = viewmat; }

    //getters
    float getFOV() { return fov; }
    DirectX::XMMATRIX getProjMat() { return _projMat; }
    DirectX::XMMATRIX getViewMat() { return _viewMat; }
    DirectX::XMMATRIX getVPMat() { return DirectX::XMMatrixMultiply(_projMat, _viewMat); }
    DirectX::XMFLOAT3 getCameraPosition() { return _eyePos; }
    DirectX::XMFLOAT3 getViewCenter() { return _center; }
    DirectX::XMFLOAT3 getViewDirection() { return _front; }
    DirectX::XMFLOAT3 getViewUpDirection() { return _up; }
    DirectX::XMMATRIX getCameraPose() { return pose_mat; }
};

#endif