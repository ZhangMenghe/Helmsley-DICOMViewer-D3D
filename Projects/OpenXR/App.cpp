#pragma comment(lib,"D3D11.lib")
#pragma comment(lib,"D3dcompiler.lib") // for shader compile
#pragma comment(lib,"Dxgi.lib") // for CreateDXGIFactory1

#include "pch.h"
#include "OXRs/OXRManager.h"
#include <OXRs/OXRMainScene.h>
#include <OXRs/XrUtility/XrMath.h>

#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Graphics.Holographic.h>
#include <SceneObjs/handSystem.h>

std::unique_ptr<DX::OXRManager> oxr_manager;
std::shared_ptr<handSystem> m_hand_sys;

int __stdcall wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int) {
	oxr_manager = std::make_unique<DX::OXRManager>();
	winrt::init_apartment();
	if (oxr_manager->InitOxrSession("Single file OpenXR")) {
		//Init Actions
		m_hand_sys = std::make_shared<handSystem>(std::unique_ptr<DX::OXRManager>(oxr_manager.get()));
	}else {
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
	//std::unique_ptr<xr::Scene> main_scene = std::make_unique<xr::Scene>(OXRMainScene(std::unique_ptr<xr::XrContext>(oxr_manager->XrContext())));
	auto main_scene = std::unique_ptr<xr::Scene>(new OXRMainScene(std::unique_ptr<xr::XrContext>(oxr_manager->XrContext())));

	oxr_manager->AddScene(main_scene.get());
	main_scene->setHandInteractionSystem(m_hand_sys);

	//oxr_manager->AddSceneFinished();
	std::function<void(float, float, float, int)> onSingle3DTouchDown = [&](float x, float y, float z, int side) {
		//for (const std::unique_ptr<xr::Scene>& scene : m_scenes)
		main_scene->onSingle3DTouchDown(x, y, z, side);
	};
	std::function<void(float, float, float, glm::mat4, int)> on3DTouchMove = [&](float x, float y, float z, glm::mat4 rot, int side) {
		//for (const std::unique_ptr<xr::Scene>& scene : m_scenes)
		main_scene->on3DTouchMove(x, y, z, rot, side);
	};
	std::function<void(int)> on3DTouchReleased = [&](int side) {
		//for (const std::unique_ptr<xr::Scene>& scene : m_scenes)
		main_scene->on3DTouchReleased(side);
	};

	bool is_running = true;
	while (is_running) {
		if(oxr_manager->BeforeUpdate()){
			std::vector<xr::HAND_TOUCH_EVENT> hand_events; std::vector<glm::vec3> hand_poses;
			m_hand_sys->Update(hand_events, hand_poses);
			for (int i = 0; i < 2; i++) {
				if (hand_events[i] == xr::HAND_TOUCH_NO_EVENT) continue;

				if (hand_events[i] == xr::HAND_TOUCH_DOWN) onSingle3DTouchDown(hand_poses[i].x, hand_poses[i].y, hand_poses[i].z, i);
				else if (hand_events[i] == xr::HAND_TOUCH_RELEASE) on3DTouchReleased(i);
				else on3DTouchMove(hand_poses[i].x, hand_poses[i].y, hand_poses[i].z, glm::mat4(1.0f), i);
			}
		}
		is_running = oxr_manager->Update();
		oxr_manager->Render();
	}
	//oxr_manager->ShutDown();

	//co_return 0;
	return 0;
}
