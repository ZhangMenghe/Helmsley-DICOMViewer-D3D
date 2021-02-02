#include "pch.h"
#include "OXRScenes.h"
#include <Common/DirectXHelper.h>

OXRScenes::OXRScenes(const std::shared_ptr<DX::DeviceResources> &deviceResources)
		: m_deviceResources(deviceResources){
	setup_resource();

	m_manager = std::make_shared<Manager>();

	m_sceneRenderer = std::unique_ptr<vrController>(new vrController(deviceResources, m_manager));
	m_sceneRenderer->InitOXRScene();

	m_fpsTextRenderer = std::unique_ptr<FpsTextRenderer>(new FpsTextRenderer(m_deviceResources));

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

	m_uiController.InitAll();

	setup_volume_local();
	//setup_volume_server();
}
void OXRScenes::setup_volume_server(){	
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
}
void OXRScenes::setup_volume_local(){
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
void OXRScenes::setup_resource() {
	if (!DX::CopyAssetData("helmsley_cached/pacs_local.txt", "helmsley_cached\\pacs_local.txt", false))
		std::cerr << "Fail to copy";
}
void OXRScenes::onViewChanged()
{
	m_sceneRenderer->CreateWindowSizeDependentResources();
}
void OXRScenes::setSpaces(XrSpace *space, XrSpace *app_space)
{
	this->space = space;
	this->app_space = app_space;
	//this->m_sceneRenderer->setSpaces(space, app_space);
}
void OXRScenes::Update()
{
	m_timer.Tick([&]() {
		m_sceneRenderer->Update(m_timer);
		m_fpsTextRenderer->Update(m_timer);
	});
}

void OXRScenes::Update(XrTime time)
{
	//m_sceneRenderer->Update(time);
}

bool OXRScenes::Render()
{
	if (m_timer.GetFrameCount() == 0)
	{
		return false;
	}
	m_sceneRenderer->Render();
	m_fpsTextRenderer->Render();

	/*m_text_texture->Draw(L"asdfasd");

	m_tex_quad->Draw(m_deviceResources->GetD3DDeviceContext(),
		DirectX::XMMatrixScaling(0.3, 0.15, 0.2));*/
}