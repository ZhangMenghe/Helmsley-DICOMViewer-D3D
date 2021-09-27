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
	Camera* vcam;
	reservedStatus(glm::mat4 mm, glm::mat4 rm, glm::vec3 sv, glm::vec3 pv, Camera* cam)
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
	~vrController();
	static vrController* instance();

	void assembleTexture(int update_target, UINT ph, UINT pw, UINT pd, float sh, float sw, float sd, UCHAR* data, int channel_num = 4);
	void onReset();
	void onReset(glm::vec3 pv, glm::vec3 sv, glm::mat4 rm, Camera* cam);
	void InitOXRScene();
	void onViewChanged(float width, float height);
	void Render(int view_id);

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
	void setPosition(glm::vec3 pos) { SpaceMat_ = glm::translate(glm::mat4(1.0), pos); }
	void setPosition(glm::mat4 pos) { SpaceMat_ = pos; }
	void setUseSpaceMat(bool use, bool reset = true) {
		m_use_space_mat = use;
		//TODO:Change to different status
		if (reset) {
			ScaleVec3_ = dvr::DEFAULT_SCALE; RotateMat_ = glm::mat4(1.0f); PosVec3_ = glm::vec3(.0f);
			volume_model_dirty = true; volume_rotate_dirty = true;
		}
	}
	void setMask(UINT num, UINT bits) {
		meshRenderer_->SetMask(num, bits);
	}
	void setRenderingMethod(dvr::RENDER_METHOD method) {
		if (m_rmethod_id == method) return;
		m_rmethod_id = method;
	}
	void setRenderingParameters(dvr::RENDER_METHOD method, float* values) {
		if (method < vRenderer_.size())
			vRenderer_[method]->setRenderingParameters(values);
	}

	//getter
	void getCuttingPlane(DirectX::XMFLOAT4& pp, DirectX::XMFLOAT4& pn) { cutter_->getCuttingPlane(pp, pn); }
	Texture* getVolumeTex() { return tex_volume; }
	Texture* getBakedTex() { return tex_baked; }
	ID3D11RasterizerState* m_render_state_front, * m_render_state_back;

	void getRPS(glm::vec3& pos, glm::vec3& scale) {
		pos = PosVec3_; scale = ScaleVec3_;
	}

private:
	static vrController* myPtr_;

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
	ID3D11ComputeShader* bakeShader_;
	ID3D11Texture3D* m_comp_tex_d3d = nullptr;
	ID3D11UnorderedAccessView* m_textureUAV;
	ID3D11Buffer* m_compute_constbuff = nullptr;

	glm::mat4 SpaceMat_;
	glm::mat4 ModelMat_, RotateMat_;
	glm::vec3 ScaleVec3_, PosVec3_;
	dvr::allConstantBuffer m_all_buff_Data;
	std::map<std::string, reservedStatus> rStates_;

	//UI
	bool m_IsPressed = false;
	glm::fvec2 Mouse_old;

	bool m_IsPressed3D[dvr::OXR_INPUT_END] = {false};
	glm::vec3 m_Mouse3D_old[dvr::OXR_INPUT_END];
	glm::vec3 vector_old;
	float distance_old = 0;

	//MVP status
	std::string cst_name;

	//volume
	glm::vec3 vol_dimension_, vol_dim_scale_;
	glm::mat4 vol_dim_scale_mat_;

	//flags
	bool volume_model_dirty, volume_rotate_dirty;
	bool m_compute_created = false;
	bool m_use_space_mat = false;

	void precompute();
	void setup_compute_shader();
	void update_volume_model_mat();
	void align_volume_to_traversal_plane();
};
#endif