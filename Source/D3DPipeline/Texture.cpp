#include "pch.h"
#include <D3DPipeline/Texture.h>

bool Texture::Initialize(ID3D11Device* device, D3D11_TEXTURE2D_DESC texDesc) {
	HRESULT hr = device->CreateTexture2D(&texDesc, nullptr, &mTex2D);
	if (FAILED(hr)) {
		mTex2D = nullptr;
		throw std::exception("fail to create ");
		return false;
	}
	mDim = TWO_DIMS;
	D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc;
	shaderResourceViewDesc.Format = texDesc.Format;
	shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
	shaderResourceViewDesc.Texture2D.MipLevels = 1;
	hr = device->CreateShaderResourceView(mTex2D, &shaderResourceViewDesc, &mTexView);
	if (FAILED(hr)) {
		mTex2D = nullptr; mTexView = nullptr;
		return false;
	}
	return true;
}
bool Texture::Initialize(ID3D11Device* device, D3D11_TEXTURE3D_DESC texDesc) {
	mDim = THREE_DIMS;
	return false;
}

bool Texture::Initialize(
	ID3D11Device* device, ID3D11DeviceContext* context, 
	D3D11_TEXTURE2D_DESC texDesc, 
	const void* data, UINT row_pitch, UINT depth_pitch) {
	if (!Initialize(device, texDesc)) return false;
	setTexData(context, data, row_pitch, depth_pitch);
	return true;
}
bool Texture::Initialize(ID3D11Device* device, ID3D11DeviceContext* context,
	D3D11_TEXTURE3D_DESC texDesc,
	const void* data, UINT row_pitch, UINT depth_pitch) {
	if (!Initialize(device, texDesc)) return false;
	setTexData(context, data, row_pitch, depth_pitch);
	return true;
}
void Texture::setTexData(ID3D11DeviceContext* context, const void* data, UINT row_pitch, UINT depth_pitch){
	switch (mDim) {
	case Texture::ONE_DIM:
		break;
	case Texture::TWO_DIMS:
		context->UpdateSubresource(mTex2D, 0, nullptr, data, row_pitch, depth_pitch);
		break;
	case Texture::THREE_DIMS:
		context->UpdateSubresource(mTex3D, 0, nullptr, data, row_pitch, depth_pitch);
		break;
	default:
		break;
	}
}

void Texture::ShutDown() {
	if (mTexView) {
		mTexView->Release();
		mTexView = nullptr;
	}
}