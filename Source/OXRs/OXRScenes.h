#ifndef OXR_SCENES_H
#define OXR_SCENES_H
#include "pch.h"
#include <vrController.h>
#include <Common/Manager.h>
#include <Common/TextTexture.h>
#include <Renderers/quadRenderer.h>
#include <Utils/dicomLoader.h>
#include <Renderers/FpsTextRenderer.h>
class OXRScenes{
public:
	OXRScenes(const std::shared_ptr<DX::DeviceResources>& deviceResources);
	void Update();
	bool Render();
	void onViewChanged();
private:
	std::shared_ptr<DX::DeviceResources> m_deviceResources;

	std::unique_ptr<vrController> m_sceneRenderer;
	std::unique_ptr<Manager> m_manager;
	std::unique_ptr<FpsTextRenderer> m_fpsTextRenderer;

	//quadRenderer* m_tex_quad;
	//TextTexture* m_text_texture;

	dicomLoader m_dicom_loader;
	DX::StepTimer m_timer;

	std::string m_ds_path = "dicom-data/Larry_Smarr_2016/series_23_Cor_LAVA_PRE-Amira/";
	DirectX::XMINT3 vol_dims = DirectX::XMINT3(512, 512, 144);

};
#endif