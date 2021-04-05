#pragma comment(lib,"D3D11.lib")
#pragma comment(lib,"D3dcompiler.lib") // for shader compile
#pragma comment(lib,"Dxgi.lib") // for CreateDXGIFactory1

#include "pch.h"
#include "OXRs/OXRManager.h"
#include <OXRs/OXRScenes.h>
#include <Utils/XrMath.h>

// QR tracking stuff
#include <winrt/Windows.Foundation.h>
#include <winrt/Microsoft.MixedReality.QR.h>
//using namespace winrt::Windows::Foundation;
using namespace winrt::Microsoft::MixedReality::QR;

#include <winrt/Windows.Graphics.Holographic.h>

DX::OXRManager* oxr_manager = nullptr;
std::unique_ptr<OXRScenes> m_oxr_scene;

class QRListHelper
{
public:
	QRListHelper() {}
	QRCodeWatcher m_qrWatcher{ nullptr };

	void OnAddedQRCode(const IInspectable&, const QRCodeAddedEventArgs& args)
	{
		//m_app.OnAddedQRCode(args);
	}

	void OnUpdatedQRCode(const IInspectable&, const QRCodeUpdatedEventArgs& args)
	{
		//m_app.OnUpdatedQRCode(args);
	}

	void OnEnumerationComplete(const IInspectable&, const IInspectable&)
	{
		//m_app.OnEnumerationComplete();
	}
};

QRListHelper* list_helper = nullptr;

void SetupQRCodes(QRListHelper* qrHelper) {
  if (QRCodeWatcher::IsSupported())
	{
			QRCodeWatcherAccessStatus status = QRCodeWatcher::RequestAccessAsync().get();
		if (status == QRCodeWatcherAccessStatus::Allowed)
		{
			qrHelper->m_qrWatcher = QRCodeWatcher();
			//qrHelper->m_qrWatcher.Added({ qrHelper, &QRListHelper::OnAddedQRCode });
			//qrHelper->m_qrWatcher.Updated({ qrHelper, &QRListHelper::OnUpdatedQRCode });
			//qrHelper->m_qrWatcher.EnumerationCompleted({ qrHelper, &QRListHelper::OnEnumerationComplete });
			qrHelper->m_qrWatcher.Start();
		}
		else
		{
			// Permission denied by system or user
			// Handle the failures
		}
	}
}



int __stdcall wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int) {
	winrt::init_apartment();
	oxr_manager = new DX::OXRManager;

	if (!oxr_manager->InitOxrSession("Single file OpenXR")) {
		oxr_manager->ShutDown();
		throw std::exception("OpenXR initialization failed");
		return 1;
	}
	//list_helper = new QRListHelper();

	//SetupQRCodes(list_helper);

	auto display = winrt::Windows::Graphics::Holographic::HolographicDisplay::GetDefault();
	auto view = display.TryGetViewConfiguration(winrt::Windows::Graphics::Holographic::HolographicViewConfigurationKind::PhotoVideoCamera);
	if (view != nullptr)
	{
		view.IsEnabled(true);
	}

	oxr_manager->InitOxrActions();
	m_oxr_scene = std::unique_ptr<OXRScenes>(new OXRScenes(std::unique_ptr<DX::DeviceResources>(oxr_manager)));
	// Create reference space 1 meter in front of user
	XrSpace refSpace = oxr_manager->createReferenceSpace(XR_REFERENCE_SPACE_TYPE_LOCAL, xr::math::Pose::Translation({ 0, 0, -1 }));

	// Create anchor for volume origin
	//XrSpace volumeSpace = oxr_manager->createAnchorSpace(xr::math::Pose::Translation({ 0, 0, -1 }));

	//m_oxr_scene->setSpaces(&volumeSpace, oxr_manager->getAppSpace());

	auto onSingle3DTouchDown = [&](float x, float y, float z, int side) {
		m_oxr_scene->onSingle3DTouchDown(x, y, z, side);
	};

	auto on3DTouchMove = [&](float x, float y, float z, glm::mat4 rot, int side) {
		m_oxr_scene->on3DTouchMove(x, y, z, rot, side);
	};

	auto on3DTouchReleased = [&](int side) {
		m_oxr_scene->on3DTouchReleased(side);
	};

	oxr_manager->onSingle3DTouchDown = onSingle3DTouchDown;
	oxr_manager->on3DTouchMove = on3DTouchMove;
	oxr_manager->on3DTouchReleased = on3DTouchReleased;

	m_oxr_scene->SetupReferenceFrame(oxr_manager->getReferenceFrame());

	while (oxr_manager->Update()) {
		m_oxr_scene->Update();
		//order matters!
		oxr_manager->Render(m_oxr_scene.get());
	}
	oxr_manager->ShutDown();

	//co_return 0;
	return 0;
}
