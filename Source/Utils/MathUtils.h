#ifndef MATH_UTILS_H
#define MATH_UTILS_H
#include "pch.h"
inline DirectX::XMMATRIX mouseRotateMat(DirectX::XMMATRIX bmat, float xoffset, float yoffset) {
    DirectX::XMFLOAT3 y_axis = {.0f, 1.0f, .0f};
    DirectX::XMFLOAT3 x_axis = { 1.0f, .0f, .0f };
    DirectX::XMVECTOR vy = XMLoadFloat3(&y_axis);
    DirectX::XMVECTOR vx = XMLoadFloat3(&x_axis);

    return DirectX::XMMatrixMultiply(
        DirectX::XMMatrixMultiply(DirectX::XMMatrixRotationAxis(vy, xoffset), DirectX::XMMatrixRotationAxis(vx, yoffset)),
        bmat
    );
}
#endif // !MATH_UTILS_H
