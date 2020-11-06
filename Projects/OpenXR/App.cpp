#pragma comment(lib,"D3D11.lib")
#pragma comment(lib,"D3dcompiler.lib") // for shader compile
#pragma comment(lib,"Dxgi.lib") // for CreateDXGIFactory1


#include "pch.h"
#include "OXRs/OXRManager.h"
#include <vrController.h>
#include <Utils/dicomLoader.h>

using namespace std;
using namespace DirectX; // Matrix math
using namespace DX;

std::string m_ds_path = "dicom-data/IRB01/2100_FATPOSTCORLAVAFLEX20secs/";
DirectX::XMINT3 vol_dims = DirectX::XMINT3(512, 512, 164);

OXRManager* oxr_manager = nullptr;
std::unique_ptr<vrController> m_sceneRenderer;
dicomLoader m_dicom_loader;
DX::StepTimer m_timer;


int __stdcall wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int) {
	oxr_manager = new OXRManager;

	if (!oxr_manager->InitOxrSession("Single file OpenXR")) {
		oxr_manager->ShutDown();
		throw std::exception("OpenXR initialization failed");
		return 1;
	}
	
	oxr_manager->InitOxrActions();
	m_sceneRenderer = std::unique_ptr<vrController>(new vrController(std::unique_ptr<DX::DeviceResources>(oxr_manager)));

	m_dicom_loader.setupDCMIConfig(vol_dims.x, vol_dims.y, vol_dims.z, -1, -1, -1, true);

	if (m_dicom_loader.loadData(m_ds_path + "data", m_ds_path + "mask")) {
		m_sceneRenderer->assembleTexture(2, vol_dims.x, vol_dims.y, vol_dims.z, -1, -1, -1, m_dicom_loader.getVolumeData(), m_dicom_loader.getChannelNum());
		//m_sceneRenderer.reset();
	}

	while (oxr_manager->Update()) {
		// Update scene objects.
		m_timer.Tick([&](){
			// TODO: Replace this with your app's content update functions.
			m_sceneRenderer->Update(m_timer);
		});

		if (m_timer.GetFrameCount() == 0){
			return 0;
		}

		//order matters!
		oxr_manager->Render(m_sceneRenderer.get());
	}
	oxr_manager->ShutDown();

	return 0;
}
