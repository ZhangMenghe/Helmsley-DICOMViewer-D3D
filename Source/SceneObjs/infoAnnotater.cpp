#include "pch.h"
#include "infoAnnotater.h"

infoAnnotater::infoAnnotater(){
}

void infoAnnotater::onCreateCanvas(ID3D11Device* device, glm::vec3 vol_dim_scale, UINT ph, UINT pw, UINT pd) {
	tex_info = std::make_unique<Texture>();
	D3D11_TEXTURE3D_DESC texInfoDesc{
		ph,pw,pd,
		1,
		DXGI_FORMAT_R32_UINT,
		D3D11_USAGE_DEFAULT,
		D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE,
		0,
		0
	};
	tex_info->Initialize(device, texInfoDesc);
	m_ph = ph; m_pw = pw; m_pd = pd;
	m_vol_dim_scale = vol_dim_scale;
	tex_info->createTexRaw(4, ph, pw, pd);

	m_brush_center = glm::vec3(-0.25f, 0.25f, 0.25f);//glm::vec3(-0.5f, 0.5f, 0.5f);
	m_brush_radius = 50;
	m_sphere = new sphereRenderer(device, 9, 7, { 1.0f, 1.0f, .0f, 1.0f });
}

bool infoAnnotater::stepCubeAnnotation(ID3D11DeviceContext* context, dvr::ANNOTATE_DIR dir, bool isBrush) {
	switch (dir)
	{
	case dvr::ANNOTATE_MOVE_Z_FORWARD:
		if (m_brush_center.z - m_step_size < -0.5f) return false;
		m_brush_center.z -= m_step_size;
		break;
	case dvr::ANNOTATE_MOVE_Z_BACKWARD:
		if (m_brush_center.z + m_step_size >0.5f) return false;
		m_brush_center.z += m_step_size;
		break;
	case dvr::ANNOTATE_MOVE_X_LEFT:
		if (m_brush_center.x - m_step_size < -0.5f) return false;
		m_brush_center.x -= m_step_size;
		break;
	case dvr::ANNOTATE_MOVE_X_RIGHT:
		if (m_brush_center.x + m_step_size > 0.5f) return false;
		m_brush_center.x += m_step_size;
		break;
	case dvr::ANNOTATE_MOVE_Y_DOWN:
		if (m_brush_center.y - m_step_size < -0.5f) return false;
		m_brush_center.y -= m_step_size;
		break;
	case dvr::ANNOTATE_MOVE_Y_UP:
		if (m_brush_center.y + m_step_size > 0.5f) return false;
		m_brush_center.y += m_step_size;
		break;
	default:
		break;
	}
	if (!isBrush) return false;
	return onDrawCube(context, m_brush_center, m_vol_dim_scale, m_brush_radius, { 0, 1, 2, 3 }, { 0xfc, 0xcc, 0xc0, 0x11 });
}
bool infoAnnotater::onDrawCube(
	ID3D11DeviceContext* context, glm::vec3 center, 
	glm::vec3 vol_dim_scale, 
	int sz, std::vector<int> pos, std::vector<unsigned char> value) {

	//d3d11_box: origin at left, top, front corner.
	glm::vec3 hsz = vol_dim_scale * (sz * 0.5f);
	auto cp = glm::vec3((center.x+0.5f) * m_pw, (0.5f-center.y) * m_ph, (center.z+0.5f) * m_pd);

	D3D11_BOX destRegion;
	destRegion.left = std::max(0, int(cp.x - hsz.x));
	destRegion.right = std::min(m_pw, UINT(cp.x + hsz.x));
	destRegion.top = std::max(0, int(cp.y - hsz.y));
	destRegion.bottom = std::min(m_ph, UINT(cp.y + hsz.y));
	destRegion.front = std::max(0, int(cp.z - hsz.z));
	destRegion.back = std::min(m_pd, UINT(cp.z + hsz.z));

	//TODO: TEST IF UPDATE NECESSARY?

	tex_info->setTexData(context, &destRegion, pos, value, sizeof(UINT));
	return true;
}
bool infoAnnotater::Draw(ID3D11DeviceContext* context, DirectX::XMMATRIX modelMat) {
	//m_sphere->Draw(context, modelMat);
	return true;
}
