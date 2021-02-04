#include "pch.h"
#include "vrController.h"
#include <Common/Manager.h>
#include <Common/DirectXHelper.h>
#include <Utils/MathUtils.h>
#include <Utils/TypeConvertUtils.h>
using namespace dvr;
using namespace DirectX;
using namespace Windows::Foundation;

vrController* vrController::myPtr_ = nullptr;

vrController* vrController::instance() {
	return myPtr_;
}

// Loads vertex and pixel shaders from files and instantiates the cube geometry.
vrController::vrController(const std::shared_ptr<DX::DeviceResources>& deviceResources, const std::shared_ptr<Manager>& manager):
	m_tracking(false),
	m_deviceResources(deviceResources),
	m_manager(manager){
	myPtr_ = this;

	auto device = deviceResources->GetD3DDevice();
	screen_quad = new screenQuadRenderer(device);
	raycast_renderer = new raycastVolumeRenderer(device);
	texvrRenderer_ = new textureBasedVolumeRenderer(device);
	cutter_ = new cuttingController(device);
	data_board_ = new dataBoard(device);
	meshRenderer_ = new organMeshRenderer(device);
	Manager::camera = new Camera;

	CreateDeviceDependentResources();
	CreateWindowSizeDependentResources();
	onReset();
}
void vrController::onReset() {
	Mouse_old = { .0f, .0f };
	rStates_.clear();
	cst_name = "";
	addStatus("default_status");
	setMVPStatus("default_status");
	if (cutter_) cutter_->onReset(m_deviceResources->GetD3DDevice());
}

void vrController::onReset(glm::vec3 pv, glm::vec3 sv, glm::mat4 rm, Camera* cam) {
	Mouse_old = { .0f, .0f };
	rStates_.clear();
	cst_name = "";

	glm::mat4 mm = glm::translate(glm::mat4(1.0), pv)
		* rm
		* glm::scale(glm::mat4(1.0), sv);
	
	if(cam == nullptr)addStatus("template", mm, rm, sv, pv, Manager::camera);
	else addStatus("template", mm, rm, sv, pv, cam);
	
	setMVPStatus("template");
	if (cutter_) cutter_->onReset(m_deviceResources->GetD3DDevice());
	volume_model_dirty = false;
}

void vrController::assembleTexture(int update_target, UINT ph, UINT pw, UINT pd, float sh, float sw, float sd, UCHAR* data, int channel_num) {
	if (update_target == 0 || update_target == 2) {
		vol_dimension_ = { ph, pw, pd };
		if (sh <= 0 || sw <= 0 || sd <= 0) {
			if (pd > 200) vol_dim_scale_ = { 1.0f, 1.0f, 0.5f };
			else if (pd > 100) vol_dim_scale_ = { 1.0f, 1.0f, pd / 300.f };
			else vol_dim_scale_ = { 1.0f, 1.0f, pd / 200.f };
		}
		else if (abs(sh - sw) < 1) {
			vol_dim_scale_ = { 1.0f, 1.0f, sd / sh };
		}
		else {
			vol_dim_scale_ = { 1.0f, 1.0f, sd / fmax(sw, sh) };
			if (sw > sh) {
				ScaleVec3_.y *= sh / sw;
			}
			else {
				ScaleVec3_.x *= sw / sh;
			}
			volume_model_dirty = true;
		}
		vol_dim_scale_mat_ = glm::scale(glm::mat4(1.0f), vol_dim_scale_);
		m_manager->setDimension(vol_dimension_);
		texvrRenderer_->setDimension(m_deviceResources->GetD3DDevice(), vol_dimension_, vol_dim_scale_);
		cutter_->setDimension(pd, vol_dim_scale_.z);
	}

	if (tex_volume != nullptr) { delete tex_volume; tex_volume = nullptr; }

	tex_volume = new Texture;
	D3D11_TEXTURE3D_DESC texDesc{
		ph,pw,pd,
		1,
		DXGI_FORMAT_R32_UINT,
		D3D11_USAGE_DEFAULT,
		D3D11_BIND_RENDER_TARGET|D3D11_BIND_SHADER_RESOURCE,
		0,
		D3D11_RESOURCE_MISC_GENERATE_MIPS
	};
	if (!tex_volume->Initialize(m_deviceResources->GetD3DDevice(), m_deviceResources->GetD3DDeviceContext(), texDesc, data)) { delete tex_volume; tex_volume = nullptr; }
	tex_volume->GenerateMipMap(m_deviceResources->GetD3DDeviceContext());

	if (tex_baked != nullptr) { delete tex_baked; tex_baked = nullptr; }
	tex_baked = new Texture;
	texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	tex_baked->Initialize(m_deviceResources->GetD3DDevice(), texDesc);
	// Generate mipmaps for this texture.
	tex_baked->GenerateMipMap(m_deviceResources->GetD3DDeviceContext());
	init_texture();
	Manager::baked_dirty_ = true;

	meshRenderer_->Setup(m_deviceResources->GetD3DDevice(), ph, pw, pd);
}

void vrController::init_texture() {
	if (m_comp_tex_d3d != nullptr) { delete m_comp_tex_d3d; m_comp_tex_d3d = nullptr; }
	if (m_textureUAV != nullptr) { delete m_textureUAV; m_textureUAV = nullptr; }

	D3D11_TEXTURE3D_DESC texDesc{
		vol_dimension_.x,vol_dimension_.y,vol_dimension_.z,
		1,
		DXGI_FORMAT_R8G8B8A8_UNORM,
		D3D11_USAGE_DEFAULT,
		D3D11_BIND_UNORDERED_ACCESS,
		0,
		0
	};

	DX::ThrowIfFailed(
		m_deviceResources->GetD3DDevice()->CreateTexture3D(&texDesc, nullptr, &m_comp_tex_d3d)
	);

	D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE3D;
	uavDesc.Format = texDesc.Format;
	uavDesc.Texture3D.MipSlice = 0;
	uavDesc.Texture3D.FirstWSlice = 0;
	uavDesc.Texture3D.WSize = vol_dimension_.z;
	DX::ThrowIfFailed(
		m_deviceResources->GetD3DDevice()->CreateUnorderedAccessView(m_comp_tex_d3d, &uavDesc, &m_textureUAV)
	);
}

// Initializes view parameters when the window size changes.
void vrController::CreateWindowSizeDependentResources(){
	Size outputSize = m_deviceResources->GetOutputSize();
	if (outputSize.Width == .0) return;
	DX::ThrowIfFailed(screen_quad->InitializeQuadTex(m_deviceResources->GetD3DDevice(), m_deviceResources->GetD3DDeviceContext(), outputSize.Width, outputSize.Height));
}

// Called once per frame, rotates the cube and calculates the model and view matrices.
void vrController::Update(DX::StepTimer const& timer) {
	if (!m_tracking){
		// Convert degrees to radians, then convert seconds to rotation angle
		float radiansPerSecond = XMConvertToRadians(m_degreesPerSecond);
		double totalRotation = timer.GetTotalSeconds() * radiansPerSecond;
		float radians = static_cast<float>(fmod(totalRotation, XM_2PI));

		Rotate(radians);
	}
}

// Rotate the 3D cube model a set amount of radians.
void vrController::Rotate(float radians){
	// Prepare to pass the updated model matrix to the shader
	XMStoreFloat4x4(&m_all_buff_Data.model, XMMatrixTranspose(XMMatrixRotationY(radians)));
}

void vrController::StartTracking()
{
	m_tracking = true;
}

// When tracking, the 3D cube can be rotated around its Y axis by tracking pointer position relative to the output screen width.
void vrController::TrackingUpdate(float positionX)
{
	if (m_tracking){
		float radians = XM_2PI * 2.0f * positionX / m_deviceResources->GetOutputSize().Width;
		Rotate(radians);
	}
}

void vrController::StopTracking(){
	m_tracking = false;
}

// Renders one frame using the vertex and pixel shaders.
void vrController::Render() {
	if (tex_volume == nullptr || tex_baked == nullptr) return;
	if (!pre_draw_) { render_scene(); return; }

	auto context = m_deviceResources->GetD3DDeviceContext();
	if (isDirty()) {
		screen_quad->SetToDrawTarget(context, m_deviceResources->GetDepthStencilView());
		render_scene();
	}
	m_deviceResources->SetBackBufferRenderTarget();
	screen_quad->Draw(context);	
	m_deviceResources->ClearCurrentDepthBuffer();
}

void vrController::precompute() {
	if (!Manager::baked_dirty_) return;
	if (!bakeShader_) CreateDeviceDependentResources();

	auto context = m_deviceResources->GetD3DDeviceContext();
	//run compute shader
	context->CSSetShader(bakeShader_, nullptr, 0);
	ID3D11ShaderResourceView* texview = tex_volume->GetTextureView();
	context->CSSetShaderResources(0, 1, &texview);
	context->CSSetUnorderedAccessViews(0, 1, &m_textureUAV, nullptr);

	if (m_compute_constbuff != nullptr) {
		// Prepare the constant buffer to send it to the graphics device.
		context->UpdateSubresource(
			m_compute_constbuff,
			0,
			nullptr,
			m_manager->getVolumeSetupConstData(),
			0,
			0
		);
		context->CSSetConstantBuffers(0, 1, &m_compute_constbuff);
	}

	context->Dispatch((vol_dimension_.x+7) / 8, (vol_dimension_.y+7) / 8, (vol_dimension_.z + 7) / 8);
	context->CopyResource(tex_baked->GetTexture3D(), m_comp_tex_d3d);
	//unbind UAV
	ID3D11UnorderedAccessView* nullUAV[] = { NULL };
	context->CSSetUnorderedAccessViews(0, 1, nullUAV, 0);
	// Disable Compute Shader
	context->CSSetShader(nullptr, nullptr, 0);

	data_board_->Update(m_deviceResources->GetD3DDevice(), context);
	Manager::baked_dirty_ = false;
}

void vrController::render_scene(){
	if (volume_model_dirty) { updateVolumeModelMat(); volume_model_dirty = false; }
	
	auto context = m_deviceResources->GetD3DDeviceContext();
	//front or back
	DirectX::XMFLOAT4X4 m_rot_mat;
	XMStoreFloat4x4(&m_rot_mat, mat42xmmatrix(RotateMat_));
	bool is_front = (m_rot_mat._33 * Manager::camera->getViewDirection().z) < 0;
	context->RSSetState(is_front? m_render_state_front: m_render_state_back);

	auto model_mat = ModelMat_ * vol_dim_scale_mat_;
	
	if (Manager::IsCuttingNeedUpdate()) cutter_->Update(model_mat); 

	bool render_complete = true;
	//////  CUTTING PLANE  //////
	if (Manager::param_bool[dvr::CHECK_CUTTING]){
		render_complete &= cutter_->Draw(m_deviceResources->GetD3DDeviceContext());
		m_deviceResources->ClearCurrentDepthBuffer();
	}

	//////   VOLUME   //////
	if (m_manager->isDrawVolume()) {
		precompute();
		if (Manager::isRayCasting()) render_complete &= raycast_renderer->Draw(context, tex_baked, mat42xmmatrix(model_mat));
		else render_complete &= texvrRenderer_->Draw(context, tex_baked, mat42xmmatrix(ModelMat_), is_front);
		m_deviceResources->ClearCurrentDepthBuffer();
	}

	///// MESH  ////
	if (m_manager->isDrawMesh()) {
		render_complete &= meshRenderer_->Draw(m_deviceResources->GetD3DDeviceContext(), tex_volume, mat42xmmatrix(model_mat));
	}

	///// CENTER LINE/////
	if (m_manager->isDrawCenterLine()) {
		auto mask_bits_ = m_manager->getMaskBits();
		for (auto line : line_renderers_)
			if ((mask_bits_ >> (line.first + 1)) & 1) 
				render_complete &= line.second->Draw(context, mat42xmmatrix(model_mat));
		m_deviceResources->ClearCurrentDepthBuffer();
	}

	///// CENTERLINE TRAVERSAL PLANE////
	if (Manager::param_bool[dvr::CHECK_CENTER_LINE_TRAVEL]) {
		render_complete &= cutter_->Draw(m_deviceResources->GetD3DDeviceContext(), is_front);
		m_deviceResources->ClearCurrentDepthBuffer();
	}

	///// OVERLAY DATA BOARD/////
	if (Manager::param_bool[dvr::CHECK_OVERLAY]) {
		render_complete &= data_board_->Draw(m_deviceResources->GetD3DDeviceContext(),
			DirectX::XMMatrixScaling(1.0, 0.3, 0.1)
			* DirectX::XMMatrixTranslation(0.8f, 0.8f, .0f),
			is_front
		);
		m_deviceResources->ClearCurrentDepthBuffer();
	}
	Manager::baked_dirty_ = false;
	m_scene_dirty = !render_complete;
	context->RSSetState(m_render_state_front);
}

void vrController::CreateDeviceDependentResources(){
	if (bakeShader_) return;
	auto loadCSTask = DX::ReadDataAsync(L"VolumeCompute.cso");
	// After the vertex shader file is loaded, create the shader and input layout.
	auto createCSTask = loadCSTask.then([this](const std::vector<byte>& fileData) {
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateComputeShader(
				&fileData[0],
				fileData.size(),
				nullptr,
				&bakeShader_
			)
		);

		CD3D11_BUFFER_DESC constantBufferDesc(sizeof(XMFLOAT4X4) *12, D3D11_BIND_CONSTANT_BUFFER);
		winrt::check_hresult(
			m_deviceResources->GetD3DDevice()->CreateBuffer(
				&constantBufferDesc,
				nullptr,
				&m_compute_constbuff
			)
		);
	});
	D3D11_RASTERIZER_DESC rasterDesc;

	rasterDesc.AntialiasedLineEnable = false;
	rasterDesc.CullMode = D3D11_CULL_BACK;
	rasterDesc.DepthBias = 0;
	rasterDesc.DepthBiasClamp = 0.0f;
	rasterDesc.DepthClipEnable = false;
	rasterDesc.FillMode = D3D11_FILL_SOLID;
	rasterDesc.FrontCounterClockwise = false;
	rasterDesc.MultisampleEnable = false;
	rasterDesc.ScissorEnable = false;
	rasterDesc.SlopeScaledDepthBias = 0.0f;
	winrt::check_hresult(
		m_deviceResources->GetD3DDevice()->CreateRasterizerState(&rasterDesc, &m_render_state_front)
	);
	rasterDesc.FrontCounterClockwise = true;
	winrt::check_hresult(
		m_deviceResources->GetD3DDevice()->CreateRasterizerState(&rasterDesc, &m_render_state_back)
	);
}
void vrController::onSingleTouchDown(float x, float y) {
	Mouse_old = { x, y };
	m_IsPressed = true;
}
void vrController::onTouchMove(float x, float y) {
	if (!m_IsPressed || !tex_volume) return;

	if (!Manager::param_bool[dvr::CHECK_CUTTING] && Manager::param_bool[dvr::CHECK_FREEZE_VOLUME]) return;

	float xoffset = x - Mouse_old.x, yoffset = Mouse_old.y - y;
	Mouse_old = { x, y };
	xoffset *= dvr::MOUSE_ROTATE_SENSITIVITY;
	yoffset *= -dvr::MOUSE_ROTATE_SENSITIVITY;

	if (Manager::param_bool[dvr::CHECK_FREEZE_VOLUME]) {
		cutter_->onRotate(xoffset, yoffset);
		m_scene_dirty = true;
		return;
	}
	RotateMat_ = mouseRotateMat(RotateMat_, xoffset, yoffset);
	volume_model_dirty = true;
}
void vrController::onTouchReleased(){
	m_IsPressed = false;
}
void vrController::onScale(float sx, float sy) {
	if (!tex_volume) return;
	//unified scaling
	if (sx > 1.0f) sx = 1.0f + (sx - 1.0f) * MOUSE_SCALE_SENSITIVITY;
	else sx = 1.0f - (1.0f - sx) * MOUSE_SCALE_SENSITIVITY;

	if (Manager::param_bool[dvr::CHECK_FREEZE_VOLUME]) {
		cutter_->onScale(sx);
	}else {
		ScaleVec3_ = ScaleVec3_ * sx;
		volume_model_dirty = true;
	}
}
void vrController::onPan(float x, float y) {
	if (!tex_volume || Manager::param_bool[dvr::CHECK_FREEZE_VOLUME]) return;

	float offx = x / Manager::screen_w * MOUSE_PAN_SENSITIVITY, offy = -y / Manager::screen_h * MOUSE_PAN_SENSITIVITY;
	PosVec3_.x += offx * ScaleVec3_.x;
	PosVec3_.y += offy * ScaleVec3_.y;
	volume_model_dirty = true;
}
void vrController::updateVolumeModelMat() {
	ModelMat_ = glm::translate(glm::mat4(1.0), PosVec3_)
		* RotateMat_
		* glm::scale(glm::mat4(1.0), ScaleVec3_);
}

bool vrController::addStatus(std::string name, glm::mat4 mm, glm::mat4 rm, glm::vec3 sv, glm::vec3 pv, Camera* cam) {
	auto it = rStates_.find(name);
	if (it != rStates_.end()) return false;

	rStates_[name] = reservedStatus(mm, rm, sv, pv, cam);
	if (Manager::screen_w != 0) rStates_[name].vcam->setProjMat(Manager::screen_w, Manager::screen_h);
	return true;
}
bool vrController::addStatus(std::string name, bool use_current_status) {
	auto it = rStates_.find(name);
	if (it != rStates_.end()) return false;

	if (use_current_status) {
		if (volume_model_dirty) {
			updateVolumeModelMat();
			volume_model_dirty = false;
		}
		rStates_[name] = reservedStatus(ModelMat_, RotateMat_, ScaleVec3_, PosVec3_, new Camera(name.c_str()));
	}
	else rStates_[name] = reservedStatus();
	if (Manager::screen_w != 0)rStates_[name].vcam->setProjMat(Manager::screen_w, Manager::screen_h);
	return true;
}
void vrController::setMVPStatus(std::string name) {
	if (name == cst_name) return;
	auto rstate_ = rStates_[name];
	ModelMat_ = rstate_.model_mat; RotateMat_ = rstate_.rot_mat; ScaleVec3_ = rstate_.scale_vec; PosVec3_ = rstate_.pos_vec; Manager::camera = rstate_.vcam;
	volume_model_dirty = false;
	cst_name = name;
}
void vrController::setupCenterLine(int id, float* data) {
	int oid = 0;
	while (id /= 2)oid++;
	if (line_renderers_.count(oid))
		line_renderers_[oid]->updateVertices(m_deviceResources->GetD3DDevice(), 4000, data);
	else
		line_renderers_[oid] = new lineRenderer(m_deviceResources->GetD3DDevice(), oid, 4000, data);
	cutter_->setupCenterLine((dvr::ORGAN_IDS)oid, data);
}
void vrController::setCuttingPlane(float value) {
	if (Manager::param_bool[dvr::CHECK_CUTTING]) {
		if (!Manager::isRayCasting()) {
			cutter_->setCutPlane(value);
			texvrRenderer_->setCuttingPlane(value);
			m_scene_dirty = true;
		}
	}else if (Manager::param_bool[dvr::CHECK_CENTER_LINE_TRAVEL]) {
		if (!cutter_->IsCenterLineAvailable()) return;
		cutter_->setCenterLinePos((int)(value * 4000.0f));
		if (Manager::param_bool[dvr::CHECK_TRAVERSAL_VIEW]) AlignModelMatToTraversalPlane();
		m_scene_dirty = true;
	}
}
void vrController::setCuttingPlane(int id, int delta) {
	if (Manager::param_bool[dvr::CHECK_CUTTING]) {
		cutter_->setCuttingPlaneDelta(delta);
		if (!Manager::isRayCasting()) texvrRenderer_->setCuttingPlaneDelta(delta);
		m_scene_dirty = true;
	}
	else if (Manager::param_bool[dvr::CHECK_CENTER_LINE_TRAVEL]) {
		if (!cutter_->IsCenterLineAvailable())return;
		cutter_->setCenterLinePos(id, delta);
		if (Manager::param_bool[dvr::CHECK_TRAVERSAL_VIEW]) AlignModelMatToTraversalPlane();
		m_scene_dirty = true;
	}
}
void vrController::setCuttingPlane(glm::vec3 pp, glm::vec3 pn) {
	cutter_->setCutPlane(pp, pn);
	m_scene_dirty = true;
}
void vrController::switchCuttingPlane(dvr::PARAM_CUT_ID cut_plane_id) {
	cutter_->SwitchCuttingPlane(cut_plane_id);
	if (Manager::param_bool[dvr::CHECK_TRAVERSAL_VIEW]) AlignModelMatToTraversalPlane();
	m_scene_dirty = true;
}
void vrController::ReleaseDeviceDependentResources(){
	raycast_renderer->Clear();
	//screen_quad->Clear();
	//texture
	if (tex_volume) delete tex_volume;
	if (tex_baked) delete tex_baked;
	rStates_.clear();
}
bool vrController::isDirty() {
	if (!tex_volume) return false;
	return (Manager::baked_dirty_ || volume_model_dirty || m_scene_dirty);
}
void vrController::AlignModelMatToTraversalPlane() {
	glm::vec3 pp, pn;
	cutter_->getCurrentTraversalInfo(pp, pn);
	pn = glm::normalize(pn);

	RotateMat_ = glm::toMat4(glm::rotation(pn, glm::vec3(.0, .0, -1.0f)));

	volume_model_dirty = true;
}