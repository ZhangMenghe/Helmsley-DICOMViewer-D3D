#ifndef BASE_DICOM_RENDERER_H
#define BASE_DICOM_RENDERER_H

#include <Renderers/baseRenderer.h>

class baseDicomRenderer:public baseRenderer {
public:
	baseDicomRenderer(ID3D11Device* device,
		const wchar_t* vname, const wchar_t* pname,
		const float* vdata = nullptr, const unsigned short* idata = nullptr,
		UINT vertice_num = 0, UINT idx_num = 0) :
		baseRenderer(device, vname, pname, vdata, idata, vertice_num, idx_num) {
	}
	virtual void setDimension(ID3D11Device* device, glm::vec3 vol_dimension, glm::vec3 vol_dim_scale){
		m_dimensions_origin = vol_dimension.z;
	}
	virtual void setCuttingPlane(float percent) {}
	virtual void setCuttingPlaneDelta(int delta) {}
	virtual void UpdateVertices(glm::mat4 model_mat) { m_vertices_dirty = false; }
	virtual bool isVerticesDirty() { return m_vertices_dirty; }
	virtual void setRenderingParameters(float* values) = 0;
	virtual bool Draw(ID3D11DeviceContext* context, Texture* tex, DirectX::XMMATRIX modelMat, bool is_front = true) = 0;
protected:
	int m_dimensions_origin;
	bool m_vertices_dirty = false;
	ID3D11BlendState* m_d3dBlendState;
};
#endif