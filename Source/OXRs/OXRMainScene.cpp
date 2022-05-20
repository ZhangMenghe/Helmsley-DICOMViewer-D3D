
#include "pch.h"
#include "OXRMainScene.h"
#include <Common/DirectXHelper.h>
#include "OXRRenderer/OpenCVFrameProcessing.h"
#include <SceneObjs/handSystem.h>

OXRMainScene::OXRMainScene(const std::shared_ptr<xr::XrContext>& context)
	:xr::Scene(context) {
	m_manager = std::make_shared<Manager>();
}

void OXRMainScene::SetupDeviceResource(const std::shared_ptr<DX::DeviceResources>& deviceResources) {
	m_sceneRenderer = std::unique_ptr<vrController>(new vrController(deviceResources, m_manager));
	m_manager->addMVPStatus("OXRCam", dvr::DEFAULT_ROTATE, glm::vec3(0.2f), dvr::DEFAULT_POS, true);

	m_deviceResources = deviceResources;
	//m_scenario = new MarkerBasedScenario(m_context);

	m_static_uiboard = std::make_unique<overUIBoard>(m_deviceResources);
	m_static_uiboard->AddBoard("fps", 
		glm::vec3(0.3, -0.3, dvr::DEFAULT_VIEW_Z), glm::vec3(0.2, 0.1, 0.1), glm::rotate(glm::mat4(1.0), 0.2f, glm::vec3(.0, 1.0, .0)),
		D2D1::ColorF::Chocolate);
	
	m_popup_uiboard = std::make_unique<overUIBoard>(m_deviceResources);
	m_popup_uiboard->CreateBackgroundBoard(glm::vec3(.0, .0, dvr::DEFAULT_VIEW_Z), glm::vec3(0.3, 0.4, 0.2));
	m_popup_uiboard->AddBoard("Annotation");
	m_popup_uiboard->AddBoard("Broadcast", L"Broadcast", L"Listen", rpcHandler::G_STATUS_SENDER);

	m_annotation_uiboard = std::make_unique<overUIBoard>(m_deviceResources);
	m_annotation_uiboard->CreateBackgroundBoard(glm::vec3(-0.2f, 0.2f, dvr::DEFAULT_VIEW_Z), glm::vec3(0.4, 0.05, 0.2));
	m_annotation_uiboard->AddBoard("Brush", 1, 2, 1, L"", L"", true);
	m_annotation_uiboard->AddBoard("StepOver", 1, 2, 2);

	m_draw_board = std::make_unique<overUIBoard>(m_deviceResources);
	m_draw_board->CreateBackgroundBoard(glm::vec3(.0, .0, dvr::DEFAULT_VIEW_Z), glm::vec3(0.8, 0.6, 0.2), true);

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

		return DX::CopyAssetData("textures/gizmo", "textures\\gizmo", false).get()
			&& DX::CopyAssetData("textures/gizmo-bounding.txt", "textures\\gizmo-bounding.txt", false).get()
			&& DX::CopyAssetData("helmsley_cached/pacs_local.txt", "helmsley_cached\\pacs_local.txt", false).get();
	});
	task.then([this](bool result) {
		if (!result) std::cerr << "Fail to copy";
		m_local_initialized = result;
	});
}
void OXRMainScene::onViewChanged(){
	//winrt::Windows::Foundation::Size outputSize = m_deviceResources->GetOutputSize();
	//m_sceneRenderer->onViewChanged(outputSize.Width, outputSize.Height);
}

bool OXRMainScene::check_ui_hit(const xr::FrameTime& frameTime) {
	std::string hit_name;
	XrVector3f pos; float radius;
	m_hand_sys->getCurrentTouchPosition(pos, radius);

	if (m_drawcanvas_visible && m_draw_board->DrawOnBoard(frameTime.FrameIndex, hit_name, glm::vec3(pos.x, pos.y, pos.z), radius)) return true;

	m_pop_up_ui_visible = (m_hand_sys->getClapNum() % 2 == 1);
	if (m_pop_up_ui_visible) {
		m_popup_uiboard->CheckHit(frameTime.FrameIndex, hit_name, glm::vec3(pos.x, pos.y, pos.z), radius);
		if (hit_name == "Annotation") {
			m_gizmo_visible = !m_gizmo_visible; return true;
		}
		if (hit_name == "Broadcast") {
			//m_rpcHandler->onBroadCastChanged();
			return true;
		}
	}
	//check gizmo menu
	if (m_gizmo_visible) {
		std::string hit_name;
		if (m_annotation_uiboard->CheckHit(frameTime.FrameIndex, hit_name, glm::vec3(pos.x, pos.y, pos.z), radius)) {
			if (m_annotation_uiboard->IsSelected(hit_name)) m_annotation_uiboard->FilterBoardSelection(hit_name);
			return true;
		}
		//check arrow
		auto is_brush = m_annotation_uiboard->IsSelected("Brush");
		auto is_step = m_annotation_uiboard->IsSelected("StepOver");
		if (is_brush || is_step) {
			int hit_id;
			if (m_gizmo_button->CheckHit(glm::vec3(pos.x, pos.y, pos.z), radius, hit_id)) {
				m_sceneRenderer->onTouchMoveAnnotation((dvr::ANNOTATE_DIR)hit_id, is_brush);
				return true;
			}
		}
	}
	return false;
}
void OXRMainScene::Update(const xr::FrameTime& frameTime){
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
		else{
			m_data_manager = new dataManager(m_dicom_loader);
		}
		setup_volume_local();
		//setup_volume_server();

		m_gizmo_button = new templateButton(m_deviceResources,
			"textures\\gizmo", "textures\\gizmo-bounding.txt", false,
			400, 400,
			glm::vec3(-0.2f, -0.2f, dvr::DEFAULT_VIEW_Z), glm::vec3(0.35, 0.35, 0.2), glm::mat4(1.0f));

		m_local_initialized = false;
	}
	if (rpcHandler::new_data_request)	{
		auto dmsg = m_rpcHandler->GetNewDataRequest();
		m_data_manager->loadData(dmsg.ds_name(), dmsg.volume_name());
		rpcHandler::new_data_request = false;
	}


	//Check if UI needs updating
	check_ui_hit(frameTime);

	//
	m_timer.Tick([&]() {
		m_dicom_loader->onUpdate();
		//m_scenario->Update();
		uint32 fps = m_timer.GetFramesPerSecond();
		m_static_uiboard->Update("fps", (fps > 0) ? std::to_wstring(fps) + L" FPS" : L" - FPS");
	});
}
void OXRMainScene::BeforeRender(const xr::FrameTime& frameTime) {

}


void OXRMainScene::Render(const xr::FrameTime& frameTime, uint32_t view_id){
	if (m_timer.GetFrameCount() == 0)
	{
		return;
	}
	if(m_draw_volume) m_sceneRenderer->Render(view_id);
	m_static_uiboard->Render();
	if (m_pop_up_ui_visible) m_popup_uiboard->Render();
	if (m_drawcanvas_visible) m_draw_board->Render();
	if (m_gizmo_visible) { m_gizmo_button->Render(); m_annotation_uiboard->Render(); }
	m_hand_sys->Draw(m_deviceResources->GetD3DDeviceContext());
}

void OXRMainScene::onSingle3DTouchDown(float x, float y, float z, int side) {
	if (m_draw_volume) m_sceneRenderer->onSingle3DTouchDown(x, y, z, side);
}
void OXRMainScene::on3DTouchMove(float x, float y, float z, glm::mat4 rot, int side) {
	if (!m_draw_volume) return;
	std::vector<dvr::OXR_POSE_TYPE> types;
	m_sceneRenderer->on3DTouchMove(x, y, z, rot, side, types);
	//SYNC: send out volume pose
	if (rpcHandler::G_STATUS_SENDER && /*m_scenario->IsTracking() &&*/ !types.empty()) {
		glm::vec3 t, s; glm::quat rot;
		m_sceneRenderer->getRST(t, s, rot);
		float* data = new float[4];
		for (auto type : types) {
			if (type == dvr::POSE_ROTATE) {
				data[0] = rot.w; data[1] = rot.x; data[2] = rot.y; data[3] = rot.z;
			}
			else if (type == dvr::POSE_SCALE) {
				data[0] = s.x; data[1] = s.y; data[2] = s.z;
			}
			else {
				data[0] = t.x; data[1] = t.y; data[2] = t.z;
			}
			m_rpcHandler->setVolumePose((helmsley::VPMsg::VPType)type, data);
		}
	}
};
void OXRMainScene::on3DTouchReleased(int side) { 
	if (m_draw_volume) m_sceneRenderer->on3DTouchReleased(side);
	//if (rpcHandler::G_STATUS_SENDER)m_rpcHandler->setGestureOp(helmsley::GestureOp_OPType_TOUCH_UP, 0, 0);
};
