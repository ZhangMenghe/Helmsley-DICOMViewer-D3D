#ifndef VR_CONTROLLER_H
#define VR_CONTROLLER_H
#include <Renderers/raycastVolumeRenderer.h>
#include <Renderers/quadRenderer.h>
#include <Common/DeviceResources.h>
#include <Common/StepTimer.h>
class vrController{
public:
	vrController(const std::shared_ptr<DX::DeviceResources>& deviceResources);
	static vrController* instance();
	void assembleTexture(int update_target, int ph, int pw, int pd, float sh, float sw, float sd, UCHAR* data, int channel_num = 4);

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
	Texture* tex_volume = nullptr, * tex_baked = nullptr;

	////compute shader
	//ID3D11ComputeShader* m_computeShader;

	DirectX::XMMATRIX ModelMat_, RotateMat_;
	DirectX::XMFLOAT3 ScaleVec3_, PosVec3_;
	dvr::allConstantBuffer	m_all_buff_Data;

	//UI
	DirectX::XMFLOAT2 Mouse_old;

	//uint32	m_indexCount;
	//Texture* texture = nullptr;
	
	

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
	//XMINT3 vol_dimension_, vol_dim_scale_;

	void Rotate(float radians);
	void render_scene();
	void init_texture();
};
#endif