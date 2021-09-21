#pragma comment(lib,"D3D11.lib")
#pragma comment(lib,"D3dcompiler.lib") // for shader compile
#pragma comment(lib,"Dxgi.lib") // for CreateDXGIFactory1

#include "pch.h"
#include "OXRs/OXRManager.h"
#include <OXRs/OXRMainScene.h>
#include <OXRs/XrUtility/XrMath.h>

#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Graphics.Holographic.h>

DX::OXRManager* oxr_manager = nullptr;

int __stdcall wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int) {
	winrt::init_apartment();
	oxr_manager = new DX::OXRManager;

	if (!oxr_manager->InitOxrSession("Single file OpenXR")) {
		//oxr_manager->ShutDown();
		throw std::exception("OpenXR initialization failed");
		return 1;
	}

	auto display = winrt::Windows::Graphics::Holographic::HolographicDisplay::GetDefault();
	auto view = display.TryGetViewConfiguration(winrt::Windows::Graphics::Holographic::HolographicViewConfigurationKind::PhotoVideoCamera);
	if (view != nullptr)
	{
		view.IsEnabled(true);
	}
	oxr_manager->AddScene(std::unique_ptr<xr::Scene>(new OXRMainScene(std::unique_ptr<xr::XrContext>(&oxr_manager->XrContext()))));
	oxr_manager->AddSceneFinished();

	while (oxr_manager->Update()) {
		oxr_manager->Render();
	}
	//oxr_manager->ShutDown();

	//co_return 0;
	return 0;
}
