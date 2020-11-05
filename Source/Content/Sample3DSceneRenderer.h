﻿#pragma once

#include "..\Common\DeviceResources.h"
#include "ShaderStructures.h"
#include "..\Common\StepTimer.h"
#include <D3DPipeline/Texture.h>
#include "quadRenderer.h"
#include <Renderers/raycastVolumeRenderer.h>
	// This sample renderer instantiates a basic rendering pipeline.
	class Sample3DSceneRenderer
	{
	public:
		Sample3DSceneRenderer(const std::shared_ptr<DX::DeviceResources>& deviceResources, bool is_holographic = false);
		void CreateDeviceDependentResources();
		void CreateWindowSizeDependentResources();
		void ReleaseDeviceDependentResources();
		void Update(DX::StepTimer const& timer);
		void Render();
		void StartTracking();
		void TrackingUpdate(float positionX);
		void StopTracking();
		bool IsTracking() { return m_tracking; }

	private:
		quadRenderer* screen_quad;
		raycastVolumeRenderer* raycast_renderer;

		// Cached pointer to device resources.
		std::shared_ptr<DX::DeviceResources> m_deviceResources;

		//// Direct3D resources for cube geometry.
		//Microsoft::WRL::ComPtr<ID3D11InputLayout>	m_inputLayout;
		//Microsoft::WRL::ComPtr<ID3D11Buffer>		m_vertexBuffer;
		//Microsoft::WRL::ComPtr<ID3D11Buffer>		m_indexBuffer;
		//Microsoft::WRL::ComPtr<ID3D11VertexShader>	m_vertexShader;
		//Microsoft::WRL::ComPtr<ID3D11PixelShader>	m_pixelShader;
		//Microsoft::WRL::ComPtr<ID3D11Buffer>		m_constantBuffer;

		////compute shader
		//ID3D11ComputeShader* m_computeShader;

		allConstantBuffer	m_all_buff_Data;
		//uint32	m_indexCount;
		Texture* texture = nullptr;
		//texture
		/*ID3D11SamplerState* m_sampleState;
		
		Texture* tex2d_srv_from_uav;
		ID3D11Texture2D* m_comp_tex_d3d = nullptr;
		ID3D11UnorderedAccessView* m_textureUAV;*/
		bool	m_tracking;
		float	m_degreesPerSecond = 1;
		// Variables used with the rendering loop.
		/*bool	m_loadingComplete;
		
		
		bool	m_isholographic;
		bool	m_render_to_texture = false;

		/*const float m_clear_color[4] = {
			0.f,0.f,0.f,0.f
		};*/
		int screen_width, screen_height;

		void Rotate(float radians);
		void render_scene();
		void init_texture();
	};