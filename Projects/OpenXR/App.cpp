#pragma comment(lib,"D3D11.lib")
#pragma comment(lib,"D3dcompiler.lib") // for shader compile
#pragma comment(lib,"Dxgi.lib") // for CreateDXGIFactory1

#include "pch.h"
#include "OXRs/OXRManager.h"
#include <OXRs/OXRScenes.h>

DX::OXRManager* oxr_manager = nullptr;
std::unique_ptr<OXRScenes> m_oxr_scene;

int __stdcall wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int) {
	oxr_manager = new DX::OXRManager;

	if (!oxr_manager->InitOxrSession("Single file OpenXR")) {
		oxr_manager->ShutDown();
		throw std::exception("OpenXR initialization failed");
		return 1;
	}
	
	oxr_manager->InitOxrActions();
	m_oxr_scene = std::unique_ptr<OXRScenes>(new OXRScenes(std::unique_ptr<DX::DeviceResources>(oxr_manager)));

	while (oxr_manager->Update()) {
		m_oxr_scene->Update();
		//order matters!
		oxr_manager->Render(m_oxr_scene.get());

	}
	oxr_manager->ShutDown();

	return 0;
}
