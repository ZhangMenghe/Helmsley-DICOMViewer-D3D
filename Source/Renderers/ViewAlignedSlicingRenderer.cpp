#include "pch.h"
#include "viewAlignedSlicingRenderer.h"
#include <D3DPipeline/Primitive.h>
#include <Common/Manager.h>
#include <vrController.h>
using namespace DirectX;
using namespace glm;


viewAlignedSlicingRenderer::viewAlignedSlicingRenderer(ID3D11Device* device)
	:baseDicomRenderer(device,
		L"viewSlicingVertexShader.cso", L"viewSlicingPixelShader.cso",
		nullptr, nullptr, 0, 0
	),
	cut_id(0)
{
	this->initialize();
	createDynamicVertexBuffer(device, 36 * MAX_DIMENSIONS);
}

void viewAlignedSlicingRenderer::create_vertex_shader(ID3D11Device* device, const std::vector<byte>& fileData) {
	winrt::check_hresult(
		device->CreateVertexShader(
			&fileData[0],
			fileData.size(),
			nullptr,
			m_vertexShader.put()
		)
	);
	static const D3D11_INPUT_ELEMENT_DESC vertexDesc[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	winrt::check_hresult(
		device->CreateInputLayout(
			vertexDesc,
			ARRAYSIZE(vertexDesc),
			&fileData[0],
			fileData.size(),
			m_inputLayout.put()
		)
	);
	m_vertex_stride = 3 * sizeof(float);//sizeof(DirectX::XMFLOAT3);
	m_vertex_offset = 0;
}
void viewAlignedSlicingRenderer::create_fragment_shader(ID3D11Device* device, const std::vector<byte>& fileData) {
	winrt::check_hresult(
		device->CreatePixelShader(
			&fileData[0],
			fileData.size(),
			nullptr,
			m_pixelShader.put()
		)
	);
	//texture
	D3D11_SAMPLER_DESC samplerDesc;

	// Create a texture sampler state description.
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
	samplerDesc.MipLODBias = 0.0f;
	samplerDesc.MaxAnisotropy = 1;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	samplerDesc.BorderColor[0] = 0;
	samplerDesc.BorderColor[1] = 0;
	samplerDesc.BorderColor[2] = 0;
	samplerDesc.BorderColor[3] = 0;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

	// Create the texture sampler state.
	winrt::check_hresult(device->CreateSamplerState(&samplerDesc, &m_sampleState));

	CD3D11_BUFFER_DESC constantBufferDesc(sizeof(dvr::ModelViewProjectionConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);
	winrt::check_hresult(
		device->CreateBuffer(
			&constantBufferDesc,
			nullptr,
			m_constantBuffer.put()
		)
	);

	CD3D11_BUFFER_DESC pixconstantBufferDesc(sizeof(texSlicingPixConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);
	winrt::check_hresult(
		device->CreateBuffer(
			&pixconstantBufferDesc,
			nullptr,
			m_pixConstantBuffer.put()
		)
	);

	D3D11_BLEND_DESC blendDesc;
	ZeroMemory(&blendDesc, sizeof(blendDesc));

	D3D11_RENDER_TARGET_BLEND_DESC rtbd;
	ZeroMemory(&rtbd, sizeof(rtbd));

	rtbd.BlendEnable = TRUE;
	rtbd.SrcBlend = D3D11_BLEND_SRC_ALPHA;//D3D11_BLEND_ONE;
	rtbd.DestBlend = D3D11_BLEND_INV_SRC_ALPHA;//D3D11_BLEND_INV_SRC_ALPHA;
	rtbd.BlendOp = D3D11_BLEND_OP_ADD;
	rtbd.SrcBlendAlpha = D3D11_BLEND_ONE;//D3D11_BLEND_ONE;
	rtbd.DestBlendAlpha = D3D11_BLEND_ZERO;//D3D11_BLEND_ZERO;
	rtbd.BlendOpAlpha = D3D11_BLEND_OP_ADD;
	rtbd.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	//rtbd.SrcBlend = D3D11_BLEND_SRC_ALPHA;//D3D11_BLEND_ONE;
	//rtbd.DestBlend = D3D11_BLEND_INV_SRC_ALPHA;//D3D11_BLEND_INV_SRC_ALPHA;
	//rtbd.BlendOp = D3D11_BLEND_OP_ADD;
	//rtbd.SrcBlendAlpha = D3D11_BLEND_ONE;//D3D11_BLEND_INV_DEST_ALPHA;
	//rtbd.DestBlendAlpha = D3D11_BLEND_ZERO;//D3D11_BLEND_ONE;
	//rtbd.BlendOpAlpha = D3D11_BLEND_OP_ADD;
	//rtbd.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	blendDesc.AlphaToCoverageEnable = false;
	blendDesc.RenderTarget[0] = rtbd;

	device->CreateBlendState(&blendDesc, &m_d3dBlendState);
}
void viewAlignedSlicingRenderer::initialize_mesh_others(ID3D11Device* device){
	m_data_dirty = true;
}
bool viewAlignedSlicingRenderer::Draw(ID3D11DeviceContext* context, Texture* tex, DirectX::XMMATRIX modelMat, bool is_front){
	if (!m_loadingComplete) return false;
	if (m_constantBuffer != nullptr) {
		DirectX::XMStoreFloat4x4(&m_const_buff_data.uViewProjMat, Manager::camera->getVPMat());
		DirectX::XMStoreFloat4x4(&m_const_buff_data.model, DirectX::XMMatrixTranspose(modelMat));

		// Prepare the constant buffer to send it to the graphics device.
		context->UpdateSubresource(
			m_constantBuffer.get(),
			0,
			nullptr,
			&m_const_buff_data,
			0,
			0
		);
		ID3D11Buffer* constantBufferNeverChanges{ m_constantBuffer.get() };
		context->VSSetConstantBuffers(0, 1, &constantBufferNeverChanges);
	}
	if (m_pixConstantBuffer != nullptr) {
		m_const_buff_data_pix.u_front = is_front;
		m_const_buff_data_pix.u_cut = Manager::param_bool[dvr::CHECK_CUTTING];
		if (m_const_buff_data_pix.u_cut) {
			vrController::instance()->getCuttingPlane(m_const_buff_data_pix.u_cut_point, m_const_buff_data_pix.u_cut_normal);
		}
		m_const_buff_data_pix.u_dist = slice_spacing;
		m_const_buff_data_pix.u_plane_normal = { pn_local.x, pn_local.y, pn_local.z, 0 };

		context->UpdateSubresource(
			m_pixConstantBuffer.get(),
			0,
			nullptr,
			&m_const_buff_data_pix,
			0,
			0
		);
		ID3D11Buffer* constantBuffer_pix{ m_pixConstantBuffer.get() };
		context->PSSetConstantBuffers(0, 1, &constantBuffer_pix);
	}
	
	context->OMSetBlendState(m_d3dBlendState, 0, 0xffffffff);

	D3D11_DEPTH_STENCIL_DESC dsDesc;

	// Depth test parameters
	dsDesc.DepthEnable = false;
	dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	dsDesc.DepthFunc = D3D11_COMPARISON_LESS;

	// Stencil test parameters
	dsDesc.StencilEnable = false;
	dsDesc.StencilReadMask = 0xFF;
	dsDesc.StencilWriteMask = 0xFF;

	// Create depth stencil state
	ID3D11DepthStencilState* pDSState;
	device->CreateDepthStencilState(&dsDesc, &pDSState);

	// Bind depth stencil state
	context->OMSetDepthStencilState(pDSState, 1);

	if (tex != nullptr) {
		auto texview = tex->GetTextureView();
		context->PSSetShaderResources(0, 1, &texview);
	}
	
	D3D11_RASTERIZER_DESC rsstate;
	rsstate.FillMode = D3D11_FILL_SOLID;
	rsstate.FrontCounterClockwise = FALSE;
	rsstate.DepthBias = 0;
	rsstate.SlopeScaledDepthBias = 0.0f;
	rsstate.DepthBiasClamp = 0.0f;
	rsstate.DepthClipEnable = TRUE;
	rsstate.ScissorEnable = FALSE;
	rsstate.MultisampleEnable = FALSE;
	rsstate.AntialiasedLineEnable = FALSE;
	rsstate.CullMode = D3D11_CULL_NONE;

	ID3D11RasterizerState* pRasterizerState;
  device->CreateRasterizerState(
		&rsstate,
		&pRasterizerState
	);

	context->RSSetState(
		pRasterizerState
	);
  baseRenderer::Draw(context, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	//setback states
	context->OMSetBlendState(nullptr, 0, 0xffffffff);
	context->OMSetDepthStencilState(nullptr, 1);
	return true;
}
void viewAlignedSlicingRenderer::setDimension(ID3D11Device* device, glm::vec3 vol_dimension, glm::vec3 vol_dim_scale) {
	dimensions = int(vol_dimension.z * DENSE_FACTOR); dimension_inv = 1.0f / dimensions;
	vol_thickness_factor = vol_dim_scale.z;// *2.0f;
	initialize_mesh_others(device);
}
void viewAlignedSlicingRenderer::setCuttingPlane(float percent) {
	cut_id = int(dimensions * percent);
	//baked_dirty_ = true;
}
void viewAlignedSlicingRenderer::setCuttingPlaneDelta(int delta) {
	cut_id = ((int)fmax(0, cut_id + delta)) % dimensions;
	//baked_dirty_ = true;
}

float RayPlane(vec3 ro, vec3 rd, vec3 pp, vec3 pn) {
	float d = dot(pn, rd);
	float t = dot(pp - ro, pn);
	return glm::abs(d) > 1e-5 ? (t / d) : (t > .0 ? 1e5 : -1e5);
}

void PlaneBox(vec3 aabb_min, vec3 aabb_max, vec3 pp, vec3 pn, std::vector<vec3>& out_points) {
	float t;
	vec3 rd;
	//axis-x
	rd = vec3(aabb_max.x - aabb_min.x, .0f, .0f);
	vec3 ro_x[4] = {
					vec3(aabb_min.x, aabb_min.y, aabb_min.z),
					vec3(aabb_min.x, aabb_max.y, aabb_min.z),
					vec3(aabb_min.x, aabb_min.y, aabb_max.z),
					vec3(aabb_min.x, aabb_max.y, aabb_max.z)
	};

	for (auto ro : ro_x) {
		t = RayPlane(ro, rd, pp, pn);
		if (t >= .0 && t < 1.0f) out_points.push_back(ro + rd * t);
	}

	//axis-y
	rd = vec3(.0f, aabb_max.y - aabb_min.y, .0f);
	vec3 ro_y[4] = {
					vec3(aabb_min.x, aabb_min.y, aabb_min.z),
					vec3(aabb_max.x, aabb_min.y, aabb_min.z),
					vec3(aabb_min.x, aabb_min.y, aabb_max.z),
					vec3(aabb_max.x, aabb_min.y, aabb_max.z)
	};
	for (auto ro : ro_y) {
		t = RayPlane(ro, rd, pp, pn);
		if (t >= .0 && t < 1.0f) out_points.push_back(ro + rd * t);
	}

	//axis-z
	rd = vec3(.0f, .0f, aabb_max.z - aabb_min.z);
	vec3 ro_z[4] = {
					vec3(aabb_min.x, aabb_min.y, aabb_min.z),
					vec3(aabb_max.x, aabb_min.y, aabb_min.z),
					vec3(aabb_min.x, aabb_max.y, aabb_min.z),
					vec3(aabb_max.x, aabb_max.y, aabb_min.z)
	};
	for (auto ro : ro_z) {
		t = RayPlane(ro, rd, pp, pn);
		if (t >= .0 && t < 1.0f) out_points.push_back(ro + rd * t);
	}
}

void SortPoints(std::vector<vec3>& points, vec3 pn) {
	if (points.empty()) return;

	auto origin = points[0];

	std::sort(points.begin() + 1, points.end(), [origin, pn](const glm::vec3& lhs, const glm::vec3& rhs) -> bool
	{
		glm::vec3 v = glm::cross(glm::normalize(lhs - origin), glm::normalize(rhs - origin));
		return glm::dot(v, pn) > .0f;
	});
}

void viewAlignedSlicingRenderer::updateVertices(ID3D11DeviceContext* context, glm::mat4 model_mat)
{
	glm::mat4 view_t = xmmatrix2mat4(Manager::camera->getViewMat());
	glm::mat4 view_ = glm::transpose(view_t);
	glm::mat4 cam = glm::inverse(view_);
	glm::vec3 eye = cam[3];
	glm::vec3 center = model_mat[3];
	glm::vec3 up = glm::vec3(0, 1, 0);

	view_ = glm::lookAt(eye, center, up);
	glm::mat4 model_view = view_ * model_mat;
	glm::mat4 model_view_inv = glm::inverse(model_view);

	glm::vec4 pnv4 = cam * glm::vec4(.0, .0, 1.0f, 1.0f);
	//glm::vec3 pn = glm::normalize(vec3(pnv4.x, pnv4.y, pnv4.z) / pnv4.w);

	glm::vec3 pn = glm::normalize(eye - center);

	

	//if (dvr::VIEW_ALIGNED_LAZY_UPDATE) {
	//	if (abs(pn.x) > abs(pn.y) && abs(pn.x) > abs(pn.z)) { pn.x = pn.x > .0f ? 1.0 : -1.0; pn.y = 0; pn.z = 0; }
	//	else if (abs(pn.y) > abs(pn.x) && abs(pn.y) > abs(pn.z)) { pn.y = pn.y > .0f ? 1.0 : -1.0; pn.x = 0; pn.z = 0; }
	//	else if (abs(pn.z) > abs(pn.y) && abs(pn.z) > abs(pn.x)) { pn.z = pn.z > .0f ? 1.0 : -1.0; pn.y = 0; pn.x = 0; }
	//	if (glm::all(glm::equal(m_last_vec3, pn))) return;
	//	m_last_vec3 = pn;
	//	//    LOGE("====PN : %f, %f, %f", pn.x, pn.y, pn.z);
	//	//    LOGE("====pp : %f, %f, %f", pp.x, pp.y, pp.z);
	//}

	//Step 1: Transform the volume bounding box vertices into view coordinates using the modelview matrix
	// Volume view vertices
	float volume_view_vertices[8][3];

	std::vector<glm::vec4> volume_models;

	for (int i = 0; i < 8; i++) {
		glm::vec4 v(cuboid[3 * i], cuboid[3 * i + 1], cuboid[3 * i + 2], 1.0f);
		glm::vec4 vv = model_mat * v;
		volume_models.push_back(vv);
		volume_view_vertices[i][0] = vv.x / vv.w; volume_view_vertices[i][1] = vv.y / vv.w; volume_view_vertices[i][2] = vv.z / vv.w;
	}
	//Step 2: Find the minimum and maximum z coordinates of the transformed vertices. Compute the number of sampling planes used between these two values using equidistant spacing from the view origin. The sampling distance is computed from the voxel size and current sampling rate.
	float z_min = std::numeric_limits<float>::max(), z_max = -z_min;
	int z_min_id = -1, z_max_id = -1;
	for (int i = 0; i < 8; i++) {
		
		auto dist = glm::dot(glm::vec3(volume_models[i]) - eye, pn);

		if (dist > z_max) {
			z_max = dist; z_max_id = i;
		}
		else if (dist < z_min) {
			z_min = dist; z_min_id = i;
		}
	}

	glm::vec3 zmin_model = volume_models[z_min_id];
	glm::vec3 zmax_model = volume_models[z_max_id];
	glm::vec3 distance = zmax_model - zmin_model;
	float slice_distance = glm::abs(glm::dot(distance, pn)); //glm::length(distance);

	m_slice_num = std::min(int(float(dimensions) * slice_distance * DENSE_FACTOR), MAX_DIMENSIONS);
	

	//For each plane in front-to-back or back-to-front order
	vec3 aabb_min(-0.5f), aabb_max(0.5f);

	int id = 3 * z_min_id;
	int end_id = 3 * z_max_id;
	glm::vec3 pp = vec3(cuboid[id], cuboid[id + 1], cuboid[id + 2]);
	glm::vec3 pp_end = vec3(cuboid[end_id], cuboid[end_id + 1], cuboid[end_id + 2]);

	pn_local = glm::normalize(glm::inverse(glm::mat3(model_mat)) * pn);

	float local_dist = glm::abs(glm::dot(pp_end - pp, pn_local));
	slice_spacing = local_dist / float(m_slice_num); // TODO

	int m_indice_sum = 0;
  float vertices[36 * MAX_DIMENSIONS] = { .0f };
	int ind = 0;

	for (int slice = 0; slice < m_slice_num; slice++, pp += pn_local * slice_spacing) {

		int vertex_num;

	  // a. Test for intersections with the edges of the bounding box. Add each intersection point to a temporary vertex list. Up to six intersections are generated, so the maximum size of the list is fixed.
		std::vector<vec3> polygon_points;
		PlaneBox(aabb_min, aabb_max, pp, pn_local, polygon_points);

		vertex_num = polygon_points.size();
		m_indice_num[slice] = (vertex_num - 2) * 3;

		if (vertex_num < 3) continue;

		//b. compute center and sort them in counter-clockwise
		//c. Tessellate the proxy polygon
		SortPoints(polygon_points, pn_local);

		/*for (int i = 0; i < vertex_num; i++) {
			vertices[3 * i] = polygon_points[i].x; vertices[3 * i + 1] = polygon_points[i].y; vertices[3 * i + 2] = polygon_points[i].z;
		}*/

		for (int i = 0; i < vertex_num - 2; i++) {
			vertices[9 * ind] = polygon_points[0].x; vertices[9 * ind + 1] = polygon_points[0].y; vertices[9 * ind + 2] = polygon_points[0].z;
			vertices[9 * ind + 3] = polygon_points[i + 1].x; vertices[9 * ind + 4] = polygon_points[i + 1].y; vertices[9 * ind + 5] = polygon_points[i + 1].z;
			vertices[9 * ind + 6] = polygon_points[i + 2].x; vertices[9 * ind + 7] = polygon_points[i + 2].y; vertices[9 * ind + 8] = polygon_points[i + 2].z;
			ind += 1;
		}

		m_indice_sum += m_indice_num[slice];
	}

	m_vertice_count = m_indice_sum * 3;//vertex_num * 3;
	m_index_count = 0;// m_indice_num[slice];
	updateVertexBuffer(context, vertices);
	m_vertice_count = m_indice_sum;
}
