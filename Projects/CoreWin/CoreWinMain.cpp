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

	m_manager = std::make_shared<Manager>();

	m_sceneRenderer = std::make_unique<vrController>(m_deviceResources, m_manager);
	m_fpsTextRenderer = std::make_unique<FpsTextRenderer>(m_deviceResources);

	m_dicom_loader = std::make_shared<dicomLoader>();

	Size outputSize = m_deviceResources->GetOutputSize();
	m_manager->onViewChange(outputSize.Width, outputSize.Height);
	m_uiController.InitAll();

	setup_resource();
}
void CoreWinMain::setup_volume_server(){
	//test remote
	std::vector<datasetResponse::datasetInfo> ds = m_data_manager->getAvailableDataset(false);
	std::vector<volumeInfo> vl;
	m_data_manager->getAvailableVolumes("IRB01", vl, false);

	volumeInfo vInfo;
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
}
void CoreWinMain::setup_volume_local() {
	//test asset demo
	std::vector<datasetResponse::datasetInfo> ds = m_data_manager->getAvailableDataset(true);
	auto dsName = ds[0].folder_name();
	std::vector<volumeInfo> vl;
	m_data_manager->getAvailableVolumes(dsName, vl, true);

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
	std::wstring dir_name(dvr::CACHE_FOLDER_NAME.begin(), dvr::CACHE_FOLDER_NAME.end());
	Windows::Storage::StorageFolder^ dst_folder = Windows::Storage::ApplicationData::Current->LocalFolder;

	auto copy_func = []() {
		std::string file_name = dvr::CACHE_FOLDER_NAME + "\\" + dvr::CONFIG_NAME;
		std::ifstream inFile("Assets\\" + file_name, std::ios::in | std::ios::binary);
		if (!inFile.is_open())
			return false;
		std::ofstream outFile(DX::getFilePath(file_name), std::ios::out | std::ios::binary);
		if (!outFile.is_open())
			return false;
		outFile << inFile.rdbuf();
		inFile.close();
		outFile.close();
		return true;
	};
	auto post_copy_func = [this]() {
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
		setup_volume_local();
		//setup_volume_server();
	};
	create_task(dst_folder->CreateFolderAsync(Platform::StringReference(dir_name.c_str()), CreationCollisionOption::OpenIfExists))
		.then([=](StorageFolder^ folder) {
		std::wstring index_file_name(dvr::CONFIG_NAME.begin(), dvr::CONFIG_NAME.end());
		if (!m_overwrite_index_file) {
			create_task(folder->TryGetItemAsync(Platform::StringReference(index_file_name.c_str()))).then([this, copy_func, post_copy_func](IStorageItem^ data) {
				if (data != nullptr) post_copy_func();
				else if (copy_func()) post_copy_func();
			});
		}
		else {
			if (copy_func())post_copy_func();
		}
	});
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
	Size outputSize = m_deviceResources->GetOutputSize();
	m_manager->onViewChange(outputSize.Width, outputSize.Height);
}

// Updates the application state once per frame.
void CoreWinMain::Update(){
	if (rpcHandler::new_data_request) {
		auto dmsg = m_rpcHandler->GetNewDataRequest();
		m_data_manager->loadData(dmsg.ds_name(), dmsg.volume_name());
		rpcHandler::new_data_request = false;
	}
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
	ID3D11RenderTargetView* const targets[1] = { m_deviceResources->GetBackBufferRenderTargetView() };
	context->OMSetRenderTargets(1, targets, m_deviceResources->GetDepthStencilView());

	// Clear the back buffer and depth stencil view.
	context->ClearRenderTargetView(m_deviceResources->GetBackBufferRenderTargetView(), dvr::SCREEN_CLEAR_COLOR);
	context->ClearDepthStencilView(m_deviceResources->GetDepthStencilView(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	// Render the scene objects.
	// TODO: Replace this with your app's content rendering functions.
	m_sceneRenderer->Render();
	m_fpsTextRenderer->Render();

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