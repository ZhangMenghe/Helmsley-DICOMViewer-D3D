#pragma once
#include "pch.h"
namespace {
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

	inline void update_board_projection_pos(float screen_width, float screen_height, DirectX::XMMATRIX& proj_mat, glm::vec3& size, glm::vec3& pos) {
		DirectX::XMFLOAT4X4 mmat_f;
		DirectX::XMStoreFloat4x4(&mmat_f, proj_mat);

		size = glm::vec3(mmat_f.m[0][0] * screen_width, mmat_f.m[1][1] * screen_height, .0f);

		auto ndc_y = std::clamp(mmat_f.m[1][3], -mmat_f.m[3][3], mmat_f.m[3][3]) / mmat_f.m[3][3];
		pos.y = (1.0f - 0.5f * (ndc_y + 1.0f)) * screen_height - size.y * 0.5f;

		auto ndc_x = std::clamp(mmat_f.m[0][3], -mmat_f.m[3][3], mmat_f.m[3][3]) / mmat_f.m[3][3];
		pos.x = (ndc_x + 1.0f) * 0.5f * screen_width - size.x * 0.5f;
	}
}

inline bool CheckHit(DirectX::XMMATRIX& view_proj_mat, DirectX::XMMATRIX& model_mat, 
	float screen_width, float screen_height,
	float px, float py) {
	auto projMat = DirectX::XMMatrixMultiply(view_proj_mat,
		DirectX::XMMatrixTranspose(model_mat));
	glm::vec3 pos, size;
	update_board_projection_pos(screen_width, screen_height, projMat, size, pos);
	return point2d_inside_rectangle(pos.x, pos.y, size.x, size.y, px, py);
}