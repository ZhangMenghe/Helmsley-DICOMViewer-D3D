#include "pch.h"
#include "OXRScenes.h"
#include <Common/DirectXHelper.h>
#include "OXRRenderer/OpenCVFrameProcessing.h"
OXRScenes::OXRScenes(const std::shared_ptr<xr::XrContext>& context)
	:xr::Scene(context) {
	m_manager = std::make_shared<Manager>();
}

void OXRScenes::SetupDeviceResource(const std::shared_ptr<DX::DeviceResources>& deviceResources) {
	m_sceneRenderer = std::unique_ptr<vrController>(new vrController(deviceResources, m_manager));
	m_sceneRenderer->InitOXRScene();
	m_deviceResources = deviceResources;
	m_scenario = std::unique_ptr<SensorVizScenario>(new SensorVizScenario(m_context));

	//m_fpsTextRenderer = std::unique_ptr<FpsTextRenderer>(new FpsTextRenderer(m_deviceResources));

	m_dicom_loader = std::make_shared<dicomLoader>();

	m_uiController.InitAll();
	setup_resource();
}

void OXRScenes::setup_volume_server()
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
void OXRScenes::setup_volume_local()
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
void OXRScenes::setup_resource()
{
	auto task = concurrency::create_task([] {
		return DX::CopyAssetData("helmsley_cached/pacs_local.txt", "helmsley_cached\\pacs_local.txt", false).get();
	});
	task.then([this](bool result) {
		if (!result) std::cerr << "Fail to copy";
		m_local_initialized = result;
	});
}
void OXRScenes::onViewChanged(){
	m_sceneRenderer->CreateWindowSizeDependentResources();
}
void OXRScenes::setSpaces(XrSpace *space, XrSpace *app_space){
	this->space = space;
	this->app_space = app_space;
}
void OXRScenes::Update(const xr::FrameTime& frameTime)
{
	if (m_local_initialized) {
		if (dvr::CONNECT_TO_SERVER)		{
			m_rpcHandler = std::make_shared<rpcHandler>("192.168.1.74:23333");
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
		m_scenario->Update(m_timer);
		m_sceneRenderer->Update(m_timer);
		//m_fpsTextRenderer->Update(m_timer);
	});
}
void OXRScenes::BeforeRender(const xr::FrameTime& frameTime) {

}


void OXRScenes::Render(const xr::FrameTime& frameTime, uint32_t view_id){
	if (m_timer.GetFrameCount() == 0)
	{
		return;
	}
	//if (view_id == 0)
	//{
	//	m_render_scene = m_scenario->Render();
	//}
	if (m_render_scene) {
		m_sceneRenderer->Render(view_id);
	}

	//m_fpsTextRenderer->Render();

	/*m_text_texture->Draw(L"asdfasd");

	m_tex_quad->Draw(m_deviceResources->GetD3DDeviceContext(),
		DirectX::XMMatrixScaling(0.3, 0.15, 0.2));*/
}