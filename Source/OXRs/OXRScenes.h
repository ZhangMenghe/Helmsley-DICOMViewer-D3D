#ifndef OXR_SCENES_H
#define OXR_SCENES_H
#include "pch.h"
#include <vrController.h>
#include <Common/Manager.h>
#include <Common/TextTexture.h>
#include <Renderers/quadRenderer.h>
#include <Utils/dicomLoader.h>
#include <Renderers/FpsTextRenderer.h>
#include <Utils/dicomLoader.h>
#include <grpc/rpcHandler.h>
class OXRScenes{
public:
	OXRScenes(const std::shared_ptr<DX::DeviceResources>& deviceResources);
	void Update();
	void Update(XrTime time);
	bool Render();
	void onViewChanged();
	void setSpaces(XrSpace * space, XrSpace * app_space);

	void onSingle3DTouchDown(float x, float y, float z, int side) { m_sceneRenderer->onSingle3DTouchDown(x, y, z, side); };
	void on3DTouchMove(float x, float y, float z, int side) { m_sceneRenderer->on3DTouchMove(x, y, z, side);  };
	void on3DTouchReleased(int side){ m_sceneRenderer->on3DTouchReleased(side); };

private:
	std::shared_ptr<DX::DeviceResources> m_deviceResources;

	std::unique_ptr<vrController> m_sceneRenderer;
	std::unique_ptr<Manager> m_manager;
	std::unique_ptr<FpsTextRenderer> m_fpsTextRenderer;

	//quadRenderer* m_tex_quad;
	//TextTexture* m_text_texture;

	XrSpace * space;
	XrSpace * app_space;

	dicomLoader m_dicom_loader;
	DX::StepTimer m_timer;

	// RPC instance
	rpcHandler* m_rpcHandler;

	// RPC thread
	std::thread* m_rpcThread;

	std::string m_ds_path = "dicom-data/Larry_Smarr_2016/series_23_Cor_LAVA_PRE-Amira/";
	DirectX::XMINT3 vol_dims = DirectX::XMINT3(512, 512, 144);

};
#endif