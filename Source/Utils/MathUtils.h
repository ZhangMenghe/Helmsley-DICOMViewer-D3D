#ifndef MATH_UTILS_H
#define MATH_UTILS_H
#include "pch.h"
const static DirectX::XMFLOAT3 DIRECTX_AXIS_Y = { .0f, 1.0f, .0f };
const static DirectX::XMFLOAT3 DIRECTX_AXIS_X = { 1.0f, .0f, .0f };
inline DirectX::XMMATRIX mouseRotateMat(DirectX::XMMATRIX bmat, float xoffset, float yoffset) {
    DirectX::XMVECTOR vy = XMLoadFloat3(&DIRECTX_AXIS_Y);
    DirectX::XMVECTOR vx = XMLoadFloat3(&DIRECTX_AXIS_X);

    return DirectX::XMMatrixMultiply(
        DirectX::XMMatrixMultiply(DirectX::XMMatrixRotationAxis(vy, xoffset), DirectX::XMMatrixRotationAxis(vx, yoffset)),
        bmat
    );
}
#endif // !MATH_UTILS_H
