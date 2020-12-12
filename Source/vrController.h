#ifndef VR_CONTROLLER_H
#define VR_CONTROLLER_H
#include "pch.h"
#include <Renderers/raycastVolumeRenderer.h>
#include <Renderers/quadRenderer.h>
#include <Common/DeviceResources.h>
#include <Common/Manager.h>
#include <Common/StepTimer.h>
#include <unordered_map>
#include <D3DPipeline/Camera.h>
#include <Renderers/textureBasedVolumeRenderer.h>
#include <Renderers/screenQuadRenderer.h>
#include <SceneObjs/cuttingPlane.h>
#include <glm/gtc/matrix_transform.hpp>
#include <Renderers/organMeshRenderer.h>
#include <Renderers/LineRenderer.h>

struct reservedStatus {
	glm::mat4 model_mat, rot_mat;
	glm::vec3 scale_vec, pos_vec;
	Camera* vcam;
	reservedStatus(glm::mat4 mm, glm::mat4 rm, glm::vec3 sv, glm::vec3 pv, Camera* cam) {
		model_mat = mm; rot_mat = rm; scale_vec = sv; pos_vec = pv; vcam = cam;
	}
	reservedStatus() :rot_mat(dvr::DEFAULT_ROTATE), scale_vec(dvr::DEFAULT_SCALE), pos_vec(dvr::DEFAULT_POS), vcam(new Camera) {
		model_mat = glm::translate(glm::mat4(1.0), pos_vec)
			* rot_mat
			* glm::scale(glm::mat4(1.0), scale_vec);
	}
};

class vrController{
public:
	vrController(const std::shared_ptr<DX::DeviceResources>& deviceResources, const std::shared_ptr<Manager>& manager);
	static vrController* instance();
	void assembleTexture(int update_target, UINT ph, UINT pw, UINT pd, float sh, float sw, float sd, UCHAR* data, int channel_num = 4);

	void onReset();
	void onReset(glm::vec3 pv, glm::vec3 sv, glm::mat4 rm, Camera* cam);

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
	bool addStatus(std::string name, glm::mat4 mm, glm::mat4 rm, glm::vec3 sv, glm::vec3 pv, Camera* cam);
	bool addStatus(std::string name, bool use_current_status = false);
	void setMVPStatus(std::string status_name);
	void setupCenterLine(int id, float* data);
	void setCuttingPlane(float value);
	void setCuttingPlane(int id, int delta);
	void setCuttingPlane(glm::vec3 pp, glm::vec3 pn);
	void switchCuttingPlane(dvr::PARAM_CUT_ID cut_plane_id);

	//getter
	void getCuttingPlane(DirectX::XMFLOAT4& pp, DirectX::XMFLOAT4& pn) { cutter_->getCuttingPlane(pp, pn); }
	Texture* getVolumeTex() { return tex_volume; }
	Texture* getBakedTex() { return tex_baked; }
	bool isDirty();
	ID3D11RasterizerState* m_render_state_front, * m_render_state_back;

private:
	static vrController* myPtr_;

	screenQuadRenderer* screen_quad;
	raycastVolumeRenderer* raycast_renderer;
	textureBasedVolumeRenderer* texvrRenderer_;
	cuttingController* cutter_;
	organMeshRenderer* meshRenderer_;
	std::unordered_map<int, lineRenderer*> line_renderers_;

	// Cached pointer to device resources.
	std::shared_ptr<DX::DeviceResources> m_deviceResources;
	std::shared_ptr<Manager> m_manager;

	//TEXTURES
	Texture *tex_volume = nullptr, *tex_baked = nullptr;

	//compute shader
	ID3D11ComputeShader* bakeShader_;
	ID3D11Texture3D* m_comp_tex_d3d = nullptr;
	ID3D11UnorderedAccessView* m_textureUAV;
	ID3D11Buffer* m_compute_constbuff = nullptr;

	glm::mat4 ModelMat_, RotateMat_;
	glm::vec3 ScaleVec3_, PosVec3_;
	dvr::allConstantBuffer	m_all_buff_Data;
	std::map<std::string, reservedStatus> rStates_;

	//UI
	bool m_IsPressed = false;
	glm::fvec2 Mouse_old;
	std::string cst_name;

	//volume
	glm::vec3 vol_dimension_, vol_dim_scale_;
	glm::mat4 vol_dim_scale_mat_;

	bool	m_tracking;
	float	m_degreesPerSecond = 1;

	//flags
	bool volume_model_dirty, m_scene_dirty;
	bool pre_draw_ = true;
	int frame_num = 0;

	void Rotate(float radians);
	void render_scene();
	void init_texture();
	void updateVolumeModelMat();
	void precompute();
	void AlignModelMatToTraversalPlane();
};
#endif