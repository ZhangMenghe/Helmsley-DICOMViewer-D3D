#ifndef MATH_UTILS_H
#define MATH_UTILS_H
#include "pch.h"
inline DirectX::XMMATRIX mouseRotateMat(DirectX::XMMATRIX bmat, float xoffset, float yoffset) {
    DirectX::FXMVECTOR y_axis = {.0f, 1.0f, .0f};
    DirectX::FXMVECTOR x_axis = { 1.0f, .0f, .0f };

    return DirectX::XMMatrixMultiply(
        DirectX::XMMatrixMultiply(DirectX::XMMatrixRotationAxis(y_axis, xoffset), DirectX::XMMatrixRotationAxis(x_axis, yoffset)),
        bmat
    );
}
#endif // !MATH_UTILS_H
