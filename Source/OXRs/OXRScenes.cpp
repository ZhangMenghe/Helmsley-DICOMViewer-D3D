#include "pch.h"
#include "OXRScenes.h"
#include <Common/DirectXHelper.h>
OXRScenes::OXRScenes(const std::shared_ptr<DX::DeviceResources>& deviceResources)
	:m_deviceResources(deviceResources) {
	m_manager = std::make_shared<Manager>();

	m_sceneRenderer = std::unique_ptr<vrController>(new vrController(deviceResources, m_manager));

	m_fpsTextRenderer = std::unique_ptr<FpsTextRenderer>(new FpsTextRenderer(m_deviceResources));

	if (dvr::CONNECT_TO_SERVER) {
		m_rpcHandler = new rpcHandler("10.68.2.105:23333");
		m_rpcThread = new std::thread(&rpcHandler::Run, m_rpcHandler);
		m_rpcHandler->setDataLoader(&m_dicom_loader);
		m_rpcHandler->setVRController(m_sceneRenderer.get());
		m_rpcHandler->setManager(m_manager.get());
		m_rpcHandler->setUIController(&m_uiController);
		dvr::LOAD_DATA_FROM_SERVER? setup_volume_server() : setup_volume_local();
	}else {
		setup_volume_local();
	}
	m_uiController.InitAll();

	/*
	std::vector<std::string> contents;
	if (!DX::CopyAssetData("helmsley_cached/pacs_local.txt", "helmsley_cached\\pacs_local.txt"))
		std::cerr << "Fail to copy";
	else if (!DX::ReadAllLines("helmsley_cached\\pacs_local.txt", contents))
		std::cerr << "File not exist";
	for (auto line : contents)
		std::cout << line << std::endl;
		*/
}
void OXRScenes::setup_volume_server() {
	auto vector = m_rpcHandler->getVolumeFromDataset("IRB01");

	if (vector.size() > 0) {
		volumeResponse::volumeInfo sel_vol_info;// = vector[0];
		for (auto vol : vector) {
			if (vol.folder_name().compare("2100_FATPOSTCORLAVAFLEX20secs") == 0) {
				sel_vol_info = vol;
				break;
			}
		}
		auto vdims = sel_vol_info.dims();
		auto spacing = sel_vol_info.resolution();
		m_dicom_loader.sendDataPrepare(
			vdims.Get(0), vdims.Get(1), vdims.Get(2),
			spacing.Get(0) * vdims.Get(0), spacing.Get(1) * vdims.Get(1), spacing.Get(2) * vdims.Get(2),
			sel_vol_info.with_mask());

		std::string path = m_rpcHandler->target_ds.folder_name() + '/' + sel_vol_info.folder_name();
		m_rpcHandler->DownloadVolume(path);
		m_rpcHandler->DownloadMasksAndCenterlines(path);
		m_dicom_loader.sendDataDone();
	}
	else {
		setup_volume_local();
	}
}
void OXRScenes::setup_volume_local() {
	m_dicom_loader.sendDataPrepare(vol_dims.x, vol_dims.y, vol_dims.z, -1, -1, -1, true);
	if (m_dicom_loader.loadData(m_ds_path + "data", m_ds_path + "mask", true)) {
		m_sceneRenderer->assembleTexture(2, vol_dims.x, vol_dims.y, vol_dims.z, -1, -1, -1, m_dicom_loader.getVolumeData(), m_dicom_loader.getChannelNum());
		//m_sceneRenderer.reset();
	}
	//m_dicom_loader.setupCenterLineData(m_sceneRenderer.get(), m_ds_path + "centerline.txt");
	m_dicom_loader.sendDataDone();
}

void OXRScenes::onViewChanged() {
	m_sceneRenderer->CreateWindowSizeDependentResources();
}
void OXRScenes::Update() {
	m_timer.Tick([&]()
	{
		m_sceneRenderer->Update(m_timer);
		m_fpsTextRenderer->Update(m_timer);
	});
}
bool OXRScenes::Render() {
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