﻿#ifndef VR_CONTROLLER_H
#define VR_CONTROLLER_H
#include "pch.h"
#include <Renderers/raycastVolumeRenderer.h>
#include <Renderers/quadRenderer.h>
#include <Common/DeviceResources.h>
#include <Common/StepTimer.h>
#include <unordered_map>
#include <D3DPipeline/Camera.h>

class reservedStatus {
public:
	DirectX::XMFLOAT4X4 model_mat, rot_mat;
	DirectX::XMFLOAT3 scale_vec, pos_vec;
	Camera* vcam;
	reservedStatus(DirectX::XMMATRIX mm, DirectX::XMMATRIX rm, DirectX::XMFLOAT3 sv, DirectX::XMFLOAT3 pv, Camera* mcam)
		:scale_vec(sv), pos_vec(pv) {
		DirectX::XMStoreFloat4x4(&model_mat, mm);
		DirectX::XMStoreFloat4x4(&rot_mat, rm);
		vcam = mcam;
	}
	reservedStatus()
		:rot_mat(dvr::DEFAULT_ROTATE), scale_vec(dvr::DEFAULT_SCALE), pos_vec(dvr::DEFAULT_POS), vcam(new Camera) {
		DirectX::XMMATRIX mrot = DirectX::XMLoadFloat4x4(&rot_mat);
		DirectX::XMMATRIX mmodel =
			DirectX::XMMatrixScaling(scale_vec.x, scale_vec.y, scale_vec.z)
			* mrot
			* DirectX::XMMatrixTranslation(pos_vec.x, pos_vec.y, pos_vec.z);
		DirectX::XMStoreFloat4x4(&model_mat, mmodel);
	}
};
struct computeConstantBuffer{
	DirectX::XMUINT4 u_tex_size;

	//opacity widget
	//DirectX::XMFLOAT2 u_opacity[60];
	//int u_widget_num;
	//int u_visible_bits;

	//contrast
	DirectX::XMFLOAT4 u_contrast;
	/*float u_contrast_low;
	float u_contrast_high;
	float u_brightness;*/

	//mask
	//UINT u_maskbits;
	//UINT u_organ_num;
	//bool u_mask_color;

	//
	//bool u_flipy;
	//bool u_show_organ;
	//int u_color_scheme;//COLOR_GRAYSCALE COLOR_HSV COLOR_BRIGHT
};
class vrController{
public:
	vrController(const std::shared_ptr<DX::DeviceResources>& deviceResources);
	static vrController* instance();
	void assembleTexture(int update_target, UINT ph, UINT pw, UINT pd, float sh, float sw, float sd, UCHAR* data, int channel_num = 4);

	void onReset();
	void onReset(DirectX::XMFLOAT3 pv, DirectX::XMFLOAT3 sv, DirectX::XMFLOAT4X4 rm, Camera* cam);

	void CreateDeviceDependentResources();
	void CreateWindowSizeDependentResources();
	void ReleaseDeviceDependentResources();
	void Update(DX::StepTimer const& timer);
	void Render();
	void StartTracking();
	void TrackingUpdate(float positionX);
	void StopTracking();
	bool IsTracking() { return m_tracking; }

	//Interaction
	void onSingleTouchDown(float x, float y);
	void onTouchMove(float x, float y);
	void onTouchReleased();
	void onScale(float sx, float sy);
	void onPan(float x, float y);

	//setter
	bool addStatus(std::string name, DirectX::XMMATRIX mm, DirectX::XMMATRIX rm, DirectX::XMFLOAT3 sv, DirectX::XMFLOAT3 pv, Camera* cam);
	bool addStatus(std::string name, bool use_current_status = false);
	void setMVPStatus(std::string status_name);

	//getter
	Texture* getVolumeTex() { return tex_volume; }
	Texture* getBakedTex() { return tex_baked; }
private:
	static vrController* myPtr_;

	quadRenderer* screen_quad;
	raycastVolumeRenderer* raycast_renderer;

	// Cached pointer to device resources.
	std::shared_ptr<DX::DeviceResources> m_deviceResources;

	//TEXTURES
	Texture *tex_volume = nullptr, *tex_baked = nullptr;

	//compute shader
	ID3D11ComputeShader* bakeShader_;
	ID3D11Texture3D* m_comp_tex_d3d = nullptr;
	ID3D11UnorderedAccessView* m_textureUAV;
	ID3D11Buffer* m_compute_constbuff = nullptr;

	computeConstantBuffer m_cmpdata;



	DirectX::XMMATRIX ModelMat_, RotateMat_;
	DirectX::XMFLOAT3 ScaleVec3_, PosVec3_;
	dvr::allConstantBuffer	m_all_buff_Data;
	std::map<std::string, reservedStatus*> rStates_;

	//UI
	bool m_IsPressed = false;
	DirectX::XMFLOAT2 Mouse_old;
	std::string cst_name;

	//volume
	DirectX::XMUINT3 vol_dimension_;
	DirectX::XMFLOAT3 vol_dim_scale_;
	DirectX::XMMATRIX vol_dim_scale_mat_;

	//uint32	m_indexCount;
	//Texture* texture = nullptr;
	
	

	//texture
	/*ID3D11SamplerState* m_sampleState;
		
	Texture* tex2d_srv_from_uav;
	*/
	bool	m_tracking;
	float	m_degreesPerSecond = 1;
	//flags
	bool volume_model_dirty;
	// Variables used with the rendering loop.
	/*bool	m_loadingComplete;
		
		
	bool	m_isholographic;
	bool	m_render_to_texture = false;

	/*const float m_clear_color[4] = {
		0.f,0.f,0.f,0.f
	};*/
	int screen_width, screen_height;
	//XMINT3 vol_dimension_, vol_dim_scale_;

	void Rotate(float radians);
	void render_scene();
	void init_texture();
	void updateVolumeModelMat();
	void precompute();
	void getGraphPoints(float values[], float*& points);
};
#endif