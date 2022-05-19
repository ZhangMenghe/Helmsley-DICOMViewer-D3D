#pragma once

//#include <wrl.h>
//#include <wrl/client.h>
#include <dxgi1_4.h>
#include <d3d11_3.h>
#include <d2d1_3.h>
#include <d2d1effects_2.h>
#include <dwrite_3.h>
#include <wincodec.h>
#include <DirectXColors.h>
#include <DirectXMath.h>
#include <memory>
#include <agile.h>
#include <concrt.h>
//winrt stuff
#include <winrt/base.h>
#include <iostream>

//glm
#include <glm/glm.hpp>

#include <strsafe.h>
//#define LOGBUFF TCHAR buf[1024]
#define LOGINFO(fmt, ...) { \
TCHAR buf[1024];\
StringCbPrintf(buf, 1024 * sizeof(TCHAR), TEXT(fmt), __VA_ARGS__);\
OutputDebugString(buf);\
}