﻿#include "pch.h"
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
	tex_info->createTexRaw(ph, pw, pd, 4);

	m_brush_center = glm::vec3(-0.25f, 0.25f, 0.25f);//glm::vec3(-0.5f, 0.5f, 0.5f);
	m_brush_radius = 50;
	m_sphere = new sphereRenderer(device, 9, 7, { 1.0f, 1.0f, .0f, 1.0f });
}
void infoAnnotater::brushCubeAnnotation(ID3D11DeviceContext* context, float offsetx, float offsety) {
	float offsetz = .0f;
	if (m_brush_center.x < 0.25f) offsetx += 0.05;
	else if (m_brush_center.y > -0.25f) offsety -= 0.05;
	else offsetz -= 0.05f;
	m_brush_center += glm::vec3(offsetx, offsety, offsetz);
	onDrawCube(context, m_brush_center, m_vol_dim_scale, m_brush_radius, { 0, 1, 2, 3 }, { 0xfc, 0xcc, 0xc0, 0x11 });
}
void infoAnnotater::onDrawCube(
	ID3D11DeviceContext* context, glm::vec3 center, 
	glm::vec3 vol_dim_scale, 
	int sz, std::vector<int> pos, std::vector<unsigned char> value) {
	//UINT* pData = new UINT[m_pix_num];
	//for (auto i = 0; i < m_pix_num; i++)pData[i] = 0xffffffff;
	//context->UpdateSubresource(tex_info->GetTexture3D(), 0, nullptr, pData, 512 * sizeof(UINT), 512 * 512 * sizeof(UINT));

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

	tex_info->setTexData(context, &destRegion, pos, value, sizeof(UINT));
}
bool infoAnnotater::Draw(ID3D11DeviceContext* context, DirectX::XMMATRIX modelMat) {
	//m_sphere->Draw(context, modelMat);
	return true;
}
