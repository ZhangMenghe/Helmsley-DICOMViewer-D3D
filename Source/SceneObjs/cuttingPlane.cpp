#include "pch.h"
#include "cuttingPlane.h"
#include <Common/Manager.h>
#include <Utils/MathUtils.h>
cuttingPlane::cuttingPlane(ID3D11Device* device) {//, DirectX::XMMATRIX model_mat
    //mat4 vm_inv = transpose(inverse(model_mat));
    //update_plane_(vec3MatNorm(vm_inv, Manager::camera->getViewDirection()));
    //onReset();
}
cuttingPlane::cuttingPlane(ID3D11Device* device, DirectX::XMFLOAT3 ps, DirectX::XMFLOAT3 pn)
:p_start_(ps), p_norm_(pn) {
    //p_rotate_mat_ = rotMatFromDir(pn);
    //onReset();
}
