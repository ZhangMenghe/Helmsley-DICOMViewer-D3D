#ifndef D3DPIPELINE_TEXTURE_H
#define D3DPIPELINE_TEXTURE_H
#include "pch.h"
class Texture {
public:
	Texture(){}
	bool Initialize(ID3D11Device* device, D3D11_TEXTURE2D_DESC texDesc);
	bool Initialize(ID3D11Device* device, D3D11_TEXTURE3D_DESC texDesc);
	bool Initialize(ID3D11Device* device, D3D11_TEXTURE2D_DESC texDesc, D3D11_RENDER_TARGET_VIEW_DESC renderTargetViewDesc);

	bool Initialize(ID3D11Device* device, ID3D11DeviceContext* context, D3D11_TEXTURE2D_DESC texDesc, const void* data);
	bool Initialize(ID3D11Device* device, ID3D11DeviceContext* context, D3D11_TEXTURE3D_DESC texDesc, const void* data);

	void setTexData(ID3D11DeviceContext* context, const void* data, UINT row_pitch, UINT depth_pitch);

	void ShutDown();

	ID3D11Texture2D* GetTexture2D() { return mTex2D; }
	ID3D11Texture3D* GetTexture3D() { return mTex3D; }
	ID3D11ShaderResourceView* GetTextureView() { return mTexView; }
	ID3D11RenderTargetView* GetRenderTargetView() { return m_renderTargetView; }

	UINT Width() const { return mWidth; }
	UINT Height() const { return mHeight; }
	UINT Depth() const { return mDepth; }

private:
	enum TexDim
	{
		ONE_DIM=0,
		TWO_DIMS,
		THREE_DIMS
	};
	TexDim mDim;
	UINT mWidth;
	UINT mHeight;
	UINT mDepth;
	ID3D11ShaderResourceView* mTexView = nullptr;
	ID3D11RenderTargetView* m_renderTargetView = nullptr;
	ID3D11Texture2D* mTex2D = nullptr;
	ID3D11Texture3D* mTex3D = nullptr;
};

#endif // !D3DPIPELINE_TEXTURE_H
