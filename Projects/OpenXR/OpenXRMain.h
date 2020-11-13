#ifndef OPENXR_MAIN_H
#define OPENXR_MAIN_H

#include "Common\StepTimer.h"
#include "Common\DeviceResources.h"
#include <vrController.h>
#include <Utils/dicomLoader.h>

// Renders Direct2D and 3D content on the screen.
namespace OpenXR
{
	class OpenXRMain : public DX::IDeviceNotify
	{
	public:
		OpenXRMain(const std::shared_ptr<DX::DeviceResources>& deviceResources);
		~OpenXRMain();
		void CreateWindowSizeDependentResources();
		void Update();
		bool Render();

		// IDeviceNotify
		virtual void OnDeviceLost();
		virtual void OnDeviceRestored();

	private:
		//std::string m_ds_path = "dicom-data/IRB01/2100_FATPOSTCORLAVAFLEX20secs/";
		//DirectX::XMINT3 vol_dims = DirectX::XMINT3(512, 512, 164);
		std::string m_ds_path = "dicom-data/Larry_Smarr_2016/series_23_Cor_LAVA_PRE-Amira/";
		DirectX::XMINT3 vol_dims = DirectX::XMINT3(512, 512, 144);
		// Cached pointer to device resources.
		std::shared_ptr<DX::DeviceResources> m_deviceResources;

		std::unique_ptr<vrController> m_sceneRenderer;
		dicomLoader m_dicom_loader;

		// Rendering loop timer.
		DX::StepTimer m_timer;
	};
}
#endif // !OPENXR_MAIN_H