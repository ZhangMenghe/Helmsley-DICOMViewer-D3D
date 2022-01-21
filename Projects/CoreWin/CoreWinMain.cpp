#include "pch.h"
#include "CoreWinMain.h"
#include <ppltasks.h>
#include <Common/DirectXHelper.h>

using namespace CoreWin;

// Loads and initializes application assets when the application is loaded.
CoreWinMain::CoreWinMain(const std::shared_ptr<DX::DeviceResources>& deviceResources) :
	m_deviceResources(deviceResources){
	// Register to be notified if the Device is lost or recreated
	m_deviceResources->RegisterDeviceNotify(this);

	m_manager = std::make_shared<Manager>();

	m_sceneRenderer = std::make_unique<vrController>(m_deviceResources, m_manager);
	m_manager->addMVPStatus("CoreCam", true);

	m_static_uiboard = std::make_unique<overUIBoard>(m_deviceResources);
	m_static_uiboard->AddBoard("fps", glm::vec3(0.8, -0.8, dvr::DEFAULT_VIEW_Z), glm::vec3(0.3, 0.2, 0.2), glm::rotate(glm::mat4(1.0), 0.2f, glm::vec3(.0, 1.0, .0)), D2D1::ColorF::Chocolate);
	
	m_popup_uiboard = std::make_unique<overUIBoard>(m_deviceResources);
	m_popup_uiboard->CreateBackgroundBoard(glm::vec3(.0, .0, dvr::DEFAULT_VIEW_Z * 0.5f), glm::vec3(0.3, 0.4, 0.2));
	m_popup_uiboard->AddBoard("Annotation");
	m_popup_uiboard->AddBoard("Broadcast", L"Broadcast", L"Listen", rpcHandler::G_STATUS_SENDER);

	m_dicom_loader = std::make_shared<dicomLoader>();

	auto outputSize = m_deviceResources->GetOutputSize();
	m_manager->onViewChange(outputSize.Width, outputSize.Height);
	m_uiController.InitAll();

	setup_resource();
}
void CoreWinMain::setup_volume_server(){
	//test remote
	std::vector<datasetResponse::datasetInfo> ds = m_data_manager->getAvailableDataset(false);
	std::vector<helmsley::volumeInfo> vl;
	m_data_manager->getAvailableVolumes("IRB01", vl, false);

	helmsley::volumeInfo vInfo;
	for (auto vli : vl) {
		if (vli.folder_name().compare("series_3_optional__trufi_1_cine") == 0) {
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
	std::vector<helmsley::volumeInfo> vl;
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
	auto task = concurrency::create_task([] {
		return DX::CopyAssetData("helmsley_cached/pacs_local.txt", "helmsley_cached\\pacs_local.txt", false).get();
	});
	task.then([this](bool result) {
		if(!result) std::cerr << "Fail to copy";
		m_local_initialized = result;
	});

}
CoreWinMain::~CoreWinMain(){
	// Deregister device notification
	m_deviceResources->RegisterDeviceNotify(nullptr);
}

// Updates application state when the window size changes (e.g. device orientation change)
void CoreWinMain::CreateWindowSizeDependentResources()
{
	winrt::Windows::Foundation::Size outputSize = m_deviceResources->GetOutputSize();

	m_sceneRenderer->onViewChanged(outputSize.Width, outputSize.Height);
	m_manager->onViewChange(outputSize.Width, outputSize.Height);
	m_popup_uiboard->onWindowSizeChanged();
}

// Updates the application state once per frame.
void CoreWinMain::Update(){
	if (m_local_initialized) {
		if (dvr::CONNECT_TO_SERVER)
		{
			m_rpcHandler = std::make_shared<rpcHandler>("localhost:23333");
			m_rpcThread = new std::thread(&rpcHandler::Run, m_rpcHandler);
			m_rpcHandler->setDataLoader(m_dicom_loader);
			m_rpcHandler->setVRController(m_sceneRenderer.get());
			m_rpcHandler->setManager(m_manager.get());
			m_rpcHandler->setUIController(&m_uiController);
			m_data_manager = new dataManager(m_dicom_loader, m_rpcHandler);
		}
		else
		{
			m_data_manager = new dataManager(m_dicom_loader);
		}
		setup_volume_local();
		m_local_initialized = false;
	}
	if (rpcHandler::new_data_request) {
		auto dmsg = m_rpcHandler->GetNewDataRequest();
		m_data_manager->loadData(dmsg.ds_name(), dmsg.volume_name());
		rpcHandler::new_data_request = false;
	}
	//if (rpcHandler::G_FORCED_STOP_BROADCAST) {
	//	m_ui_board->Update("broadcast", L"Listen");
	//	rpcHandler::G_FORCED_STOP_BROADCAST = false;
	//}
	// Update scene objects.
	m_timer.Tick([&]()
	{
		m_dicom_loader->onUpdate();
		//m_fpsTextRenderer->Update(m_timer);
		uint32 fps = m_timer.GetFramesPerSecond();
		m_static_uiboard->Update("fps", (fps > 0) ? std::to_wstring(fps) + L" FPS" : L" - FPS");
	});
}

// Renders the current frame according to the current application state.
// Returns true if the frame was rendered and is ready to be displayed.
bool CoreWinMain::Render(){
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
	m_sceneRenderer->Render(0);
	//m_fpsTextRenderer->Render();
	m_static_uiboard->Render();
	if(m_pop_up_ui_visible)m_popup_uiboard->Render();
	return true;
}

// Notifies renderers that device resources need to be released.
void CoreWinMain::OnDeviceLost()
{
}

// Notifies renderers that device resources may now be recreated.
void CoreWinMain::OnDeviceRestored()
{
	//m_fpsTextRenderer->CreateDeviceDependentResources();
	CreateWindowSizeDependentResources();
}
void CoreWinMain::OnPointerPressed(float x, float y) {
	if (m_pop_up_ui_visible) {
		std::string hit_name;
		m_popup_uiboard->CheckHit(m_timer.GetFrameCount(), hit_name, x, y);
		if (hit_name == "Annotation") {

		}
		else if (hit_name == "Broadcast") {
			//m_rpcHandler->onBroadCastChanged();
		}

		//if (m_popup_uiboard->CheckHit("Broadcast", x, y)) {
		//	std::cout << "hit" << std::endl;
		//}
		//hit test
		//std::string hit_name;
		//m_popup_uiboard->CheckHit(m_timer.GetFrameCount(), hit_name, glm::vec3(x, y, .0f), 0.1f);
		//if (hit_name == "Annotation") {

		//}
		//else if (hit_name == "Broadcast") {
			//m_rpcHandler->onBroadCastChanged();
		//}
	}
	else {
		m_sceneRenderer->onSingleTouchDown(x, y);
		if (rpcHandler::G_STATUS_SENDER)m_rpcHandler->setGestureOp(helmsley::GestureOp_OPType_TOUCH_DOWN, x, y);
	}
}
void CoreWinMain::OnPointerMoved(float x, float y) {
	if (m_waitfor_operation) {
		m_sceneRenderer->onTouchMove(x, y);
		if (rpcHandler::G_STATUS_SENDER)m_rpcHandler->setGestureOp(helmsley::GestureOp_OPType_TOUCH_MOVE, x, y);
	}
}
void CoreWinMain::OnPointerReleased() {
	if (m_waitfor_operation) {
		m_sceneRenderer->onTouchReleased();
		if (rpcHandler::G_STATUS_SENDER)m_rpcHandler->setGestureOp(helmsley::GestureOp_OPType_TOUCH_UP, 0, 0);
	}
}