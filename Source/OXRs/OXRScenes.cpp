#include "pch.h"
#include "OXRScenes.h"
OXRScenes::OXRScenes(const std::shared_ptr<DX::DeviceResources>& deviceResources)
:m_deviceResources(deviceResources) {
	m_manager = std::unique_ptr<Manager>(new Manager());

	m_sceneRenderer = std::unique_ptr<vrController>(new vrController(deviceResources));
	
	m_fpsTextRenderer = std::unique_ptr<FpsTextRenderer>(new FpsTextRenderer(m_deviceResources));

	//TextTextureInfo textInfo{ 256, 128 }; // pixels
	//textInfo.Margin = 5; // pixels
	//textInfo.TextAlignment = DWRITE_TEXT_ALIGNMENT_LEADING;
	//textInfo.ParagraphAlignment = DWRITE_PARAGRAPH_ALIGNMENT_NEAR;
	//m_text_texture = new TextTexture(m_deviceResources, textInfo);

	//m_tex_quad = new quadRenderer(deviceResources->GetD3DDevice());
	//m_tex_quad->setTexture(m_text_texture);

	/*m_tex_quad->setQuadSize(deviceResources->GetD3DDevice(),
		deviceResources->GetD3DDeviceContext(),
		300, 150);*/

	m_rpcHandler = new rpcHandler("localhost:23333");
	m_rpcThread = new std::thread(&rpcHandler::Run, m_rpcHandler);
	m_rpcHandler->setLoader(&m_dicom_loader);

	m_dicom_loader.setupDCMIConfig(vol_dims.x, vol_dims.y, vol_dims.z, -1, -1, -1, true);

	auto vector = m_rpcHandler->getVolumeFromDataset("Larry_Smarr_2017", false);

	if (vector.size() > 0) {
		std::string path = "Larry_Smarr_2017/" + vector[0].folder_name();//m_rpcHandler->target_ds.folder_name() + vector[0].folder_name();

		m_rpcHandler->DownloadVolume(path);
		m_rpcHandler->DownloadMasks(path);
		m_sceneRenderer->assembleTexture(2, vol_dims.x, vol_dims.y, vol_dims.z, -1, -1, -1, m_dicom_loader.getVolumeData(), m_dicom_loader.getChannelNum());

	}
	else {
		if (m_dicom_loader.loadData(m_ds_path + "data", m_ds_path + "mask")) {
			m_sceneRenderer->assembleTexture(2, vol_dims.x, vol_dims.y, vol_dims.z, -1, -1, -1, m_dicom_loader.getVolumeData(), m_dicom_loader.getChannelNum());
			//m_sceneRenderer.reset();
		}
	}

	
}
void OXRScenes::onViewChanged() {
	m_sceneRenderer->CreateWindowSizeDependentResources();
}
void OXRScenes::setSpaces(XrSpace * space, XrSpace * app_space) {
	this->space = space;
	this->app_space = app_space;
	//this->m_sceneRenderer->setSpaces(space, app_space);
}
void OXRScenes::Update() {
	m_timer.Tick([&]()
	{
		// TODO: Replace this with your app's content update functions.
		m_sceneRenderer->Update(m_timer);
		m_fpsTextRenderer->Update(m_timer);
	});
}

void OXRScenes::Update(XrTime time) {
	//m_sceneRenderer->Update(time);
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