#pragma once

#include <dxgi1_4.h>
#include <d3d11_4.h>
#include <d2d1_3.h>
#include <d2d1effects_2.h>
#include <dwrite_3.h>
#include <wincodec.h>
#include <DirectXColors.h>
#include <DirectXMath.h>
#include <memory>
#include <concrt.h>
//winrt stuff
#include <winrt/base.h>
#include <iostream>

//glm
#include <glm/glm.hpp>

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN // Exclude rarely-used stuff from Windows headers
#define XR_USE_PLATFORM_WIN32
#define XR_USE_GRAPHICS_API_D3D11
#define XR_MSFT_spatial_graph_bridge

#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>
#include <strsafe.h>

//#define LOGBUFF TCHAR buf[1024]
#define LOGINFO(fmt, ...) { \
TCHAR buf[1024];\
StringCbPrintf(buf, 1024 * sizeof(TCHAR), TEXT(fmt), __VA_ARGS__);\
OutputDebugString(buf);\
}