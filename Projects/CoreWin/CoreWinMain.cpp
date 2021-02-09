#include "pch.h"
#include "CoreWinMain.h"
#include <Common/DirectXHelper.h>

using namespace CoreWin;
using namespace Windows::Foundation;
using namespace Windows::System::Threading;
using namespace Concurrency;

// Loads and initializes application assets when the application is loaded.
CoreWinMain::CoreWinMain(const std::shared_ptr<DX::DeviceResources>& deviceResources) :
	m_deviceResources(deviceResources){
	// Register to be notified if the Device is lost or recreated
	m_deviceResources->RegisterDeviceNotify(this);

	setup_resource();

	m_manager = std::make_shared<Manager>();

	m_sceneRenderer = std::make_unique<vrController>(m_deviceResources, m_manager);
	m_fpsTextRenderer = std::make_unique<FpsTextRenderer>(m_deviceResources);

	m_dicom_loader = std::make_shared<dicomLoader>();

	if (dvr::CONNECT_TO_SERVER) {
		m_rpcHandler = std::make_shared<rpcHandler>("localhost:23333");
		m_rpcThread = new std::thread(&rpcHandler::Run, m_rpcHandler);
		m_rpcHandler->setDataLoader(m_dicom_loader);
		m_rpcHandler->setVRController(m_sceneRenderer.get());
		m_rpcHandler->setManager(m_manager.get());
		m_rpcHandler->setUIController(&m_uiController);
		m_data_manager = new dataManager(m_dicom_loader, m_rpcHandler);
	}
	else {
		m_data_manager = new dataManager(m_dicom_loader);

	}

	Windows::Foundation::Size outputSize = m_deviceResources->GetOutputSize();
	m_manager->onViewChange(outputSize.Width, outputSize.Height);
	m_uiController.InitAll();

	setup_volume_local();
	//setup_volume_server();
}
void CoreWinMain::setup_volume_server(){
	//test remote
	std::vector<datasetResponse::datasetInfo> ds = m_data_manager->getAvailableDataset(false);
	std::vector<volumeResponse::volumeInfo> vl = m_data_manager->getAvailableVolumes("IRB01", false);

	volumeResponse::volumeInfo vInfo;
	for (auto vli : vl) {
		if (vli.folder_name().compare("2100_FATPOSTCORLAVAFLEX20secs") == 0) {
			vInfo = vli; break;
		}
	}
	auto dims = vInfo.dims();
	auto spacing = vInfo.resolution();
	m_dicom_loader->sendDataPrepare(
		dims[0], dims[1], dims[2],
		spacing[0] * dims[0], spacing[1] * dims[1], vInfo.volume_loc_range(),
		vInfo.with_mask());
	m_data_manager->loadData("IRB01", vInfo, false);

	//auto vector = m_rpcHandler->getVolumeFromDataset("IRB02");

	//if (vector.size() > 0) {
	//	volumeResponse::volumeInfo sel_vol_info;// = vector[0];
	//	for (auto vol : vector) {
	//		if (vol.folder_name().compare("21_WATERPOSTCORLAVAFLEX20secs") == 0) {
	//			sel_vol_info = vol;
	//			break;
	//		}
	//	}
	//	auto vdims = sel_vol_info.dims();
	//	auto spacing = sel_vol_info.resolution();
	//	m_dicom_loader->sendDataPrepare(
	//		vdims.Get(0), vdims.Get(1), vdims.Get(2),
	//		spacing.Get(0) * vdims.Get(0), spacing.Get(1) * vdims.Get(1), spacing.Get(2) * vdims.Get(2),
	//		sel_vol_info.with_mask());

	//	std::string path;// = m_rpcHandler->target_ds.folder_name() + '/' + sel_vol_info.folder_name();
	//	m_rpcHandler->DownloadVolume(path);
	//	m_rpcHandler->DownloadMasksAndCenterlines(path);
	//	m_dicom_loader.sendDataDone();
	//}
	//else {
	//	setup_volume_local();
	//}
}
void CoreWinMain::setup_volume_local() {
	//test asset demo
	std::vector<datasetResponse::datasetInfo> ds = m_data_manager->getAvailableDataset(true);
	auto dsName = ds[0].folder_name();
	std::vector<volumeResponse::volumeInfo> vl = m_data_manager->getAvailableVolumes(dsName, true);

	auto vInfo = vl[0];
	auto dims = vInfo.dims();
	auto spacing = vInfo.resolution();
	m_dicom_loader->sendDataPrepare(
		dims[0], dims[1], dims[2], 
		spacing[0] * dims[0], spacing[1] * dims[1], vInfo.volume_loc_range(), 
		vInfo.with_mask());
	m_data_manager->loadData(dsName, vInfo, true);
}

void CoreWinMain::setup_resource() {
	if (!DX::CopyAssetData("helmsley_cached/pacs_local.txt", "helmsley_cached\\pacs_local.txt", true))
		std::cerr << "Fail to copy";
}
CoreWinMain::~CoreWinMain(){
	// Deregister device notification
	m_deviceResources->RegisterDeviceNotify(nullptr);
}

// Updates application state when the window size changes (e.g. device orientation change)
void CoreWinMain::CreateWindowSizeDependentResources() 
{
	// TODO: Replace this with the size-dependent initialization of your app's content.
	m_sceneRenderer->CreateWindowSizeDependentResources();
	Windows::Foundation::Size outputSize = m_deviceResources->GetOutputSize();
	m_manager->onViewChange(outputSize.Width, outputSize.Height);
}

// Updates the application state once per frame.
void CoreWinMain::Update()
{
	// Update scene objects.
	m_timer.Tick([&]()
	{
		// TODO: Replace this with your app's content update functions.
		m_sceneRenderer->Update(m_timer);
		m_fpsTextRenderer->Update(m_timer);
	});
}

// Renders the current frame according to the current application state.
// Returns true if the frame was rendered and is ready to be displayed.
bool CoreWinMain::Render() 
{
	// Don't try to render anything before the first Update.
	if (m_timer.GetFrameCount() == 0)
	{
		return false;
	}

	auto context = m_deviceResources->GetD3DDeviceContext();

	// Reset the viewport to target the whole screen.
	auto viewport = m_deviceResources->GetScreenViewport();
	context->RSSetViewports(1, &viewport);

	// Reset render targets to the screen.
	ID3D11RenderTargetView *const targets[1] = { m_deviceResources->GetBackBufferRenderTargetView() };
	context->OMSetRenderTargets(1, targets, m_deviceResources->GetDepthStencilView());

	// Clear the back buffer and depth stencil view.
	context->ClearRenderTargetView(m_deviceResources->GetBackBufferRenderTargetView(), dvr::SCREEN_CLEAR_COLOR);
	context->ClearDepthStencilView(m_deviceResources->GetDepthStencilView(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	// Render the scene objects.
	// TODO: Replace this with your app's content rendering functions.
	m_sceneRenderer->Render();
	//m_fpsTextRenderer->Render();
	
	return true;
}

// Notifies renderers that device resources need to be released.
void CoreWinMain::OnDeviceLost()
{
	m_sceneRenderer->ReleaseDeviceDependentResources();
	m_fpsTextRenderer->ReleaseDeviceDependentResources();
}

// Notifies renderers that device resources may now be recreated.
void CoreWinMain::OnDeviceRestored()
{
	m_sceneRenderer->CreateDeviceDependentResources();
	m_fpsTextRenderer->CreateDeviceDependentResources();
	CreateWindowSizeDependentResources();
}
void CoreWinMain::OnPointerPressed(float x, float y) {
	m_sceneRenderer->onSingleTouchDown(x, y);
}
void CoreWinMain::OnPointerMoved(float x, float y) {
	m_sceneRenderer->onTouchMove(x, y);
}
void CoreWinMain::OnPointerReleased() {
	m_sceneRenderer->onTouchReleased();
}