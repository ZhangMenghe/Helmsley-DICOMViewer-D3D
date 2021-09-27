#include "pch.h"
#include "SlateCameraRenderer.h"
#include <Common/DirectXHelper.h>
#include <D3DPipeline/Primitive.h>
#include <Common/Manager.h>
#include <glm/gtx/transform.hpp>
#include <Utils/TypeConvertUtils.h>
#include <Utils/MathUtils.h>
#include <Utils/CVMathUtils.h>


#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>  // cv::Canny()
#include <opencv2/aruco.hpp>
#include <opencv2/core/mat.hpp>
#include <opencv2/calib3d.hpp>
#include <glm/gtx/rotate_vector.hpp> 
#include <vrController.h>
#include <glm/gtx/component_wise.hpp>

#include <winrt/Windows.Perception.Spatial.h>
#include <winrt/Windows.Perception.Spatial.Preview.h>
using namespace winrt::Windows::Perception;
using namespace winrt::Windows::Perception::Spatial;
using namespace winrt::Windows::Perception::Spatial::Preview;
using namespace DirectX;

#include "strsafe.h"

void SlateCameraRenderer::CameraUpdateThread(SlateCameraRenderer* pSlateCameraRenderer, HANDLE hasData, ResearchModeSensorConsent* pCamAccessConsent){
	HRESULT hr = S_OK;
	LARGE_INTEGER qpf;
	uint64_t lastQpcNow = 0;

	QueryPerformanceFrequency(&qpf);

	if (hasData != nullptr)
	{
		DWORD waitResult = WaitForSingleObject(hasData, INFINITE);

		if (waitResult == WAIT_OBJECT_0)
		{
			switch (*pCamAccessConsent)
			{
			case ResearchModeSensorConsent::Allowed:
				OutputDebugString(L"Access is granted");
				break;
			case ResearchModeSensorConsent::DeniedBySystem:
				OutputDebugString(L"Access is denied by the system");
				hr = E_ACCESSDENIED;
				break;
			case ResearchModeSensorConsent::DeniedByUser:
				OutputDebugString(L"Access is denied by the user");
				hr = E_ACCESSDENIED;
				break;
			case ResearchModeSensorConsent::NotDeclaredByApp:
				OutputDebugString(L"Capability is not declared in the app manifest");
				hr = E_ACCESSDENIED;
				break;
			case ResearchModeSensorConsent::UserPromptRequired:
				OutputDebugString(L"Capability user prompt required");
				hr = E_ACCESSDENIED;
				break;
			default:
				OutputDebugString(L"Access is denied by the system");
				hr = E_ACCESSDENIED;
				break;
			}
		}
		else
		{
			hr = E_UNEXPECTED;
		}
	}

	if (FAILED(hr))
	{
		return;
	}

	winrt::check_hresult(pSlateCameraRenderer->m_pRMCameraSensor->OpenStream());

	while (!pSlateCameraRenderer->m_fExit && pSlateCameraRenderer->m_pRMCameraSensor)
	{
		static int gFrameCount = 0;
		HRESULT hr = S_OK;
		IResearchModeSensorFrame* pSensorFrame = nullptr;
		LARGE_INTEGER qpcNow;
		uint64_t uqpcNow;
		QueryPerformanceCounter(&qpcNow);
		uqpcNow = qpcNow.QuadPart;
		ResearchModeSensorTimestamp timeStamp;

		winrt::check_hresult(pSlateCameraRenderer->m_pRMCameraSensor->GetNextBuffer(&pSensorFrame));

		pSensorFrame->GetTimeStamp(&timeStamp);

		{
			if (lastQpcNow != 0)
			{
				pSlateCameraRenderer->m_refreshTimeInMilliseconds =
					(1000 *
						(uqpcNow - lastQpcNow)) /
					qpf.QuadPart;
			}

			if (pSlateCameraRenderer->m_lastHostTicks != 0)
			{
				pSlateCameraRenderer->m_sensorRefreshTime = timeStamp.HostTicks - pSlateCameraRenderer->m_lastHostTicks;
			}

			std::lock_guard<std::mutex> guard(pSlateCameraRenderer->m_mutex);

			if (pSlateCameraRenderer->m_frameCallback)
			{
				pSlateCameraRenderer->m_frameCallback(pSensorFrame, pSlateCameraRenderer->m_frameCtx);
			}

			if (pSlateCameraRenderer->m_pSensorFrame)
			{
				pSlateCameraRenderer->m_pSensorFrame->Release();
			}

			pSlateCameraRenderer->m_pSensorFrame = pSensorFrame;
			lastQpcNow = uqpcNow;
			pSlateCameraRenderer->m_lastHostTicks = timeStamp.HostTicks;
		}
	}

	if (pSlateCameraRenderer->m_pRMCameraSensor)
	{
		pSlateCameraRenderer->m_pRMCameraSensor->CloseStream();
	}
}

SlateCameraRenderer::SlateCameraRenderer(ID3D11Device* device)
:baseRenderer(device, L"QuadVertexShader.cso", L"NaiveColorPixelShader.cso",
	quad_vertices_pos_w_tex, quad_indices, 24, 6),
	m_input_layout_id(dvr::INPUT_POS_TEX_2D){
	initialize();
	vrController::instance()->setUseSpaceMat(true);
}
SlateCameraRenderer::SlateCameraRenderer(ID3D11Device* device,
	IResearchModeSensor* pLLSensor, HANDLE hasData, ResearchModeSensorConsent* pCamAccessConsent)
:SlateCameraRenderer(device){
	m_pRMCameraSensor = pLLSensor;
	m_pRMCameraSensor->AddRef();
	m_pSensorFrame = nullptr;
	m_frameCallback = nullptr;

	m_pCameraUpdateThread = new std::thread(CameraUpdateThread, this, hasData, pCamAccessConsent);

}

void SlateCameraRenderer::setPosition(glm::vec3 pos) {
	m_quad_matrix = glm::translate(glm::mat4(1.0), pos) * m_quad_matrix;
}
bool SlateCameraRenderer::GetFirstTransformation(cv::Vec3d& rvec, cv::Vec3d& tvec) {
	//std::lock_guard<std::mutex> guard(m_cornerMutex);
	if (m_rvecs.size() > 0) {
		rvec = m_rvecs[0]; tvec = m_tvecs[0];
		return true;
	}
	return false;
}
bool SlateCameraRenderer::update_cam_texture(ID3D11DeviceContext* context) {
	if (m_pSensorFrame == nullptr) return false;
	if (texture == nullptr) {

		ResearchModeSensorResolution resolution;//640*480
		m_pSensorFrame->GetResolution(&resolution);

		if (m_pRMCameraSensor->GetSensorType() == DEPTH_LONG_THROW)
		{
			m_slateWidth = resolution.Width * 2;
		}
		else
		{
			m_slateWidth = resolution.Width;
		}
		m_slateHeight = resolution.Height;
		if (m_pRMCameraSensor->GetSensorType() == LEFT_FRONT) {
			m_quad_matrix = m_quad_matrix
				* glm::scale(glm::mat4(1.0), glm::vec3( m_slateWidth/ m_slateHeight, 1.0, 1.0));
				//* glm::rotate(glm::mat4(1.0), -glm::half_pi<float>(), glm::vec3(.0, .0, 1.0));
		}
		else {
			m_quad_matrix = m_quad_matrix
				* glm::scale(glm::mat4(1.0), glm::vec3(m_slateHeight / m_slateWidth, 1.0, 1.0))
				* glm::rotate(glm::mat4(1.0), glm::half_pi<float>(), glm::vec3(.0, .0, 1.0));
		}

		D3D11_TEXTURE2D_DESC texDesc;
		texDesc.Width = m_slateWidth;
		texDesc.Height = m_slateHeight;
		texDesc.Format = DXGI_FORMAT_R8_UNORM;
		texDesc.Usage = D3D11_USAGE_DEFAULT;
		texDesc.MipLevels = 1;
		texDesc.ArraySize = 1;
		texDesc.SampleDesc.Count = 1;
		texDesc.SampleDesc.Quality = 0;
		texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
		texDesc.CPUAccessFlags = 0;
		texDesc.MiscFlags = D3D11_RESOURCE_MISC_SHARED;

		D3D11_RENDER_TARGET_VIEW_DESC view_desc{
		texDesc.Format,
		D3D11_RTV_DIMENSION_TEXTURE2D,
		};
		view_desc.Texture2D.MipSlice = 0;
		texture = new Texture;
		texture->Initialize(device, texDesc, view_desc);

		m_tex_size = m_slateWidth * m_slateHeight;

		//setup camera matrix
		//float camera_data[9] = {
		//	375.11117222,   0.,         325.94678612,
		//	0.,         376.667932,   229.69762885 -left,
		//	0.,           0.,           1.
		//};

		//	0,											376.667932.,        229.69762885,
		//	-375.11117222.,         0,									-325.94678612,
		//	0.,           0.,           1.
		//m_cameraMatrix = (cv::Mat1d(3, 3) << 370.06088626, 0, 306.86274468, 0, 374.89346096, 238.62886492, 0, 0, 1); // m
		m_cameraMatrix = (cv::Mat1d(3, 3) << 370.06088626, 0, 322.86274468, 0, 374.89346096, 232.52886492, 0, 0, 1);
		
		//m_cameraMatrix = (cv::Mat1d(3, 3) << 374.74548357, 0, 248.08241952, 0, 370.32639999, 321.97007822, 0, 0, 1);
		
		//m_cameraMatrix = cv::Mat(3, 3, CV_32F, camera_data);
		//float distor_data[5] = { -0.04310492,  0.22544408, -0.00800435,  0.00223716, -0.25756141 };
		//m_distCoeffs = cv::Mat(1, 5, CV_32F, distor_data);
		m_distCoeffs = (cv::Mat1d(1, 5) << -0.04411806, 0.13393567, -0.00460908, 0.00304738, -0.09357143);
		//m_distCoeffs = (cv::Mat1d(1, 5) << -0.04792172, 0.16861088, 0.00267955, 0.00622755, -0.163583);
	}

	ResearchModeSensorResolution resolution;
	IResearchModeSensorVLCFrame* pVLCFrame = nullptr;
	//const BYTE* pImage = nullptr;
	if (m_texture_data != nullptr) {m_texture_data = nullptr;}
	
	size_t outBufferCount = 0;
	m_pSensorFrame->GetResolution(&resolution);

	//DX::ThrowIfFailed();
	if (FAILED(m_pSensorFrame->QueryInterface(IID_PPV_ARGS(&pVLCFrame)))) return false;
	pVLCFrame->GetBuffer(&m_texture_data, &outBufferCount);

	ResearchModeSensorTimestamp timeStamp;
	m_pSensorFrame->GetTimeStamp(&timeStamp);
	HRESULT hr = S_OK;
	
	/*TCHAR buf[1024];
	size_t cbDest = 1024 * sizeof(TCHAR);
	StringCbPrintf(buf, cbDest, TEXT("GUID:(%d)\n"), (long)m_guid.Data1);
	OutputDebugString(buf);*/

	//m_frameOfReference = locator.CreateAttachedFrameOfReferenceAtCurrentHeading();

	auto timestamp = PerceptionTimestampHelper::FromSystemRelativeTargetTime(winrt::Windows::Foundation::TimeSpan{ timeStamp.HostTicks });
	//auto coordinateSystem = m_referenceFrame;//m_referenceFrame.CoordinateSystem();//GetStationaryCoordinateSystemAtTimestamp(timestamp);
	auto location = locator.TryLocateAtTimestamp(timestamp, m_referenceFrame);
	if (location) {
		//auto magic = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0, 0, 1));
		//auto magic2 = glm::translate(glm::mat4(1.0f), glm::vec3(1, 2, 3));
		auto rotMatrix = make_float4x4_from_quaternion(location.Orientation());
		auto transMatrix = make_float4x4_translation(location.Position());
	  /*
		XMMATRIX rot = XMLoadFloat4x4(&rotMatrix);
		XMMATRIX trans = XMLoadFloat4x4(&transMatrix);
		glm::mat4 rot_glm = xmmatrix2mat4(rot);
		glm::mat4 trans_glm = xmmatrix2mat4(trans);*/
		auto transform4x4 = rotMatrix * transMatrix;
		auto transformMatrix = XMLoadFloat4x4(&transform4x4);
		
		cameraToWorld = xmmatrix2mat4((transformMatrix));//XMMatrixTranspose
		//xmmatrix2mat4(Manager::camera->getViewMat())
		//cameraToWorld = flip * cameraToWorld;
		//cameraToWorld = magic3 * cameraToWorld;
		//cameraToWorld = magic * cameraToWorld;
		//// Change coordinate system
		//auto flip = glm::scale(glm::mat4(1), glm::vec3(1, 1, -1));
		//auto swap = glm::mat4(0.0f);
		//swap[0][1] = -1;
		//swap[1][0] = 1;
		//swap[2][2] = 1;
		//swap[3][3] = 1;

	 // //rot_glm = rot_glm * flip;
		////rot_glm = rot_glm * swap;
		//rot_glm = glm::transpose(rot_glm);
		////trans_glm[3][0] = -trans_glm[3][1];
		////trans_glm[3][1] = -trans_glm[3][0];
		////trans_glm[3][2] = -trans_glm[3][2];
		//auto magic = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0, 0, 1));
		if (dvr::PRINT_CAMERA_MATRIX) {
			TCHAR buf[1024];
			size_t cbDest = 1024 * sizeof(TCHAR);
			StringCbPrintf(buf, cbDest, TEXT("Location:(%f,%f,%f)\n"), (float)cameraToWorld[3][0], (float)cameraToWorld[3][1], (float)cameraToWorld[3][2]);
			OutputDebugString(buf);
		}

		////rot_glm = xmmatrix2mat4(Manager::camera->getViewMat());
		//rot_glm = glm::mat4(glm::mat3(rot_glm));
		//cameraToWorld = rot_glm;//trans_glm * rot_glm;
		////cameraToWorld = magic * flip * cameraToWorld; //
		//cameraToWorld = glm::inverse(cameraToWorld);
	}

	//pVLCFrame->GetGain(&gain);
	//pVLCFrame->GetExposure(&exposure);

	//sprintf(printString, "####CameraGain %d %I64d\n", gain, exposure);
	//OutputDebugStringA(printString);

	auto row_pitch = (m_slateWidth) * sizeof(BYTE);
	texture->setTexData(context, m_texture_data, row_pitch, 0);
	return true;
}
bool SlateCameraRenderer::Update(ID3D11DeviceContext* context) {
	if (!update_cam_texture(context)) return false;
	//UpdateExtrinsicsMatrix();

	if(m_pRMCameraSensor->GetSensorType() != LEFT_FRONT) {
		return true;
	}

	//try estimate marker
	static cv::Ptr<cv::aruco::Dictionary> dictionary = cv::aruco::getPredefinedDictionary(cv::aruco::DICT_6X6_250);

	cv::Mat processed(m_slateHeight, m_slateWidth, CV_8U, (void*)m_texture_data);
	//std::memcpy(processed.data, (void*)m_texture_data, m_slateHeight * m_slateWidth * sizeof(byte));
	//cv::rotate(processed, processed, cv::ROTATE_90_CLOCKWISE);
	std::vector<int> ids;
	std::vector<std::vector<cv::Point2f>> corners;
	cv::aruco::detectMarkers(processed, dictionary, corners, ids);
	if (ids.empty()) return true;

	// if at least one marker detected
	cv::aruco::estimatePoseSingleMarkers(corners, 0.15, m_cameraMatrix, m_distCoeffs, m_rvecs, m_tvecs);
	
	/*cv::Mat R;
	Rodrigues(m_rvecs[0], R);

	glm::mat4 rot_mat(1.0f);

	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 3; j++) {
			rot_mat[i][j] = (float)R.at<double>(i, j);
		}
	}*/
	auto tvec = m_tvecs[0];

	glm::quat q = getQuaternion(m_rvecs[0]);
	//q.y = -q.y; q.z = -q.z;
	glm::mat4 rot_mat = glm::toMat4(q);

	glm::mat4 model_mat =
		glm::translate(glm::mat4(1.0), glm::vec3(tvec[0], tvec[1], tvec[2])) * rot_mat;

	//glm::mat4 cam_inv = glm::transpose(xmmatrix2mat4(Manager::camera->getViewMat()));//vrController::instance()->getCameraExtrinsicsMat(0);

	vrController::instance()->setPosition(cameraToWorld * model_mat); // 
	return true;
}
void SlateCameraRenderer::UpdateExtrinsicsMatrix() {
	IResearchModeCameraSensor* pCameraSensor = nullptr;
	winrt::check_hresult(m_pRMCameraSensor->QueryInterface(IID_PPV_ARGS(&pCameraSensor)));

	DirectX::XMFLOAT4X4 CameraPose;
	winrt::check_hresult(pCameraSensor->GetCameraExtrinsicsMatrix(&CameraPose));

	DirectX::XMMATRIX cameraNodeToRigPoseInverted;
	DirectX::XMMATRIX cameraNodeToRigPose;
	DirectX::XMVECTOR det;

	cameraNodeToRigPose = DirectX::XMLoadFloat4x4(&CameraPose);

	det = XMMatrixDeterminant(cameraNodeToRigPose);
	cameraNodeToRigPoseInverted = DirectX::XMMatrixInverse(&det, cameraNodeToRigPose);

	DirectX::XMVECTOR outScale, outRotQuat, outTrans;
	XMMatrixDecompose(&outScale, &outRotQuat, &outTrans, cameraNodeToRigPoseInverted);
	
	DirectX::XMFLOAT3 tvec;
	DirectX::XMStoreFloat3(&tvec, outTrans);
	
	glm::vec3 ttc = glm::vec3(tvec.y, tvec.x, tvec.z);

	//vrController::instance()->setCameraExtrinsicsMat(m_pRMCameraSensor->GetSensorType() == LEFT_FRONT ? 0 : 1, xmmatrix2mat4(cameraNodeToRigPoseInverted));//glm::translate(glm::mat4(1.0), ttc));
}
// Renders one frame using the vertex and pixel shaders.
bool SlateCameraRenderer::Draw(ID3D11DeviceContext* context, glm::mat4 modelMat){
	modelMat = m_quad_matrix * modelMat;
	if (!m_loadingComplete) return false;
	std::lock_guard<std::mutex> guard(m_mutex);
	Update(context);

	if (m_constantBuffer != nullptr) {
		XMStoreFloat4x4(&m_constantBufferData.uViewProjMat, Manager::camera->getVPMat());
		XMStoreFloat4x4(&m_constantBufferData.model, XMMatrixTranspose(mat42xmmatrix(modelMat)));

		// Prepare the constant buffer to send it to the graphics device.
		context->UpdateSubresource(
			m_constantBuffer.get(),
			0,
			nullptr,
			&m_constantBufferData,
			0,
			0
		);
	}

	baseRenderer::Draw(context);
	return true;
}
void SlateCameraRenderer::SetGUID(GUID guid) {
	m_guid = guid;
	locator = SpatialGraphInteropPreview::CreateLocatorForNode(m_guid);
}
void SlateCameraRenderer::create_vertex_shader(ID3D11Device* device, const std::vector<byte>& fileData) {
	// After the vertex shader file is loaded, create the shader and input layout.
	winrt::check_hresult(
		device->CreateVertexShader(
			&fileData[0],
			fileData.size(),
			nullptr,
			m_vertexShader.put()
		)
	);

	if (m_input_layout_id == dvr::INPUT_POS_TEX_2D) {
		winrt::check_hresult(
			device->CreateInputLayout(
				dvr::g_vinput_pos_tex_desc,
				ARRAYSIZE(dvr::g_vinput_pos_tex_desc),
				&fileData[0],
				fileData.size(),
				m_inputLayout.put()
			)
		);
		m_vertex_stride = sizeof(dvr::VertexPosTex2d);
	}
	else {
		winrt::check_hresult(
			device->CreateInputLayout(
				dvr::g_vinput_pos_3d_desc,
				ARRAYSIZE(dvr::g_vinput_pos_3d_desc),
				&fileData[0],
				fileData.size(),
				m_inputLayout.put()
			)
		);
		m_vertex_stride = sizeof(dvr::VertexPos3d);
	}
	m_vertex_offset = 0;
}
void SlateCameraRenderer::create_fragment_shader(ID3D11Device* device, const std::vector<byte>& fileData) {
	// After the pixel shader file is loaded, create the shader and constant buffer.
	winrt::check_hresult(
		device->CreatePixelShader(
			&fileData[0],
			fileData.size(),
			nullptr,
			m_pixelShader.put()
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
	winrt::check_hresult(device->CreateSamplerState(&samplerDesc, &m_sampleState));

	CD3D11_BUFFER_DESC constantBufferDesc(sizeof(dvr::ModelViewProjectionConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);
	winrt::check_hresult(
		device->CreateBuffer(
			&constantBufferDesc,
			nullptr,
			m_constantBuffer.put()
		)
	);
}