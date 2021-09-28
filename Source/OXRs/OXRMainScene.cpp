#include "pch.h"
#include "OXRMainScene.h"
#include <Common/DirectXHelper.h>
#include "OXRRenderer/OpenCVFrameProcessing.h"
OXRMainScene::OXRMainScene(const std::shared_ptr<xr::XrContext>& context)
	:xr::Scene(context) {
	m_manager = std::make_shared<Manager>();
}

void OXRMainScene::SetupDeviceResource(const std::shared_ptr<DX::DeviceResources>& deviceResources) {
	m_sceneRenderer = std::unique_ptr<vrController>(new vrController(deviceResources, m_manager));
	m_manager->addMVPStatus("OXRCam", dvr::DEFAULT_ROTATE, glm::vec3(0.5f), dvr::DEFAULT_POS, new Camera(), true);

	m_deviceResources = deviceResources;
	m_scenario = new MarkerBasedScenario(m_context);

	m_ui_board = std::make_unique<overUIBoard>(m_deviceResources);
	m_ui_board->AddBoard("fps", glm::vec3(0.3, -0.3, dvr::DEFAULT_VIEW_Z), glm::vec3(0.3, 0.2, 0.2), glm::rotate(glm::mat4(1.0), 0.2f, glm::vec3(.0, 1.0, .0)));
	m_ui_board->AddBoard("broadcast", glm::vec3(-0.3, 0.1, -0.1f), glm::vec3(0.15f, 0.1f, 0.1f), glm::mat4(1.0));
	m_ui_board->Update("broadcast", rpcHandler::G_STATUS_SENDER ? L"Broadcast" : L"Listen");

	m_dicom_loader = std::make_shared<dicomLoader>();

	m_uiController.InitAll();
	setup_resource();
}

void OXRMainScene::setup_volume_server()
{
	//test remote
	std::vector<datasetResponse::datasetInfo> ds = m_data_manager->getAvailableDataset(false);
	std::vector<helmsley::volumeInfo> vl;
	m_data_manager->getAvailableVolumes("IRB01", vl, false);

	helmsley::volumeInfo vInfo;
	for (auto vli : vl)
	{
		if (vli.folder_name().compare("2100_FATPOSTCORLAVAFLEX20secs") == 0)
		{
			vInfo = vli;
			break;
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
void OXRMainScene::setup_volume_local()
{
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
void OXRMainScene::setup_resource()
{
	auto task = concurrency::create_task([] {
		return DX::CopyAssetData("helmsley_cached/pacs_local.txt", "helmsley_cached\\pacs_local.txt", false).get();
	});
	task.then([this](bool result) {
		if (!result) std::cerr << "Fail to copy";
		m_local_initialized = result;
	});
}
void OXRMainScene::onViewChanged(){
	winrt::Windows::Foundation::Size outputSize = m_deviceResources->GetOutputSize();
	m_sceneRenderer->onViewChanged(outputSize.Width, outputSize.Height);
	m_ui_board->CreateWindowSizeDependentResources(outputSize.Width, outputSize.Height);
}
void OXRMainScene::Update(const xr::FrameTime& frameTime)
{
	if (m_local_initialized) {
		if (dvr::CONNECT_TO_SERVER)		{
			m_rpcHandler = std::make_shared<rpcHandler>("192.168.0.128:23333");
			m_rpcThread = new std::thread(&rpcHandler::Run, m_rpcHandler);
			m_rpcHandler->setDataLoader(m_dicom_loader);
			m_rpcHandler->setVRController(m_sceneRenderer.get());
			m_rpcHandler->setManager(m_manager.get());
			m_rpcHandler->setUIController(&m_uiController);
			m_data_manager = new dataManager(m_dicom_loader, m_rpcHandler);
		}
		else		{
			m_data_manager = new dataManager(m_dicom_loader);
		}
		setup_volume_local();
		//setup_volume_server();
		m_local_initialized = false;
	}
	if (rpcHandler::new_data_request)	{
		auto dmsg = m_rpcHandler->GetNewDataRequest();
		m_data_manager->loadData(dmsg.ds_name(), dmsg.volume_name());
		rpcHandler::new_data_request = false;
	}
	m_timer.Tick([&]() {
		m_dicom_loader->onUpdate();
		m_scenario->Update();
		uint32 fps = m_timer.GetFramesPerSecond();
		m_ui_board->Update("fps", (fps > 0) ? std::to_wstring(fps) + L" FPS" : L" - FPS");
	});
}
void OXRMainScene::BeforeRender(const xr::FrameTime& frameTime) {

}


void OXRMainScene::Render(const xr::FrameTime& frameTime, uint32_t view_id){
	if (m_timer.GetFrameCount() == 0)
	{
		return;
	}
	m_sceneRenderer->Render(view_id);
	m_ui_board->Render();
}

void OXRMainScene::onSingle3DTouchDown(float x, float y, float z, int side) {
	//check sphere-plane intersection
	if (m_ui_board->CheckHit("broadcast", x, y, z)) {
		m_rpcHandler->onBroadCastChanged();
		m_ui_board->Update("broadcast", rpcHandler::G_STATUS_SENDER ? L"Broadcast" : L"Listen");
	}
	else {
		m_sceneRenderer->onSingle3DTouchDown(x, y, z, side);
		//if (rpcHandler::G_STATUS_SENDER)m_rpcHandler->setGestureOp(helmsley::GestureOp_OPType_TOUCH_DOWN, x, y);
	}
	//m_scenario->onSingle3DTouchDown(x, y, z, side);
}
void OXRMainScene::on3DTouchMove(float x, float y, float z, glm::mat4 rot, int side) {
	m_sceneRenderer->on3DTouchMove(x, y, z, rot, side);
	//if (rpcHandler::G_STATUS_SENDER)m_rpcHandler->setGestureOp(helmsley::GestureOp_OPType_TOUCH_MOVE, x, y);
};
void OXRMainScene::on3DTouchReleased(int side) { 
	m_sceneRenderer->on3DTouchReleased(side); 
	//if (rpcHandler::G_STATUS_SENDER)m_rpcHandler->setGestureOp(helmsley::GestureOp_OPType_TOUCH_UP, 0, 0);
};
