#ifndef TYPE_CONVERT_UTILS_H
#define TYPE_CONVERT_UTILS_H
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

inline glm::mat4 xmmatrix2mat4(DirectX::XMMATRIX mmat) {
    DirectX::XMFLOAT4X4 mmat_f;
    DirectX::XMStoreFloat4x4(&mmat_f, mmat);
    const float* tmp = mmat_f.m[0];
    return glm::make_mat4(tmp);
}
inline DirectX::XMMATRIX mat42xmmatrix(glm::mat4 gmat) {
    DirectX::XMFLOAT4X4 mmat_f(glm::value_ptr(gmat));
    return DirectX::XMLoadFloat4x4(&mmat_f);
}
inline glm::vec3 float32vec3(DirectX::XMFLOAT3 v) {
    return glm::vec3(v.x, v.y, v.z);
}

#endif