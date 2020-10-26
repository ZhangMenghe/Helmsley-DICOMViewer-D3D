#pragma comment(lib,"D3D11.lib")
#pragma comment(lib,"D3dcompiler.lib") // for shader compile
#pragma comment(lib,"Dxgi.lib") // for CreateDXGIFactory1


#include "pch.h"
#include "OXRs/OXRManager.h"
#include "Content/Sample3DSceneRenderer.h"

using namespace std;
using namespace DirectX; // Matrix math
using namespace DX;


OXRManager* oxr_manager = nullptr;
Sample3DSceneRenderer* scene = nullptr;
DX::StepTimer m_timer;


int __stdcall wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int) {
	oxr_manager = new OXRManager;

	if (!oxr_manager->InitOxrSession("Single file OpenXR")) {
		oxr_manager->ShutDown();
		throw std::exception("OpenXR initialization failed");
		return 1;
	}
	
	oxr_manager->InitOxrActions();
	scene = new Sample3DSceneRenderer(std::unique_ptr<DX::DeviceResources>(oxr_manager), true);

	while (oxr_manager->Update()) {
		// Update scene objects.
		m_timer.Tick([&](){
			// TODO: Replace this with your app's content update functions.
			scene->Update(m_timer);
		});

		if (m_timer.GetFrameCount() == 0){
			return 0;
		}

		//order matters!
		oxr_manager->Render(scene);
	}
	oxr_manager->ShutDown();

	return 0;
}
