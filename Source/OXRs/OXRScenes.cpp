#include "pch.h"
#include "OXRScenes.h"
#include <Common/DirectXHelper.h>

OXRScenes::OXRScenes(const std::shared_ptr<DX::DeviceResources>& deviceResources)
	: m_deviceResources(deviceResources) {
	m_manager = std::make_shared<Manager>();

	m_sceneRenderer = std::unique_ptr<vrController>(new vrController(deviceResources, m_manager));
	m_sceneRenderer->InitOXRScene();

	//m_fpsTextRenderer = std::unique_ptr<FpsTextRenderer>(new FpsTextRenderer(m_deviceResources));

	m_dicom_loader = std::make_shared<dicomLoader>();

	m_uiController.InitAll();

	setup_resource();
}
void OXRScenes::setup_volume_server() {
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
void OXRScenes::setup_volume_local() {
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
void OXRScenes::setup_resource() {
	if (!DX::CopyAssetData("helmsley_cached/pacs_local.txt", "helmsley_cached\\pacs_local.txt", false).get())
		std::cerr << "Fail to copy";

	//std::wstring dir_name(dvr::CACHE_FOLDER_NAME.begin(), dvr::CACHE_FOLDER_NAME.end());
	//Windows::Storage::StorageFolder^ dst_folder = Windows::Storage::ApplicationData::Current->LocalFolder;

	//auto copy_func = []() {
	//	std::string file_name = dvr::CACHE_FOLDER_NAME + "\\" + dvr::CONFIG_NAME;
	//	std::ifstream inFile("Assets\\" + file_name, std::ios::in | std::ios::binary);
	//	if (!inFile.is_open())
	//		return false;
	//	std::ofstream outFile(DX::getFilePath(file_name), std::ios::out | std::ios::binary);
	//	if (!outFile.is_open())
	//		return false;
	//	outFile << inFile.rdbuf();
	//	inFile.close();
	//	outFile.close();
	//	return true;
	//};
	//auto post_copy_func = [this]() {
	//	
	//	//if (dvr::CONNECT_TO_SERVER) {
	//	//	m_rpcHandler = std::make_shared<rpcHandler>("10.68.2.105:23333");
	//	//	m_rpcThread = new std::thread(&rpcHandler::Run, m_rpcHandler);
	//	//	m_rpcHandler->setDataLoader(m_dicom_loader);
	//	//	m_rpcHandler->setVRController(m_sceneRenderer.get());
	//	//	m_rpcHandler->setManager(m_manager.get());
	//	//	m_rpcHandler->setUIController(&m_uiController);
	//	//	m_data_manager = new dataManager(m_dicom_loader, m_rpcHandler);
	//	//}
	//	//else {
	//	//	m_data_manager = new dataManager(m_dicom_loader);
	//	//}
	//	//setup_volume_local();
	//	//setup_volume_server();
	//};
	//create_task(dst_folder->CreateFolderAsync(Platform::StringReference(dir_name.c_str()), CreationCollisionOption::OpenIfExists))
	//	.then([=](StorageFolder^ folder) {
	//	std::wstring index_file_name(dvr::CONFIG_NAME.begin(), dvr::CONFIG_NAME.end());
	//	if (!m_overwrite_index_file) {
	//		create_task(folder->TryGetItemAsync(Platform::StringReference(index_file_name.c_str()))).then([this, copy_func, post_copy_func](IStorageItem^ data) {
	//			if (data != nullptr) post_copy_func();
	//			else if (copy_func()) post_copy_func();
	//		});
	//	}
	//	else {
	//		if (copy_func())post_copy_func();
	//	}
	//});
	/*if (!DX::CopyAssetData("helmsley_cached/pacs_local.txt", "helmsley_cached\\pacs_local.txt", false))
		std::cerr << "Fail to copy";*/
	if (dvr::CONNECT_TO_SERVER) {
		m_rpcHandler = std::make_shared<rpcHandler>("10.68.2.105:23333");
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
//>>>>>>> 2fdada30c36168ffc76a01b037759300a062193a
}
void OXRScenes::onViewChanged()
{
	m_sceneRenderer->CreateWindowSizeDependentResources();
}
void OXRScenes::setSpaces(XrSpace* space, XrSpace* app_space)
{
	this->space = space;
	this->app_space = app_space;
	//this->m_sceneRenderer->setSpaces(space, app_space);
}
void OXRScenes::Update() {
	if (rpcHandler::new_data_request) {
		auto dmsg = m_rpcHandler->GetNewDataRequest();
		m_data_manager->loadData(dmsg.ds_name(), dmsg.volume_name());
		rpcHandler::new_data_request = false;
	}
	m_timer.Tick([&]() {
		m_sceneRenderer->Update(m_timer);
		//m_fpsTextRenderer->Update(m_timer);
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
	//m_fpsTextRenderer->Render();

	/*m_text_texture->Draw(L"asdfasd");

	m_tex_quad->Draw(m_deviceResources->GetD3DDeviceContext(),
		DirectX::XMMatrixScaling(0.3, 0.15, 0.2));*/
}