#ifndef VR_CONTROLLER_H
#define VR_CONTROLLER_H
#include <Renderers/quadRenderer.h>
#include <Common/DeviceResources.h>
#include <Common/Manager.h>
#include <Common/StepTimer.h>
#include <unordered_map>
#include <D3DPipeline/Camera.h>
#include <Renderers/baseDicomRenderer.h>

#include <Renderers/screenQuadRenderer.h>
#include <SceneObjs/cuttingPlane.h>
#include <glm/gtc/matrix_transform.hpp>
#include <Renderers/organMeshRenderer.h>
#include <Renderers/LineRenderer.h>
#include <SceneObjs/dataVisualizer.h>

struct reservedStatus
{
	glm::mat4 model_mat, rot_mat;
	glm::vec3 scale_vec, pos_vec;
	Camera *vcam;
	reservedStatus(glm::mat4 mm, glm::mat4 rm, glm::vec3 sv, glm::vec3 pv, Camera *cam)
	{
		model_mat = mm;
		rot_mat = rm;
		scale_vec = sv;
		pos_vec = pv;
		vcam = cam;
	}
	reservedStatus() : rot_mat(dvr::DEFAULT_ROTATE), scale_vec(dvr::DEFAULT_SCALE), pos_vec(dvr::DEFAULT_POS), vcam(new Camera)
	{
		model_mat = glm::translate(glm::mat4(1.0), pos_vec) * rot_mat * glm::scale(glm::mat4(1.0), scale_vec);
	}
};

class vrController
{
public:
	vrController(const std::shared_ptr<DX::DeviceResources>& deviceResources, const std::shared_ptr<Manager>& manager);
	static vrController* instance();
	void assembleTexture(int update_target, UINT ph, UINT pw, UINT pd, float sh, float sw, float sd, UCHAR* data, int channel_num = 4);

	void onReset();
	void onReset(glm::vec3 pv, glm::vec3 sv, glm::mat4 rm, Camera* cam);
	void InitOXRScene();

	void CreateDeviceDependentResources();
	void CreateWindowSizeDependentResources();
	void ReleaseDeviceDependentResources();
	void Update(DX::StepTimer const& timer);
	//void Update(XrTime time);
	void Render(int view_id);
	void StartTracking();
	void TrackingUpdate(float positionX);
	void StopTracking();
	bool IsTracking() { return m_tracking; }

	//Interaction
	void onSingleTouchDown(float x, float y);
	void onSingle3DTouchDown(float x, float y, float z, int side);
	void onTouchMove(float x, float y);
	void on3DTouchMove(float x, float y, float z, glm::mat4 rot, int side);
	void onTouchReleased();
	void on3DTouchReleased(int side);
	void onScale(float sx, float sy);
	void onScale(float scale);
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
	void setPosition(glm::vec3 pos) { SpaceMat_ = glm::translate(glm::mat4(1.0), pos); m_present = true; }
	void setPosition(glm::mat4 pos) { SpaceMat_ = pos; m_present = true; }
	void setCameraExtrinsicsMat(int id, glm::mat4 ExtrinsicsMat) { m_extrinsics_mats[id] = ExtrinsicsMat; }
	void setUseSpaceMat(bool use) { m_use_space_mat = use; }
	void setMask(UINT num, UINT bits) {
		meshRenderer_->SetMask(num, bits);
	}
	void setRenderingMethod(dvr::RENDER_METHOD method) {
		//if(method == dvr::TEXTURE_BASED)
		//	OutputDebugString(L"======TEXTURE-BASED======\n");
		//else if(method == dvr::VIEW_ALIGN_SLICING)
		//	OutputDebugString(L"======VIEW-ALIGNMED======\n");
		//else if (method == dvr::RAYCASTING)
		//	OutputDebugString(L"======RAYCAST======\n");

		if (m_rmethod_id == method) return;
		m_rmethod_id = method;
		//vRenderer_[m_rmethod_id]->dirtyPrecompute();
	}
	void setRenderingParameters(dvr::RENDER_METHOD method, float* values) {
		//OutputDebugString(L"======RAYCAST======\n");
		//TCHAR buf[1024];
		//size_t cbDest = 1024 * sizeof(TCHAR);
		//StringCbPrintf(buf, cbDest, TEXT("Render Method: %d, value %f\n"), (int)method, values[0]);
		//OutputDebugString(buf);

		if (method < vRenderer_.size())
			vRenderer_[method]->setRenderingParameters(values);
		else {
			Manager::indiv_rendering_params[method] = values[0];
		}
	}
	//getter
	void getCuttingPlane(DirectX::XMFLOAT4 &pp, DirectX::XMFLOAT4 &pn) { cutter_->getCuttingPlane(pp, pn); }
	Texture *getVolumeTex() { return tex_volume; }
	Texture *getBakedTex() { return tex_baked; }
	bool isDirty();
	//glm::mat4 getFrameModelMat() { return Frame_model_mat; }
	ID3D11RasterizerState *m_render_state_front, *m_render_state_back;
	glm::mat4 getCameraExtrinsicsMat(int id) {
		return m_extrinsics_mats[id];
	}
	void getRPS(glm::vec3& pos, glm::vec3& scale) {
		pos = PosVec3_; scale = ScaleVec3_;
	}

private:
	static vrController *myPtr_;

	std::vector<baseDicomRenderer*> vRenderer_;
	int m_rmethod_id = -1;

	screenQuadRenderer* screen_quad;
	cuttingController* cutter_;
	dataBoard* data_board_;
	organMeshRenderer* meshRenderer_;
	std::unordered_map<int, lineRenderer*> line_renderers_;

	// Cached pointer to device resources.
	std::shared_ptr<DX::DeviceResources> m_deviceResources;
	std::shared_ptr<Manager> m_manager;

	//TEXTURES
	Texture *tex_volume = nullptr, *tex_baked = nullptr;

	//compute shader
	ID3D11ComputeShader *bakeShader_;
	ID3D11Texture3D *m_comp_tex_d3d = nullptr;
	ID3D11UnorderedAccessView *m_textureUAV;
	ID3D11Buffer *m_compute_constbuff = nullptr;

	//glm::mat4 Frame_model_mat;
	glm::mat4 SpaceMat_;
	glm::mat4 ModelMat_, RotateMat_;
	glm::vec3 ScaleVec3_, PosVec3_;
	dvr::allConstantBuffer m_all_buff_Data;
	std::map<std::string, reservedStatus> rStates_;
	glm::mat4 m_extrinsics_mats[2] = { glm::mat4(1.0) };
	//UI
	bool m_IsPressed = false;
	//DirectX::XMFLOAT2 Mouse_old;

	bool m_IsPressed_left = false;
	bool m_IsPressed_right = false;
	glm::vec3 Mouse3D_old_left;
	glm::vec3 Mouse3D_old_right;

	glm::vec3 Mouse3D_old_mid;

	float sens = 1.0f; // 0.1f;

	glm::vec3 vector_old;
	float distance_old = 0;
	float uniScale = 1.0f;

	glm::fvec2 Mouse_old;
	std::string cst_name;

	//volume
	glm::vec3 vol_dimension_, vol_dim_scale_;
	glm::mat4 vol_dim_scale_mat_;

	//uint32	m_indexCount;
	//Texture* texture = nullptr;

	//texture
	/*ID3D11SamplerState* m_sampleState;
		
	Texture* tex2d_srv_from_uav;
	*/
	bool m_tracking;
	float m_degreesPerSecond = 1;

	bool m_present = false;
	//flags
	int frame_num = 0;
	bool volume_model_dirty, m_scene_dirty, volume_rotate_dirty;
	bool pre_draw_ = true;
	bool m_compute_created = false;
	bool m_use_space_mat = false;
	void Rotate(float radians);
	void render_scene(int view_id);
	void init_texture();
	void updateVolumeModelMat();
	void precompute();
	void AlignModelMatToTraversalPlane();
	bool isRayCasting() { return m_rmethod_id == (int)dvr::RAYCASTING; }
};
#endif