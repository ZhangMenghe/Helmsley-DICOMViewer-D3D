#pragma once
#include "pch.h"
namespace HDUI {
	// Test if a 2D point (x,y) is in polygon with npol edges and xp,yp vertices
// The following code is by Randolph Franklin, it returns 1 for interior points and 0 for exterior points.
	inline int pnpoly(int npol, float* xp, float* yp, float x, float y) {
		int i, j, c = 0;
		for (i = 0, j = npol - 1; i < npol; j = i++) {
			if ((((yp[i] <= y) && (y < yp[j])) ||
				((yp[j] <= y) && (y < yp[i]))) &&
				(x < (xp[j] - xp[i]) * (y - yp[i]) / (yp[j] - yp[i]) + xp[i]))
				c = !c;
		}
		return c;
	}
	// Test if a sphere intersect with plane
	inline bool sphere_intersects_plane_point(glm::vec3 sc, float sr, glm::vec3 pp, glm::vec3 pn, float hprx, float hpry) {
		float d = glm::dot((sc - pp), pn);
		if (fabs(d) <= sr) {
			glm::vec3 proj = pn * d;
			auto point = sc - proj;
			float xps[4];
			float yps[4];
			xps[0] = pp.x - hprx; xps[1] = pp.x + hprx; xps[2] = pp.x + hprx; xps[3] = pp.x - hprx;
			yps[0] = pp.y + hpry; yps[1] = pp.y + hpry; yps[2] = pp.y - hpry; yps[3] = pp.y - hpry;

			return (pnpoly(4, xps, yps, point.x, point.y) == 1);
		}
		return false;
	}
	inline bool point2d_inside_rectangle(float offsetx, float offsety, float sizex, float sizey, float px, float py) {
		return (px >= offsetx
			&& px < offsetx + sizex
			&& py >= offsety
			&& py < offsety + sizey);
	}
	inline bool bounding_boxes_overlap(float b1x, float b1y, float b2x, float b2y, float radius) {

		return (std::min(b1x,b2x) > std::max(b1x, b2x) && // width > 0
			std::min(b1y, b2y) > std::max(b1y, b2y));

	}
	inline void update_board_projection_pos(float screen_width, float screen_height, DirectX::XMMATRIX& proj_mat, glm::vec3& size, glm::vec3& pos) {
		DirectX::XMFLOAT4X4 mmat_f;
		DirectX::XMStoreFloat4x4(&mmat_f, proj_mat);


		//DirectX::XMVECTOR point_model_space = DirectX::XMVectorSet(-0.5, 0, .0, 1.0);
		//auto proj_point_1 = DirectX::XMVector4Transform(point_model_space, proj_mat);

		//DirectX::XMVECTOR point_model_space2 = DirectX::XMVectorSet(0.5, 0, .0, 1.0);
		//auto proj_point_2 = DirectX::XMVector4Transform(point_model_space2, proj_mat);

		//DirectX::XMFLOAT4 v2F, v2F2;    //the float where we copy the v2 vector members
		//DirectX::XMStoreFloat4(&v2F, proj_point_1);   //the function used to copy
		//auto ndc_x1 = std::clamp(v2F.x, -v2F.w, v2F.w) / v2F.w;

		//DirectX::XMStoreFloat4(&v2F2, proj_point_2);   //the function used to copy

		//auto ndc_x2 = std::clamp(v2F2.x, -v2F2.w, v2F2.w) / v2F2.w;

		size = glm::vec3(mmat_f.m[0][0]/ mmat_f.m[3][3]*0.5f * screen_width, mmat_f.m[1][1] / mmat_f.m[3][3] * 0.5f * screen_height, .0f);

		auto ndc_y = std::clamp(mmat_f.m[1][3], -mmat_f.m[3][3], mmat_f.m[3][3]) / mmat_f.m[3][3];
		pos.y = (1.0f - 0.5f * (ndc_y + 1.0f)) * screen_height - size.y * 0.5f;

		auto ndc_x = std::clamp(mmat_f.m[0][3], -mmat_f.m[3][3], mmat_f.m[3][3]) / mmat_f.m[3][3];
		pos.x = (ndc_x + 1.0f) * 0.5f * screen_width - size.x * 0.5f;
	}
}

inline bool CheckHitRespToModelMtx(DirectX::XMMATRIX& view_proj_mat, DirectX::XMMATRIX& model_mat, 
	float screen_width, float screen_height,
	float px, float py,
	glm::vec3& proj_pos, glm::vec3& proj_size) {
	auto projMat = DirectX::XMMatrixMultiply(view_proj_mat,
		DirectX::XMMatrixTranspose(model_mat));
	HDUI::update_board_projection_pos(screen_width, screen_height, projMat, proj_size, proj_pos);
	return HDUI::point2d_inside_rectangle(proj_pos.x, proj_pos.y, proj_size.x, proj_size.y, px, py);
}
inline bool CheckHitWithinSphere(glm::vec3 sc, float radius, glm::vec3 check_pos, float half_plane_x, float half_plane_y) {
	return HDUI::sphere_intersects_plane_point(sc, radius, check_pos, glm::vec3(.0, .0, 1.0), half_plane_x, half_plane_y);
}