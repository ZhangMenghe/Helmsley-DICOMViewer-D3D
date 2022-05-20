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
	auto sz = row_pitch * texDesc.Height;
	m_rawdata = new unsigned char[sz];
	memcpy(m_rawdata, data, sz);

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
void Texture::createTexRaw(int unit_size, UINT ph, UINT pw, UINT pd) {
	m_rawdata = new unsigned char[ph * pw * pd * unit_size];
	memset(m_rawdata, 0x00, ph * pw * pd * unit_size);
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
void Texture::setTexData(ID3D11DeviceContext* context, D3D11_BOX* box,
	std::vector<int> pos, std::vector<unsigned char> value, 
	int unit_size, const BYTE* mask) {
	auto width = box->right - box->left;
	auto height = box->bottom - box->top;
	auto depth = box->back - box->front;

	auto row_pitch = width * unit_size;
	auto depth_pitch = row_pitch * height;

	int idx = 0, mask_id = 0;
	std::lock_guard<std::mutex> lock(m_memory_mutex);
	unsigned char* fData = new unsigned char[depth_pitch * depth];
	memset(fData, 0x00, depth_pitch * depth);
	for (auto z = box->front; z < box->back; z++) {
		auto slice_offset = depth_pitch * z;
		for (auto y = box->top; y < box->bottom; y++) {
			auto line_offset = slice_offset + mWidth * y * unit_size;
			auto buff_offset = idx;
			for (auto x = box->left; x < box->right; x++) {
				if (mask[mask_id++]) { 
					memset(m_rawdata + line_offset+x*unit_size, 0xff, unit_size);
				}
				idx += unit_size;
			}
			
			memcpy(fData + buff_offset, m_rawdata + line_offset + box->left * unit_size, unit_size * (box->right - box->left));
		}
	}
	if(mDim == Texture::THREE_DIMS)
	context->UpdateSubresource(
		mTex3D,
		0,
		box,
		fData,
		row_pitch,
		depth_pitch);
	else
		context->UpdateSubresource(
			mTex2D.get(),
			0,
			box,
			fData,
			row_pitch,
			depth_pitch);
	delete fData; fData = nullptr;
} 

void Texture::ShutDown() {
	if (mTexView) {
		mTexView->Release();
		mTexView = nullptr;
	}
}