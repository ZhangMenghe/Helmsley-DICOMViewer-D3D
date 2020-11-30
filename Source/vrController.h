#ifndef VR_CONTROLLER_H
#define VR_CONTROLLER_H
#include "pch.h"
#include <Renderers/raycastVolumeRenderer.h>
#include <Renderers/quadRenderer.h>
#include <Common/DeviceResources.h>
#include <Common/StepTimer.h>
#include <unordered_map>
#include <D3DPipeline/Camera.h>
#include <Renderers/textureBasedVolumeRenderer.h>
#include <Renderers/screenQuadRenderer.h>
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
	DirectX::XMFLOAT4 u_opacity[30];
	int u_widget_num;
	int u_visible_bits;

	//contrast
	float u_contrast_low;
	float u_contrast_high;
	float u_brightness;

	//mask
	UINT u_maskbits;
	UINT u_organ_num;
	int u_mask_color;

	//
	int u_flipy;
	int u_show_organ;
	UINT u_color_scheme;//COLOR_GRAYSCALE COLOR_HSV COLOR_BRIGHT
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
	bool isDirty();

private:
	static vrController* myPtr_;

	screenQuadRenderer* screen_quad;
	raycastVolumeRenderer* raycast_renderer;
	textureBasedVolumeRenderer* texvrRenderer_;

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

	bool	m_tracking;
	float	m_degreesPerSecond = 1;

	//flags
	bool volume_model_dirty;
	bool pre_draw_ = true;
	int frame_num = 0;
	void Rotate(float radians);
	void render_scene();
	void init_texture();
	void updateVolumeModelMat();
	void precompute();
	void getGraphPoints(float values[], float*& points);
	static bool isRayCasting() {
		return true;
		//return Manager::param_bool[dvr::CHECK_RAYCAST]; 
	}
};
#endif