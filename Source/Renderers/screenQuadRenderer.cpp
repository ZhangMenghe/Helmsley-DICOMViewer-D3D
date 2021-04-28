#include "pch.h"
#include "screenQuadRenderer.h"
#include <D3DPipeline/Primitive.h>
using namespace DirectX;
screenQuadRenderer* screenQuadRenderer::myPtr_ = nullptr;
screenQuadRenderer* screenQuadRenderer::instance() {
	return myPtr_;
}
screenQuadRenderer::screenQuadRenderer(ID3D11Device* device)
	:quadRenderer(device, L"Naive2DTexVertexShader.cso", L"QuadPixelShader.cso", quad_vertices_pos_w_tex_full){
	myPtr_ = this;

	D3D11_BLEND_DESC omDesc;
	ZeroMemory(&omDesc, sizeof(D3D11_BLEND_DESC));
	omDesc.RenderTarget[0].BlendEnable = TRUE;
	omDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_COLOR;
	omDesc.RenderTarget[0].DestBlend = D3D11_BLEND_SRC_COLOR;
	omDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	omDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_SRC_ALPHA;
	omDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
	omDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	omDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	device->CreateBlendState(&omDesc, &d3dBlendState);
}

bool screenQuadRenderer::InitializeQuadTex(ID3D11Device* device, ID3D11DeviceContext* context, float width, float height){
	{
		texture = new Texture;
		D3D11_TEXTURE2D_DESC texDesc;
		texDesc.Width = width;
		texDesc.Height = height;
		texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		texDesc.Usage = D3D11_USAGE_DEFAULT;
		texDesc.MipLevels = 1;
		texDesc.ArraySize = 1;
		texDesc.SampleDesc.Count = 1;
		texDesc.SampleDesc.Quality = 0;
		texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
		texDesc.CPUAccessFlags = 0;
		texDesc.MiscFlags = D3D11_RESOURCE_MISC_SHARED;

		D3D11_RENDER_TARGET_VIEW_DESC view_desc{
			texDesc.Format,
			D3D11_RTV_DIMENSION_TEXTURE2D,
		};
		view_desc.Texture2D.MipSlice = 0;
		if (!texture->Initialize(device, texDesc, view_desc)) { delete texture; texture = nullptr; return false; }
	}


	{
		ID3D11Texture2D* depth_texture = nullptr;
		D3D11_TEXTURE2D_DESC texDesc;
		texDesc.Width = width;
		texDesc.Height = height;
		texDesc.Format = DXGI_FORMAT_D16_UNORM ;
		texDesc.Usage = D3D11_USAGE_DEFAULT;
		texDesc.MipLevels = 1;
		texDesc.ArraySize = 1;
		texDesc.SampleDesc.Count = 1;
		texDesc.SampleDesc.Quality = 0;
		texDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
		texDesc.CPUAccessFlags = 0;
		texDesc.MiscFlags = 0;

		/*D3D11_RENDER_TARGET_VIEW_DESC view_desc{
			texDesc.Format,
			D3D11_RTV_DIMENSION_TEXTURE2D,
		};
		view_desc.Texture2D.MipSlice = 0;*/

		CD3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc(
			D3D11_DSV_DIMENSION_TEXTURE2D,
			DXGI_FORMAT_D16_UNORM ,
			0 /* mipSlice */,
			0,
			1 /* arraySize */);
		device->CreateTexture2D(&texDesc, nullptr, &depth_texture);
		device->CreateDepthStencilView(depth_texture, &depthStencilViewDesc, &depthStencilView);
	}

	//D3D11_MAPPED_SUBRESOURCE mappedResource;
	//context->Map(texture->GetTexture2D(), 0, D3D11_MAP_READ, 0, &mappedResource);

	//debug only: fuse tex with naive data
	/*auto imageSize = texDesc.Width * texDesc.Height * 4;
	unsigned char* m_targaData = new unsigned char[imageSize];
	for (int i = 0; i < imageSize; i += 4) {
		m_targaData[i] = (unsigned char)255;
		m_targaData[i + 1] = 0;
		m_targaData[i + 2] = 0;
		m_targaData[i + 3] = (unsigned char)255;
	}
	if (!texture->Initialize(device, context, texDesc, m_targaData)) { delete texture; texture = nullptr; return false; }
	
	*/
	//D3D11_MAPPED_SUBRESOURCE mappedResource;
	//context->Map(texture->GetTexture2D(), 0, D3D11_MAP_READ, 0, &mappedResource);

	return true;
}
void screenQuadRenderer::Draw(ID3D11DeviceContext* context) {
	if (!m_loadingComplete) return;
	//context->OMSetBlendState(d3dBlendState, 0, 0xffffffff);
	baseRenderer::Draw(context);
}
void screenQuadRenderer::SetToDrawTarget(ID3D11DeviceContext* context, DX::DeviceResources* resources){
	auto tview = texture->GetRenderTargetView();
	// Create a depth stencil view into the slice of the depth stencil texture array for this swapchain image.
				// This is a lightweight operation which can be done for each viewport projection.
	

	ID3D11RenderTargetView* const targets[1] = { tview };
	context->OMSetRenderTargets(1, targets, depthStencilView);
	resources->saveCurrentTargetViews(tview, depthStencilView);
	context->ClearRenderTargetView(tview, dvr::SCREEN_CLEAR_COLOR);
	context->ClearDepthStencilView(depthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);
}
