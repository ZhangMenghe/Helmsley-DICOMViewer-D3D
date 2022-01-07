#ifndef VR_CONTROLLER_H
#define VR_CONTROLLER_H
#include <Renderers/quadRenderer.h>
#include <Common/DeviceResources.h>
#include <Common/Manager.h>
#include <Common/StepTimer.h>
#include <D3DPipeline/Camera.h>
#include <Renderers/baseDicomRenderer.h>
#include <Renderers/screenQuadRenderer.h>
#include <SceneObjs/cuttingPlane.h>
#include <glm/gtc/matrix_transform.hpp>
#include <Renderers/organMeshRenderer.h>
#include <Renderers/LineRenderer.h>
#include <SceneObjs/dataVisualizer.h>

class vrController{
public:
	vrController(const std::shared_ptr<DX::DeviceResources>& deviceResources, const std::shared_ptr<Manager>& manager);
	static vrController* instance();

	void assembleTexture(int update_target, UINT ph, UINT pw, UINT pd, float sh, float sw, float sd, UCHAR* data, int channel_num = 4);
	void onReset();
	void onReset(glm::vec3 pv, glm::vec3 sv, glm::mat4 rm, Camera* cam, const std::string& state_name = "");
	void onViewChanged(float width, float height);
	void Render(int view_id);

	//Interaction
	void onSingleTouchDown(float x, float y);
	void onSingle3DTouchDown(float x, float y, float z, int side);
	void onTouchMove(float x, float y);
	void on3DTouchMove(float x, float y, float z, glm::mat4 rot, int side, std::vector<dvr::OXR_POSE_TYPE>& types);
	void onTouchReleased();
	void on3DTouchReleased(int side);
	void onScale(float sx, float sy);
	void onScale(float scale);
	void onPan(float x, float y);

	//setter
	void setupCenterLine(int id, float* data);
	void setCuttingPlane(float value);
	void setCuttingPlane(int id, int delta);
	void setCuttingPlane(glm::vec3 pp, glm::vec3 pn);
	void switchCuttingPlane(dvr::PARAM_CUT_ID cut_plane_id);
	void setPosition(glm::vec3 pos) { SpaceMat_ = glm::translate(glm::mat4(1.0), pos); }
	void setPosition(glm::mat4 pos) { SpaceMat_ = pos; }
	void setMask(UINT num, UINT bits) { m_meshRenderer->SetMask(num, bits); }
	void setRenderingMethod(dvr::RENDER_METHOD method) {
		if (m_rmethod_id == method) return;
		m_rmethod_id = method;
	}
	void setRenderingParameters(dvr::RENDER_METHOD method, float* values) {
		if (method < m_vRenderers.size())
			m_vRenderers[method]->setRenderingParameters(values);
	}

	//getter
	void getCuttingPlane(DirectX::XMFLOAT4& pp, DirectX::XMFLOAT4& pn) { m_cutter->getCuttingPlane(pp, pn); }
	Texture* getVolumeTex() { return tex_volume.get(); }
	Texture* getBakedTex() { return tex_baked.get(); }
	ID3D11RasterizerState* m_render_state_front, * m_render_state_back;

	void getRST(glm::vec3& pos, glm::vec3& scale, glm::quat& rm) {
		pos = PosVec3_; scale = ScaleVec3_; rm = glm::quat_cast(RotateMat_);
	}

private:
	static vrController* myPtr_;

	std::vector<std::unique_ptr<baseDicomRenderer>> m_vRenderers;
	int m_rmethod_id = -1;

	std::unique_ptr<screenQuadRenderer> m_screen_quad;
	std::unique_ptr<cuttingController> m_cutter;
	std::unique_ptr<dataBoard> m_data_board;

	std::unique_ptr<organMeshRenderer> m_meshRenderer;
	std::unordered_map<int, std::unique_ptr<lineRenderer>> m_line_renderers;

	// Cached pointer to device resources.
	std::shared_ptr<DX::DeviceResources> m_deviceResources;
	std::shared_ptr<Manager> m_manager;

	//TEXTURES
	std::unique_ptr<Texture> tex_volume, tex_info, tex_baked;
	bool m_volume_valid = false;

	//compute shader
	std::unique_ptr<ID3D11ComputeShader> m_bakeShader;
	std::unique_ptr<ID3D11Texture3D> m_comp_tex_d3d;
	std::unique_ptr<ID3D11UnorderedAccessView> m_textureUAV;
	std::unique_ptr<ID3D11Buffer> m_compute_constbuff;
	bool m_comp_shader_setup = false;

	glm::mat4 SpaceMat_ = glm::mat4(1.0f);
	glm::mat4 ModelMat_, RotateMat_;
	glm::vec3 ScaleVec3_, PosVec3_;
	dvr::allConstantBuffer m_all_buff_Data;

	//UI
	bool m_IsPressed = false;
	glm::fvec2 Mouse_old;

	bool m_IsPressed3D[dvr::OXR_INPUT_END] = {false};
	glm::vec3 m_Mouse3D_old[dvr::OXR_INPUT_END];
	glm::vec3 vector_old;
	float distance_old = 0;

	//volume
	glm::vec3 vol_dimension_, vol_dim_scale_;
	glm::mat4 vol_dim_scale_mat_;

	//flags
	bool volume_model_dirty, volume_rotate_dirty;
	//bool m_use_space_mat = false;

	void precompute();
	void setup_compute_shader();
	void update_volume_model_mat();
	void align_volume_to_traversal_plane();
};
#endif