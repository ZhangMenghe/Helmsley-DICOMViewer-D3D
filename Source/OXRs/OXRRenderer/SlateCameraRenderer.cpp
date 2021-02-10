#include "pch.h"
#include "SlateCameraRenderer.h"
#include <Common/DirectXHelper.h>
#include <D3DPipeline/Primitive.h>
#include <Common/Manager.h>
#include <glm/gtx/transform.hpp>
#include <Utils/TypeConvertUtils.h>
using namespace DirectX;


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
	this->initialize();
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


void SlateCameraRenderer::update_cam_texture(ID3D11DeviceContext* context) {
	if (m_pSensorFrame == nullptr) return;
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
		
		m_quad_matrix = glm::scale(glm::mat4(1.0), glm::vec3(m_slateHeight / m_slateWidth, 1.0, 1.0))
			* glm::rotate(glm::mat4(1.0), -glm::half_pi<float>() , glm::vec3(.0,.0,1.0)) ;
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
	}

	ResearchModeSensorResolution resolution;
	IResearchModeSensorVLCFrame* pVLCFrame = nullptr;
	const BYTE* pImage = nullptr;
	size_t outBufferCount = 0;
	m_pSensorFrame->GetResolution(&resolution);

	DX::ThrowIfFailed(m_pSensorFrame->QueryInterface(IID_PPV_ARGS(&pVLCFrame)));
	pVLCFrame->GetBuffer(&pImage, &outBufferCount);

	//pVLCFrame->GetGain(&gain);
	//pVLCFrame->GetExposure(&exposure);

	//sprintf(printString, "####CameraGain %d %I64d\n", gain, exposure);
	//OutputDebugStringA(printString);

	auto row_pitch = (m_slateWidth) * sizeof(BYTE);
	texture->setTexData(context, pImage, row_pitch, 0);
}
// Renders one frame using the vertex and pixel shaders.
bool SlateCameraRenderer::Draw(ID3D11DeviceContext* context, glm::mat4 modelMat){
	modelMat = m_quad_matrix * modelMat;
	if (!m_loadingComplete) return false;
	std::lock_guard<std::mutex> guard(m_mutex);
	update_cam_texture(context);
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
void SlateCameraRenderer::create_vertex_shader(ID3D11Device* device, const std::vector<byte>& fileData) {
	// After the vertex shader file is loaded, create the shader and input layout.
	DX::ThrowIfFailed(
		device->CreateVertexShader(
			&fileData[0],
			fileData.size(),
			nullptr,
			m_vertexShader.put()
		)
	);

	if (m_input_layout_id == dvr::INPUT_POS_TEX_2D) {
		DX::ThrowIfFailed(
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
		DX::ThrowIfFailed(
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
	DX::ThrowIfFailed(
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
	DX::ThrowIfFailed(device->CreateSamplerState(&samplerDesc, &m_sampleState));

	CD3D11_BUFFER_DESC constantBufferDesc(sizeof(dvr::ModelViewProjectionConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);
	DX::ThrowIfFailed(
		device->CreateBuffer(
			&constantBufferDesc,
			nullptr,
			m_constantBuffer.put()
		)
	);
}