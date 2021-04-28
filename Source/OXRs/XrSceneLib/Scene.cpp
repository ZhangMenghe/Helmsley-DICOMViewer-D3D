#include "pch.h"
#include "Scene.h"

using namespace DirectX;
using namespace xr;

Scene::Scene(const std::shared_ptr<xr::XrContext>& context)
    : m_context(context){
}
