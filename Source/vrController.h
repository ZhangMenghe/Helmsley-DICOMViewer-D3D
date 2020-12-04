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
#include <pbr/PbrModel.h>
#include <pbr/PbrMaterial.h>
#include <pbr/PbrModelObject.h>

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

struct HandData {
	std::shared_ptr<XrHandTrackerEXT> TrackerHandle;

	// Data to display hand joints tracking
	std::shared_ptr<PbrModelObject> JointModel;
	std::array<Pbr::NodeIndex_t, XR_HAND_JOINT_COUNT_EXT> PbrNodeIndices{};
	std::array<XrHandJointLocationEXT, XR_HAND_JOINT_COUNT_EXT> JointLocations{};

	// Data to display hand mesh tracking
	std::shared_ptr<XrSpace> MeshSpace;
	std::shared_ptr<XrSpace> ReferenceMeshSpace;
	std::vector<DirectX::XMFLOAT4> VertexColors;
	std::shared_ptr<PbrModelObject> MeshObject;

	// Data to process open-palm reference hand.
	XrHandMeshMSFT meshState{ XR_TYPE_HAND_MESH_MSFT };
	std::unique_ptr<uint32_t[]> IndexBuffer{};
	std::unique_ptr<XrHandMeshVertexMSFT[]> VertexBuffer{};

	HandData() = default;
	HandData(HandData&&) = delete;
	HandData(const HandData&) = delete;
};

class vrController{
public:
	vrController(const std::shared_ptr<DX::DeviceResources>& deviceResources);
	static vrController* instance();
	void assembleTexture(int update_target, UINT ph, UINT pw, UINT pd, float sh, float sw, float sd, UCHAR* data, int channel_num = 4);

	void onReset();
	void onReset(glm::vec3 pv, glm::vec3 sv, glm::mat4 rm, Camera* cam);

	void CreateDeviceDependentResources();
	void CreateWindowSizeDependentResources();
	void ReleaseDeviceDependentResources();
	void Update(DX::StepTimer const& timer);
	void Update(XrTime time);
	void Render();
	void StartTracking();
	void TrackingUpdate(float positionX);
	void StopTracking();
	bool IsTracking() { return m_tracking; }

	//Interaction
	void onSingleTouchDown(float x, float y);
	void onSingle3DTouchDown(float x, float y, float z, int side);
	void onTouchMove(float x, float y);
	void on3DTouchMove(float x, float y, float z, int side);
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

	void setSpaces(XrSpace * space, XrSpace * app_space);

	//getter
	Texture* getVolumeTex() { return tex_volume; }
	Texture* getBakedTex() { return tex_baked; }
	bool isDirty();

private:
	XrSpace * space;
	XrSpace * app_space;

	static vrController* myPtr_;

	screenQuadRenderer* screen_quad;
	raycastVolumeRenderer* raycast_renderer;
	textureBasedVolumeRenderer* texvrRenderer_;
	cuttingController* cutter_;
	organMeshRenderer* meshRenderer_;
	std::unordered_map<int, lineRenderer*> line_renderers_;

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

	DirectX::XMMATRIX SpaceMat_;

	glm::mat4 ModelMat_, RotateMat_;
	glm::vec3 ScaleVec3_, PosVec3_;
	dvr::allConstantBuffer	m_all_buff_Data;
	std::map<std::string, reservedStatus> rStates_;

	//UI
	bool m_IsPressed = false;

	DirectX::XMFLOAT2 Mouse_old;

	bool m_IsPressed_left = false;
	bool m_IsPressed_right = false;
	DirectX::XMFLOAT3 Mouse3D_old_left;
	DirectX::XMFLOAT3 Mouse3D_old_right;

	float sens = 1.0f;// 0.1f;

	XrVector3f vector_old;
	float distance_old = 0;
	float uniScale = 0.5f;

	glm::fvec2 Mouse_old;
	std::string cst_name;

	//volume
	glm::vec3 vol_dimension_, vol_dim_scale_;
	glm::mat4 vol_dim_scale_mat_;

	//uint32	m_indexCount;
	//Texture* texture = nullptr;

	// hand

	Context context;
	enum class HandDisplayMode { Mesh, Joints, Count };
	HandDisplayMode m_mode{ HandDisplayMode::Mesh };

	std::shared_ptr<Pbr::Material> m_meshMaterial, m_jointMaterial;

	HandData m_leftHandData;
	HandData m_rightHandData;

	// Anchor
	XrSpatialAnchorMSFT* xrSpatialAnchorMSFT;
	
	//texture
	/*ID3D11SamplerState* m_sampleState;
		
	Texture* tex2d_srv_from_uav;
	*/
	bool	m_tracking;
	float	m_degreesPerSecond = 1;

	//flags
	bool volume_model_dirty;
	bool pre_draw_ = false;
	int frame_num = 0;
	void Rotate(float radians);
	void render_scene();
	void init_texture();
	void updateVolumeModelMat();
	void precompute();
	void getGraphPoints(float values[], float*& points);
	static bool isRayCasting() {
		return false;
		//return Manager::param_bool[dvr::CHECK_RAYCAST]; 
	}
};
#endif