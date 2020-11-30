﻿#include "pch.h"
#include "vrController.h"
#include <Common/Manager.h>
#include <Common/DirectXHelper.h>
#include <Utils/MathUtils.h>
using namespace DirectX;
using namespace Windows::Foundation;

vrController* vrController::myPtr_ = nullptr;

vrController* vrController::instance() {
	return myPtr_;
}

// Loads vertex and pixel shaders from files and instantiates the cube geometry.
vrController::vrController(const std::shared_ptr<DX::DeviceResources>& deviceResources) :
	m_tracking(false),
	m_deviceResources(deviceResources){
	myPtr_ = this;

	screen_quad = new screenQuadRenderer(m_deviceResources->GetD3DDevice());
	raycast_renderer = new raycastVolumeRenderer(m_deviceResources->GetD3DDevice());
	texvrRenderer_ = new textureBasedVolumeRenderer(m_deviceResources->GetD3DDevice());

	Manager::camera = new Camera;
	CreateDeviceDependentResources();
	CreateWindowSizeDependentResources();
	onReset();
}
void vrController::onReset() {
	SpaceMat_ = xr::math::LoadXrPose(xr::math::Pose::Identity());
	PosVec3_.z = -1.0f;
	Mouse_old = { .0f, .0f };
	rStates_.clear();
	cst_name = "";
	addStatus("default_status");
	setMVPStatus("default_status");
}

void vrController::onReset(DirectX::XMFLOAT3 pv, DirectX::XMFLOAT3 sv, DirectX::XMFLOAT4X4 rm, Camera* cam) {
	Mouse_old = { .0f, .0f };
	rStates_.clear();
	cst_name = "";
	DirectX::XMMATRIX mrot = DirectX::XMLoadFloat4x4(&rm);

	XMMATRIX mmodel =
		DirectX::XMMatrixScaling(sv.x, sv.y, sv.z)
		* mrot
		* DirectX::XMMatrixTranslation(pv.x, pv.y, pv.z);

	addStatus("template", mmodel, mrot, sv, pv, cam);
	setMVPStatus("template");

	volume_model_dirty = false;
}

void vrController::assembleTexture(int update_target, UINT ph, UINT pw, UINT pd, float sh, float sw, float sd, UCHAR* data, int channel_num) {
	if (update_target == 0 || update_target == 2) {
		vol_dimension_ = { ph, pw, pd };
		m_cmpdata.u_tex_size = { ph,pw,pd, (UINT)0 };

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
		vol_dim_scale_mat_ = DirectX::XMMatrixScaling(vol_dim_scale_.x, vol_dim_scale_.y, vol_dim_scale_.z);
		texvrRenderer_->setDimension(m_deviceResources->GetD3DDevice(), vol_dimension_, vol_dim_scale_);
		//cutter_->setDimension(pd, vol_dim_scale_.z);
	}

	if (tex_volume != nullptr) { delete tex_volume; tex_volume = nullptr; }

	tex_volume = new Texture;
	D3D11_TEXTURE3D_DESC texDesc{
		ph,pw,pd,
		1,
		DXGI_FORMAT_R32_UINT,
		D3D11_USAGE_DEFAULT,
		D3D11_BIND_SHADER_RESOURCE,
		0,
		0
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

	Pbr::Resources * pbrResources = new Pbr::Resources(m_deviceResources->GetD3DDevice());

	// Set up a light source (an image-based lighting environment map will also be loaded and contribute to the scene lighting).
	pbrResources->SetLight({ 0.0f, 0.7071067811865475f, 0.7071067811865475f }, Pbr::RGB::White);

	// Read the BRDF Lookup Table used by the PBR system into a DirectX texture.
	std::vector<byte> brdfLutFileData = ReadFileBytes(FindFileInAppFolder(L"brdf_lut.png", { "", L"Pbr_uwp" }));
	winrt::com_ptr<ID3D11ShaderResourceView> brdLutResourceView =
		Pbr::Texture::LoadTextureImage(m_deviceResources->GetD3DDevice(), brdfLutFileData.data(), (uint32_t)brdfLutFileData.size());
	pbrResources->SetBrdfLut(brdLutResourceView.get());

	winrt::com_ptr<ID3D11ShaderResourceView> diffuseTextureView;
	winrt::com_ptr<ID3D11ShaderResourceView> specularTextureView;

	diffuseTextureView = Pbr::Texture::CreateFlatCubeTexture(m_deviceResources->GetD3DDevice(), Pbr::RGBA::White);
	specularTextureView = Pbr::Texture::CreateFlatCubeTexture(m_deviceResources->GetD3DDevice(), Pbr::RGBA::White);

	pbrResources->SetEnvironmentMap(specularTextureView.get(), diffuseTextureView.get());


	context.PbrResources = pbrResources;
	context.DeviceContext = m_deviceResources->GetD3DDeviceContext();

	m_jointMaterial = Pbr::Material::CreateFlat(*context.PbrResources, Pbr::RGBA::White, 0.85f, 0.01f);
	m_meshMaterial = Pbr::Material::CreateFlat(*context.PbrResources, Pbr::RGBA::White, 1, 0);

	auto createJointObjects = [&](HandData& handData) {
		auto jointModel = std::make_shared<Pbr::Model>();
		Pbr::PrimitiveBuilder primitiveBuilder;

		// Create a axis object attached to each joint location
		for (uint32_t k = 0; k < std::size(handData.PbrNodeIndices); k++) {
			handData.PbrNodeIndices[k] = jointModel->AddNode(DirectX::XMMatrixIdentity(), Pbr::RootNodeIndex, "joint");
			primitiveBuilder.AddAxis(1.0f, 0.5f, handData.PbrNodeIndices[k]);
		}

		// Now that the axis have been added for each joint into the primitive builder,
		// it can be baked into the model as a single primitive.
		jointModel->AddPrimitive(Pbr::Primitive(*context.PbrResources, primitiveBuilder, m_jointMaterial));
		//handData.JointModel = AddObject(std::make_shared<PbrModelObject>(std::move(jointModel)));
		//handData.JointModel->SetVisible(false);
	};

	//// For each hand, initialize the joint objects, hand mesh buffers and corresponding spaces.
	//const std::tuple<XrHandEXT, HandData&> hands[] = { {XrHandEXT::XR_HAND_LEFT_EXT, m_leftHandData},
	//																									{XrHandEXT::XR_HAND_RIGHT_EXT, m_rightHandData} };
	//for (const auto& [hand, handData] : hands) {
	//	XrHandTrackerCreateInfoEXT createInfo{ XR_TYPE_HAND_TRACKER_CREATE_INFO_EXT };
	//	createInfo.hand = hand;
	//	createInfo.handJointSet = XR_HAND_JOINT_SET_DEFAULT_EXT;
	//	context.Extensions->xrCreateHandTrackerEXT(
	//		,
	//		&createInfo,
	//		handData.TrackerHandle.get());

	//	createJointObjects(handData);

	//	// Initialize buffers to receive hand mesh indices and vertices
	//	const XrSystemHandTrackingMeshPropertiesMSFT& handMeshSystemProperties = context.System.HandMeshProperties;
	//	handData.IndexBuffer = std::make_unique<uint32_t[]>(handMeshSystemProperties.maxHandMeshIndexCount);
	//	handData.VertexBuffer = std::make_unique<XrHandMeshVertexMSFT[]>(handMeshSystemProperties.maxHandMeshVertexCount);

	//	handData.meshState.indexBuffer.indexCapacityInput = handMeshSystemProperties.maxHandMeshIndexCount;
	//	handData.meshState.indexBuffer.indices = handData.IndexBuffer.get();
	//	handData.meshState.vertexBuffer.vertexCapacityInput = handMeshSystemProperties.maxHandMeshVertexCount;
	//	handData.meshState.vertexBuffer.vertices = handData.VertexBuffer.get();

	//	XrHandMeshSpaceCreateInfoMSFT meshSpaceCreateInfo{ XR_TYPE_HAND_MESH_SPACE_CREATE_INFO_MSFT };
	//	meshSpaceCreateInfo.poseInHandMeshSpace = xr::math::Pose::Identity();
	//	meshSpaceCreateInfo.handPoseType = XR_HAND_POSE_TYPE_TRACKED_MSFT;
	//	context.Extensions->xrCreateHandMeshSpaceMSFT(
	//		*handData.TrackerHandle, &meshSpaceCreateInfo, handData.MeshSpace.get());

	//	meshSpaceCreateInfo.handPoseType = XR_HAND_POSE_TYPE_REFERENCE_OPEN_PALM_MSFT;
	//	context.Extensions->xrCreateHandMeshSpaceMSFT(
	//		*handData.TrackerHandle, &meshSpaceCreateInfo, handData.ReferenceMeshSpace.get());
	//}

	//// Set a clap detector that will toggle the display mode.
	//m_clapDetector = std::make_unique<StateChangeDetector>(
	//	[this](XrTime time) {
	//	const XrHandJointLocationEXT& leftPalmLocation = m_leftHandData.JointLocations[XR_HAND_JOINT_PALM_EXT];
	//	const XrHandJointLocationEXT& rightPalmLocation = m_rightHandData.JointLocations[XR_HAND_JOINT_PALM_EXT];

	//	if (xr::math::Pose::IsPoseValid(leftPalmLocation) && xr::math::Pose::IsPoseValid(rightPalmLocation)) {
	//		const XMVECTOR leftPalmPosition = xr::math::LoadXrVector3(leftPalmLocation.pose.position);
	//		const XMVECTOR rightPalmPosition = xr::math::LoadXrVector3(rightPalmLocation.pose.position);
	//		const float distance = XMVectorGetX(XMVector3Length(XMVectorSubtract(leftPalmPosition, rightPalmPosition)));
	//		return distance - leftPalmLocation.radius - rightPalmLocation.radius < 0.02 /*meter*/;
	//	}

	//	return false;
	//},
	//	[this]() { m_mode = (HandDisplayMode)(((uint32_t)m_mode + 1) % (uint32_t)HandDisplayMode::Count); });

	Manager::baked_dirty_ = true;
}

void vrController::init_texture() {

	//uncomment this to use 3d texture
	/*tex_volume = new Texture;
	D3D11_TEXTURE3D_DESC texDesc{
		400,400,400,
		1,
		DXGI_FORMAT_R8G8B8A8_UNORM,
		D3D11_USAGE_DEFAULT,
		D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE,
		0,
		D3D11_RESOURCE_MISC_SHARED
	};

	auto imageSize = texDesc.Width * texDesc.Height * texDesc.Depth * 4;
	unsigned char* m_targaData = new unsigned char[imageSize];
	for (auto i = 0; i < imageSize; i += 4) {
		m_targaData[i] = 0xff;// (unsigned char)255;
		m_targaData[i + 1] = 0;
		m_targaData[i + 2] = 0;
		m_targaData[i + 3] = (unsigned char)10;
	}
	if (!tex_volume->Initialize(m_deviceResources->GetD3DDevice(), m_deviceResources->GetD3DDeviceContext(), texDesc, m_targaData)) { delete tex_volume; tex_volume = nullptr; }
	*/
	/*if (tex_baked != nullptr) { delete tex_baked; tex_baked = nullptr; }
	tex_baked = new Texture;
	D3D11_TEXTURE2D_DESC texDesc;
	texDesc.Width = screen_width;
	texDesc.Height = screen_height;
	texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	texDesc.MipLevels = 1;
	texDesc.ArraySize = 1;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	texDesc.CPUAccessFlags = 0;
	texDesc.MiscFlags = D3D11_RESOURCE_MISC_SHARED;

	auto imageSize = texDesc.Width * texDesc.Height * 4;
	unsigned char* m_targaData = new unsigned char[imageSize];
	for (int i = 0; i < imageSize; i += 4) {
		m_targaData[i] = (unsigned char)255;
		m_targaData[i + 1] = 0;
		m_targaData[i + 2] = 0;
		m_targaData[i + 3] = (unsigned char)255;
	}
	if (!tex_baked->Initialize(m_deviceResources->GetD3DDevice(),
		m_deviceResources->GetD3DDeviceContext(),
		texDesc, m_targaData)) {
		delete tex_baked;
		tex_baked = nullptr;
	}

	*/
	if (m_comp_tex_d3d != nullptr) { delete m_comp_tex_d3d; m_comp_tex_d3d = nullptr; }
	if (m_textureUAV != nullptr) { delete m_textureUAV; m_textureUAV = nullptr; }

	////Basic random texture which is shader resource and UAV
	//D3D11_TEXTURE2D_DESC texDesc2 = {};
	//texDesc2.Width = screen_width;
	//texDesc2.Height = screen_height;
	//texDesc2.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	//texDesc2.Usage = D3D11_USAGE_DEFAULT;
	//texDesc2.MipLevels = 1;
	//texDesc2.ArraySize = 1;
	//texDesc2.SampleDesc.Count = 1;
	//texDesc2.SampleDesc.Quality = 0;
	//texDesc2.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
	//texDesc2.CPUAccessFlags = 0;
	//texDesc2.MiscFlags = D3D11_RESOURCE_MISC_SHARED;

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
	//screen_width = outputSize.Width; screen_height = outputSize.Height;
	//screen_quad->setQuadSize(m_deviceResources->GetD3DDevice(), m_deviceResources->GetD3DDeviceContext(), outputSize.Width, outputSize.Height);
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

void vrController::Update(XrTime time) {
	XrSpaceLocation origin{ XR_TYPE_SPACE_LOCATION };
	//xrLocateSpace(*space, *app_space, time, &origin);
	//SpaceMat_ = xr::math::LoadXrPose(origin.pose);
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
	if (m_tracking)
	{
		float radians = XM_2PI * 2.0f * positionX / m_deviceResources->GetOutputSize().Width;
		Rotate(radians);
	}
}

void vrController::StopTracking(){
	m_tracking = false;
}

// Renders one frame using the vertex and pixel shaders.
void vrController::Render() {
	if (!tex_volume) return;
	if (!pre_draw_) { render_scene(); return; }

	auto context = m_deviceResources->GetD3DDeviceContext();
	//TODO:WTF..
	if (frame_num<10 || isDirty()) {
		screen_quad->SetToDrawTarget(context, m_deviceResources->GetDepthStencilView());
		render_scene();
		frame_num++;
	}
	m_deviceResources->SetBackBufferRenderTarget();
	screen_quad->Draw(context);	
	m_deviceResources->ClearCurrentDepthBuffer();
}
void vrController::getGraphPoints(float values[], float*& points) {
	DirectX::XMFLOAT2 lb, lm, lt, rb, rm, rt;
	float half_top = values[dvr::TUNE_WIDTHTOP] / 2.0f;
	float half_bottom = std::fmax(values[dvr::TUNE_WIDTHBOTTOM] / 2.0f, half_top);

	float lb_x = values[dvr::TUNE_CENTER] - half_bottom;
	float rb_x = values[dvr::TUNE_CENTER] + half_bottom;

	lb = DirectX::XMFLOAT2(lb_x, .0f);
	rb = DirectX::XMFLOAT2(rb_x, .0f);

	lt = DirectX::XMFLOAT2(values[dvr::TUNE_CENTER] - half_top, values[dvr::TUNE_OVERALL]);
	rt = DirectX::XMFLOAT2(values[dvr::TUNE_CENTER] + half_top, values[dvr::TUNE_OVERALL]);

	float mid_y = values[dvr::TUNE_LOWEST] * values[dvr::TUNE_OVERALL];
	lm = DirectX::XMFLOAT2(lb_x, mid_y);
	rm = DirectX::XMFLOAT2(rb_x, mid_y);

	//if (points) delete points;
	points = new float[12]{
			lb.x, lb.y, lm.x, lm.y, lt.x, lt.y,
			rb.x, rb.y, rm.x, rm.y, rt.x, rt.y
	};
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
		m_deviceResources->GetD3DDeviceContext()->UpdateSubresource(
			m_compute_constbuff,
			0,
			nullptr,
			&m_cmpdata,
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
	Manager::baked_dirty_ = false;
}

void vrController::render_scene(){
	precompute();
	auto context = m_deviceResources->GetD3DDeviceContext();


	if (volume_model_dirty) { updateVolumeModelMat(); volume_model_dirty = false; }

	auto model_mat = vol_dim_scale_mat_ * ModelMat_ * SpaceMat_;
	
	if(isRayCasting())
		raycast_renderer->Draw(m_deviceResources->GetD3DDeviceContext(), tex_baked, model_mat);
	else {
		auto dir = Manager::camera->getViewDirection();
		DirectX::XMFLOAT4X4 m_rot_mat;
		XMStoreFloat4x4(&m_rot_mat, RotateMat_);
		float front_test = m_rot_mat._33 * dir.z;
		texvrRenderer_->Draw(m_deviceResources->GetD3DDeviceContext(), tex_baked, DirectX::XMMatrixTranspose(ModelMat_), front_test < 0);//DirectX::XMMatrixTranspose(ModelMat_)
	}
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

		//m_cmpdata.u_tex_size = { vol_dimension_.x, vol_dimension_.y, vol_dimension_.z, (UINT)0 };

		float opa_values[5] = {
			1.0f,
			.0f,
			2.0f,
			0.0f,
			1.0f
		};
		float* points;
		getGraphPoints(opa_values, points);

		for (int i = 0; i < 3; i++)m_cmpdata.u_opacity[i] = { points[4 * i], points[4 * i + 1] , points[4 * i + 2] , points[4 * i + 3] };
		m_cmpdata.u_widget_num = 1;
		m_cmpdata.u_visible_bits = 1;
		//contrast
		m_cmpdata.u_contrast_low = .0f;
		m_cmpdata.u_contrast_high = .2f;
		m_cmpdata.u_brightness = 0.5f;
		//mask
		m_cmpdata.u_maskbits = 0xffff - 1;//mask_bits_;
		m_cmpdata.u_organ_num = 7;//mask_num
		m_cmpdata.u_mask_color = 1;
		//others
		m_cmpdata.u_flipy = 0;
		m_cmpdata.u_show_organ = 1;
		m_cmpdata.u_color_scheme = 2;

	});

	/*
	// Load shaders asynchronously.
	auto loadVSTask = DX::ReadDataAsync(L"SampleVertexShader.cso");
	auto loadPSTask = DX::ReadDataAsync(L"SamplePixelShader.cso");
	auto loadCSTask = DX::ReadDataAsync(L"NaiveComputeShader.cso");

	// After the vertex shader file is loaded, create the shader and input layout.
	auto createVSTask = loadVSTask.then([this](const std::vector<byte>& fileData) {
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateVertexShader(
				&fileData[0],
				fileData.size(),
				nullptr,
				&m_vertexShader
				)
			);

		static const D3D11_INPUT_ELEMENT_DESC vertexDesc [] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};

		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateInputLayout(
				vertexDesc,
				ARRAYSIZE(vertexDesc),
				&fileData[0],
				fileData.size(),
				&m_inputLayout
				)
			);
	});

	// After the pixel shader file is loaded, create the shader and constant buffer.
	auto createPSTask = loadPSTask.then([this](const std::vector<byte>& fileData) {
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreatePixelShader(
				&fileData[0],
				fileData.size(),
				nullptr,
				&m_pixelShader
				)
			);
		//texture
		D3D11_SAMPLER_DESC samplerDesc;

		// Create a texture sampler state description.
		samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.MipLODBias = 0.0f;
		samplerDesc.MaxAnisotropy = 1;
		samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
		samplerDesc.BorderColor[0] = 0;
		samplerDesc.BorderColor[1] = 0;
		samplerDesc.BorderColor[2] = 0;
		samplerDesc.BorderColor[3] = 0;
		samplerDesc.MinLOD = 0;
		samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

		// Create the texture sampler state.
		DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateSamplerState(&samplerDesc, &m_sampleState));

		CD3D11_BUFFER_DESC constantBufferDesc(sizeof(ModelViewProjectionConstantBuffer) , D3D11_BIND_CONSTANT_BUFFER);
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateBuffer(
				&constantBufferDesc,
				nullptr,
				&m_constantBuffer
				)
			);
	});

	// After the vertex shader file is loaded, create the shader and input layout.
	auto createCSTask = loadCSTask.then([this](const std::vector<byte>& fileData) {
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateComputeShader(
				&fileData[0],
				fileData.size(),
				nullptr,
				&m_computeShader
			)
		);

	});

	// Once both shaders are loaded, create the mesh.
	auto createCubeTask = (createPSTask && createVSTask && createCSTask).then([this] () {
		D3D11_SUBRESOURCE_DATA vertexBufferData = {0};
		vertexBufferData.pSysMem = cube_vertices_pos_w_tex;
		vertexBufferData.SysMemPitch = 0;
		vertexBufferData.SysMemSlicePitch = 0;
		CD3D11_BUFFER_DESC vertexBufferDesc(sizeof(cube_vertices_pos_w_tex), D3D11_BIND_VERTEX_BUFFER);
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateBuffer(
				&vertexBufferDesc,
				&vertexBufferData,
				&m_vertexBuffer
				)
			);

		D3D11_SUBRESOURCE_DATA indexBufferData = {0};
		indexBufferData.pSysMem = cube_indices;
		indexBufferData.SysMemPitch = 0;
		indexBufferData.SysMemSlicePitch = 0;
		CD3D11_BUFFER_DESC indexBufferDesc(sizeof(cube_indices), D3D11_BIND_INDEX_BUFFER);
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateBuffer(
				&indexBufferDesc,
				&indexBufferData,
				&m_indexBuffer
				)
			);
	});

	// Once the cube is loaded, the object is ready to be rendered.
	createCubeTask.then([this] () {
		m_loadingComplete = true;
	});
	*/
}
void vrController::onSingleTouchDown(float x, float y) {
	Mouse_old = { x, y };
	m_IsPressed = true;
}

void vrController::onSingle3DTouchDown(float x, float y, float z, int side) {
	//char debug[256];
	//sprintf(debug, "Touch %d: %f,%f,%f\n", side, x, y, z);
	//OutputDebugStringA(debug);
	if(side == 0) {
		Mouse3D_old_left = { x, y, z };
		m_IsPressed_left = true;
		if(m_IsPressed_right) {
		  // record distance for scaling
			XrVector3f delta = { x = Mouse3D_old_right.x , y - Mouse3D_old_right.y, z - Mouse3D_old_right.z };
			distance_old = xr::math::Length(delta);
			vector_old = delta;

		}
	}else {
		Mouse3D_old_right = { x, y, z };
		m_IsPressed_right = true;
		if (m_IsPressed_left) {
			// record distance for scaling
			XrVector3f delta = { x - Mouse3D_old_left.x , y - Mouse3D_old_left.y, z - Mouse3D_old_left.z };
			distance_old = xr::math::Length(delta);
			vector_old = delta;
		}
	}

}

void vrController::onTouchMove(float x, float y) {
	if (!m_IsPressed || !tex_volume) return;

	//if (!Manager::param_bool[dvr::CHECK_CUTTING] && Manager::param_bool[dvr::CHECK_FREEZE_VOLUME]) return;

	//if (raycastRenderer_)isRayCasting() ? raycastRenderer_->dirtyPrecompute() : texvrRenderer_->dirtyPrecompute();

	float xoffset = x - Mouse_old.x, yoffset = Mouse_old.y - y;
	Mouse_old = { x, y };
	xoffset *= dvr::MOUSE_ROTATE_SENSITIVITY;
	yoffset *= -dvr::MOUSE_ROTATE_SENSITIVITY;

	/*if (Manager::param_bool[dvr::CHECK_FREEZE_VOLUME]) {
		cutter_->onRotate(xoffset, yoffset);
		return;
	}*/

	RotateMat_ = mouseRotateMat(RotateMat_, xoffset, yoffset);
	volume_model_dirty = true;
}

void vrController::on3DTouchMove(float x, float y, float z, int side) {

	if (m_IsPressed_left) {
	  if(side == 0) {
			//char debug[256];
			//sprintf(debug, "Touch %d: %f,%f,%f\n", side, x, y, z);
			//OutputDebugStringA(debug);
	  }
	}
	if (m_IsPressed_right) {
	  if(side == 1) {
			//char debug[256];
			//sprintf(debug, "Touch %d: %f,%f,%f\n", side, x, y, z);
			//OutputDebugStringA(debug);
	  }
	}

  if(!(m_IsPressed_left && m_IsPressed_right)) {
		// not both hand: translate
    if(side == 0) {
      if(m_IsPressed_left) {
				float dx = x - Mouse3D_old_left.x;
				float dy = y - Mouse3D_old_left.y;
				float dz = z - Mouse3D_old_left.z;

				PosVec3_.x += dx * sens;
				PosVec3_.y += dy * sens;
				PosVec3_.z += dz * sens;

				Mouse3D_old_left = { x, y, z };
				volume_model_dirty = true;
      }
    }else {
			if (m_IsPressed_right) {
				float dx = x - Mouse3D_old_right.x;
				float dy = y - Mouse3D_old_right.y;
				float dz = z - Mouse3D_old_right.z;

				PosVec3_.x += dx * sens;
				PosVec3_.y += dy * sens;
				PosVec3_.z += dz * sens;

				Mouse3D_old_right = { x, y, z };
				volume_model_dirty = true;
			}
    }
  }else {
    // both hand: scale
		XrVector3f delta;
		if (side == 0) {
			delta.x = x - Mouse3D_old_right.x;
			delta.y = y - Mouse3D_old_right.y;
			delta.z = z - Mouse3D_old_right.z;

			float distance = xr::math::Length(delta);

			// Scale
			float ratio = distance / distance_old;

			uniScale *= ratio;

			distance_old = distance;
			Mouse3D_old_left = { x, y, z };
			volume_model_dirty = true;
		}
		else {
			delta.x = x - Mouse3D_old_left.x;
			delta.y = y - Mouse3D_old_left.y;
			delta.z = z - Mouse3D_old_left.z;

			float distance = xr::math::Length(delta);

			// Scale
			float ratio = distance / distance_old;

			uniScale *= ratio;

			distance_old = distance;
			Mouse3D_old_right = { x, y, z };
			volume_model_dirty = true;
		}

		XrVector3f curr = xr::math::Normalize(XrVector3f{ delta.x, 0, delta.z });
		XrVector3f old = xr::math::Normalize(XrVector3f{ vector_old.x, 0, vector_old.z });
		XrVector3f axis = xr::math::Normalize(xr::math::Cross(old, curr));
		float angle = std::acos(xr::math::Dot(old, curr));


		if (angle > 0.01f) {
			auto q = xr::math::Quaternion::RotationAxisAngle(axis, angle);
			auto rot = XMMatrixRotationQuaternion(xr::math::LoadXrQuaternion(q));

			RotateMat_ *= rot;
		}
		vector_old = xr::math::Normalize(delta);
  }

}

void vrController::onTouchReleased(){
	m_IsPressed = false;
}

void vrController::on3DTouchReleased(int side) {
	//char debug[256];
	if (side == 0) {
		//sprintf(debug, "Release %d: %f,%f,%f\n", side, Mouse3D_old_left.x, Mouse3D_old_left.y, Mouse3D_old_left.z);
	}else {
		//sprintf(debug, "Release %d: %f,%f,%f\n", side, Mouse3D_old_right.x, Mouse3D_old_right.y, Mouse3D_old_right.z);
	}
	//OutputDebugStringA(debug);
	if (side == 0) {
		m_IsPressed_left = false;
	}
	else {
		m_IsPressed_right = false;
	}
}

void vrController::onScale(float sx, float sy) {

}

void vrController::onScale(float scale) {
  
}

void vrController::onPan(float x, float y) {

}
void vrController::updateVolumeModelMat() {
	ModelMat_ =
		DirectX::XMMatrixScaling(uniScale, uniScale, uniScale)
		* DirectX::XMMatrixScaling(ScaleVec3_.x, ScaleVec3_.y, ScaleVec3_.z)
		* RotateMat_ *
		DirectX::XMMatrixTranslation(PosVec3_.x, PosVec3_.y, PosVec3_.z);
  
}

bool vrController::addStatus(std::string name, DirectX::XMMATRIX mm, DirectX::XMMATRIX rm, DirectX::XMFLOAT3 sv, DirectX::XMFLOAT3 pv, Camera* cam) {
	auto it = rStates_.find(name);
	if (it != rStates_.end()) return false;

	rStates_[name] = new reservedStatus(mm, rm, sv, pv, cam);
	if (Manager::screen_w != 0) rStates_[name]->vcam->setProjMat(Manager::screen_w, Manager::screen_h);
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
		rStates_[name] = new reservedStatus(ModelMat_, RotateMat_, ScaleVec3_, PosVec3_, new Camera(name.c_str()));
		//rStates_[name] = new reservedStatus(ModelMat_, RotateMat_, ScaleVec3_, PosVec3_, Manager::camera());
	}
	else rStates_[name] = new reservedStatus();
	if (Manager::screen_w != 0) {
		rStates_[name]->vcam->setViewMat(Manager::camera->getViewMat());
		//rStates_[name]->vcam->setProjMat(Manager::screen_w, Manager::screen_h);
	}
	return true;
}
void vrController::setMVPStatus(std::string status_name) {
	if (status_name == cst_name) return;
	auto rstate_ = rStates_[status_name];
	ModelMat_ = DirectX::XMLoadFloat4x4(&rstate_->model_mat);
	RotateMat_ = DirectX::XMLoadFloat4x4(&rstate_->rot_mat);
	ScaleVec3_ = rstate_->scale_vec; PosVec3_ = rstate_->pos_vec;
	Manager::camera = rstate_->vcam;
	volume_model_dirty = false;
	cst_name = status_name;
}

void vrController::setSpaces(XrSpace * space, XrSpace * app_space) {
  this->space = space;
	this->app_space = app_space;
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
	if (volume_model_dirty || Manager::baked_dirty_) {
		//meshRenderer_->dirtyPrecompute();
		return true;
	}
	/*if (Manager::IsCuttingNeedUpdate() && cutter_->isPrecomputeDirty()) {
		meshRenderer_->dirtyPrecompute();
		if (isRayCasting())raycastRenderer_->dirtyPrecompute();
		else texvrRenderer_->dirtyPrecompute();
		return true;
	}
	if (Manager::param_bool[dvr::CHECK_VOLUME_ON]) {
		if (isRayCasting()) return raycastRenderer_->isPrecomputeDirty();
		return texvrRenderer_->isPrecomputeDirty();
	}*/
	return false;
}
