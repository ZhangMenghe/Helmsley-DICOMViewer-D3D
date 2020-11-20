#include "pch.h"
#include <D3DPipeline/Texture.h>
#include <Common/DirectXHelper.h>
bool Texture::Initialize(ID3D11Device* device, D3D11_TEXTURE2D_DESC texDesc) {
	//HRESULT hr = device->CreateTexture2D(&texDesc, nullptr, mTex2D.put());
	//if (FAILED(hr)) {
	//	mTex2D = nullptr;
	//	return false;
	//}
	DX::ThrowIfFailed(device->CreateTexture2D(&texDesc, nullptr, mTex2D.put()));
	mWidth = texDesc.Width; mHeight = texDesc.Height;
	mDim = TWO_DIMS;
	D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc;
	shaderResourceViewDesc.Format = texDesc.Format;
	shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
	shaderResourceViewDesc.Texture2D.MipLevels = -1;
	auto hr = device->CreateShaderResourceView(mTex2D.get(), &shaderResourceViewDesc, &mTexView);
	if (FAILED(hr)) {
		mTex2D = nullptr; mTexView = nullptr;
		return false;
	}
	return true;
}

bool Texture::Initialize(ID3D11Device* device, D3D11_TEXTURE3D_DESC texDesc) {
	HRESULT hr = device->CreateTexture3D(&texDesc, nullptr, &mTex3D);
	if (FAILED(hr)) {
		mTex3D = nullptr;
		throw std::exception("fail to create 3d tex");
		return false;
	}
	mWidth = texDesc.Width; mHeight = texDesc.Height; mDepth = texDesc.Depth;
	mDim = THREE_DIMS;
	D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc;
	shaderResourceViewDesc.Format = texDesc.Format;
	shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
	shaderResourceViewDesc.Texture3D.MostDetailedMip = 0;
	shaderResourceViewDesc.Texture3D.MipLevels = -1;
	hr = device->CreateShaderResourceView(mTex3D, &shaderResourceViewDesc, &mTexView);
	if (FAILED(hr)) {
		mTex3D = nullptr; mTexView = nullptr;
		return false;
	}
	return true;
}

bool Texture::Initialize(
	ID3D11Device* device, ID3D11DeviceContext* context, 
	D3D11_TEXTURE2D_DESC texDesc, 
	const void* data) {
	if (!Initialize(device, texDesc)) return false;
	auto row_pitch = (texDesc.Width * 4) * sizeof(unsigned char);
	setTexData(context, data, row_pitch, 0);
	return true;
}
bool Texture::Initialize(ID3D11Device* device, ID3D11DeviceContext* context,
	D3D11_TEXTURE3D_DESC texDesc,
	const void* data) {
	if (!Initialize(device, texDesc)) return false;
	auto row_pitch = (texDesc.Width * 4) * sizeof(unsigned char);
	auto depth_pitch = row_pitch * texDesc.Height;
	setTexData(context, data, row_pitch, depth_pitch);
	return true;
}
bool Texture::Initialize(ID3D11Device* device, D3D11_TEXTURE2D_DESC texDesc, D3D11_RENDER_TARGET_VIEW_DESC renderTargetViewDesc) {
	if (!Initialize(device, texDesc)) return false;
	auto result = device->CreateRenderTargetView(mTex2D.get(), &renderTargetViewDesc, &m_renderTargetView);
	return !FAILED(result);
}
void Texture::GenerateMipMap(ID3D11DeviceContext* context) {
	if(mTexView!=nullptr)context->GenerateMips(mTexView);
}

void Texture::setTexData(ID3D11DeviceContext* context, const void* data, UINT row_pitch, UINT depth_pitch){
	switch (mDim) {
	case Texture::ONE_DIM:
		break;
	case Texture::TWO_DIMS:
		context->UpdateSubresource(mTex2D.get(), 0, nullptr, data, row_pitch, depth_pitch);
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