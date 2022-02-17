#include "pch.h"
#include "dataVisualizer.h"
#include <Common/Manager.h>
#include <D3DPipeline/Primitive.h>
#include <vrController.h>
#include <Common/DirectXHelper.h>
using namespace dvr;

dataBoard::dataBoard(ID3D11Device* device){
    onReset(device);
    //todo: seems not working ...
    //ID3D11DepthStencilState* m_DepthStencilState;
    CD3D11_DEPTH_STENCIL_DESC depthStencilDesc(CD3D11_DEFAULT{});
    depthStencilDesc.DepthEnable = true;
    depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
    depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    //winrt::check_hresult(device->CreateDepthStencilState(&depthStencilDesc, &m_DepthStencilState));

    if (d3dBlendState == nullptr) {
        D3D11_BLEND_DESC omDesc;
        ZeroMemory(&omDesc, sizeof(D3D11_BLEND_DESC));
        omDesc.RenderTarget[0].BlendEnable = TRUE;
        omDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
        omDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
        omDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
        omDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_SRC_ALPHA;
        omDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
        omDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
        omDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
        device->CreateBlendState(&omDesc, &d3dBlendState);
    }
}
dataBoard::~dataBoard(){
    for (auto render : m_opacity_graphs) delete render;
    m_opacity_graphs.clear();
    for (auto vertices : m_opacity_vertices) {
        delete[]vertices; vertices = nullptr;
    }
    m_opacity_vertices.clear();
    delete m_board_quad; m_board_quad = nullptr;
    delete m_color_bar; m_color_bar = nullptr;
    //delete m_DepthStencilState; m_DepthStencilState = nullptr;
    delete d3dBlendState; d3dBlendState = nullptr;
}
void getVerticesWithScaleOff(float*& qvertices, DirectX::XMFLOAT3 scale, DirectX::XMFLOAT3 offset, bool is_with_tex) {
    if (is_with_tex) {
        for (int i = 0; i < 4; i++) {
            qvertices[6 * i] = quad_vertices_pos_w_tex[6 * i] * scale.x + offset.x;
            qvertices[6 * i + 1] = quad_vertices_pos_w_tex[6 * i + 1] * scale.y + offset.y;
            qvertices[6 * i + 2] = quad_vertices_pos_w_tex[6 * i + 2] * scale.y + offset.z;
        }
    }
    else {
        for (int i = 0; i < 4; i++) {
            qvertices[3 * i] = quad_vertices_3d[3 * i] * scale.x + offset.x;
            qvertices[3 * i + 1] = quad_vertices_3d[3 * i + 1] * scale.y + offset.y;
            qvertices[3 * i + 2] = quad_vertices_3d[3 * i + 2] * scale.z + offset.z;
        }
    }
}
void dataBoard::onReset(ID3D11Device* device){
    //CREATE NEW
    if (!m_initialized) {
        m_board_quad = new quadRenderer(device, { 0.086f, 0.098f, 0.23f, 1.0f });

        //color bar
        m_color_vertices = new float[24];
        memcpy(m_color_vertices, quad_vertices_pos_w_tex, 24 * sizeof(float));

        DirectX::XMFLOAT3 scale = { 0.99f, 0.2f, 1.0f };
        DirectX::XMFLOAT3 offset = { .0f, 0.4f, .001f };

        for (int i = 0; i < 4; i++) {
            m_color_vertices[6 * i] = quad_vertices_pos_w_tex[6 * i] * scale.x + offset.x;
            m_color_vertices[6 * i + 1] = quad_vertices_pos_w_tex[6 * i + 1] * scale.y + offset.y;
            m_color_vertices[6 * i + 2] = quad_vertices_pos_w_tex[6 * i + 2] * scale.y + offset.z;
        }

        m_color_bar = new quadRenderer(device, L"QuadVertexShader.cso", L"QuadPixelShader.cso", m_color_vertices);

        //compute shader

        auto loadCSTask = DX::ReadDataAsync(L"colorTransferCompute.cso");
        // After the vertex shader file is loaded, create the shader and input layout.
        auto createCSTask = loadCSTask.then([this, device](const std::vector<byte>& fileData) {
            DX::ThrowIfFailed(
                device->CreateComputeShader(
                    &fileData[0],
                    fileData.size(),
                    nullptr,
                    &m_color_comp_shader
                )
            );

            CD3D11_BUFFER_DESC constantBufferDesc(sizeof(DirectX::XMFLOAT4X4) * 12, D3D11_BIND_CONSTANT_BUFFER);
            winrt::check_hresult(
                device->CreateBuffer(
                    &constantBufferDesc,
                    nullptr,
                    &m_compute_constbuff
                )
            );
        });

        D3D11_TEXTURE2D_DESC texDesc;
        texDesc.Width = 256;
        texDesc.Height = 120;
        texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        texDesc.Usage = D3D11_USAGE_DEFAULT;
        texDesc.MipLevels = 1;
        texDesc.ArraySize = 1;
        texDesc.SampleDesc.Count = 1;
        texDesc.SampleDesc.Quality = 0;
        texDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
        texDesc.CPUAccessFlags = 0;
        texDesc.MiscFlags = 0;

        DX::ThrowIfFailed(
            device->CreateTexture2D(&texDesc, nullptr, &m_comp_tex)
        );
        D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
        uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
        uavDesc.Format = texDesc.Format;
        uavDesc.Texture2D.MipSlice = 0;
        DX::ThrowIfFailed(
            device->CreateUnorderedAccessView(m_comp_tex, &uavDesc, &m_textureUAV)
        );


        texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
        texDesc.MiscFlags = D3D11_RESOURCE_MISC_SHARED;

        D3D11_RENDER_TARGET_VIEW_DESC view_desc{
            texDesc.Format,
            D3D11_RTV_DIMENSION_TEXTURE2D,
        };
        view_desc.Texture2D.MipSlice = 0;

        m_color_tex = new Texture;
        m_color_tex->Initialize(device, texDesc, view_desc);

        m_color_bar->setTexture(m_color_tex);
        m_initialized = true;
    }
}

void dataBoard::onViewChange(int width, int height){
    //if(!rects_.empty()){
    //    //setup overlay rect
    //    for(auto &rect:rects_){
    //        auto& r=rect.second;
    //        if(r.width > 1.0f){
    //            r.width/=width; r.left/=width;
    //            r.height/=height; r.top/=height;
    //        }else if(_screen_w!=0){
    //            r.width*=_screen_w/width; r.left*=_screen_w/width;
    //            r.height*=_screen_h/height; r.top*=_screen_h/height;
    //        }
    //        renderers_[rect.first]->setRelativeRenderRect(r.width, r.height, r.left, r.top-r.height);
    //    }
    //}
    //_screen_w = width; _screen_h = height;
}

void dataBoard::Update(ID3D11Device* device, ID3D11DeviceContext* context) {
    volumeSetupConstBuffer vol_setup;
    Manager::instance()->getVolumeSetupConstData(vol_setup);
    if (vol_setup.u_widget_num == 0) {
        for (auto renderer : m_opacity_graphs) delete renderer;
        for (auto data : m_opacity_vertices) { delete[]data; data = nullptr; }
    }
    else{
        int dirty_wid = Manager::instance()->getDirtyOpacityId();
        if (dirty_wid < 0)return;
        switch (vol_setup.u_widget_num - m_opacity_graphs.size()) {
            //remove graph at dirty wid
        case -1:
            m_opacity_graphs.erase(m_opacity_graphs.begin() + dirty_wid);
            m_opacity_vertices.erase(m_opacity_vertices.begin() + dirty_wid);
            break;
            //change data
        case 0:
            setup_opacity_widget_vertcies(
                Manager::instance()->getDirtyWidgetPoints(),
                m_opacity_vertices[dirty_wid],
                dirty_wid+1
            );
            m_opacity_graphs[dirty_wid]->updateVertexBuffer(context, m_opacity_vertices[dirty_wid]);
            break;
        case 1:
            m_opacity_graphs.push_back(new quadRenderer(device,
                L"Naive3DVertexShader.cso", L"ClippedColorPixelShader.cso",
                nullptr, m_opacity_indices, 0, 12, dvr::INPUT_POS_3D
            ));

            m_opacity_vertices.push_back(new float[18]);
            if (m_default_opacity_data == nullptr)
                setup_opacity_widget_vertcies(
                    Manager::instance()->getDefaultWidgetPoints(),
                    m_default_opacity_data,
                    m_opacity_vertices.size()
                );
            memcpy(m_opacity_vertices.back(), m_default_opacity_data, 18 * sizeof(float));

            //create a dynamic vertex buffer
            m_opacity_graphs.back()->createDynamicVertexBuffer(device, 18);
            m_opacity_graphs.back()->updateVertexBuffer(context, m_default_opacity_data);

            CD3D11_BUFFER_DESC pixconstBufferDesc(sizeof(dvr::ColorConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);
            dvr::ColorConstantBuffer tdata;
            tdata.u_color = { 0.678f, 0.839f, 0.969f, 0.5f };
            D3D11_SUBRESOURCE_DATA color_resource;
            color_resource.pSysMem = &tdata;
            m_opacity_graphs.back()->createPixelConstantBuffer(device, pixconstBufferDesc, &color_resource);
        }
    }
    Manager::instance()->resetDirtyOpacityId();
}
void dataBoard::setup_opacity_widget_vertcies(float* in_data, float*& out_data, int pos) {
    if (in_data == nullptr)in_data = Manager::instance()->getDefaultWidgetPoints();

    DirectX::XMFLOAT3 mscale = { 0.99f, 0.7f, 1.0f };
    DirectX::XMFLOAT3 moff = { .0f, -0.15f, .001f* pos };

    if(out_data == nullptr) out_data = new float[18];
    for (int i = 0; i < 6; i++) {
        out_data[3 * i] = (in_data[2 * i]-0.5f) * mscale.x + moff.x;
        out_data[3 * i + 1] = (in_data[2 * i+1] - 0.5f)* mscale.y + moff.y;
        out_data[3 * i + 2] = moff.z;
    }
}
bool dataBoard::Draw(ID3D11DeviceContext* context, DirectX::XMMATRIX model_mat) {
    bool render_complete = true;

    render_complete &= m_board_quad->Draw(context, model_mat);
    //color bar texture
    context->CSSetShader(m_color_comp_shader, nullptr, 0);
    ID3D11ShaderResourceView* texview = m_color_tex->GetTextureView();
    context->CSSetShaderResources(0, 1, &texview);
    context->CSSetUnorderedAccessViews(0, 1, &m_textureUAV, nullptr);

    volumeSetupConstBuffer vol_setup;
    Manager::instance()->getVolumeSetupConstData(vol_setup);

    if (m_compute_constbuff != nullptr) {
        // Prepare the constant buffer to send it to the graphics device.
        context->UpdateSubresource(
            m_compute_constbuff,
            0,
            nullptr,
            &vol_setup,
            0,
            0
        );
        context->CSSetConstantBuffers(0, 1, &m_compute_constbuff);
    }

    context->Dispatch((256 + 7) / 8, 1, 1);
    context->CopyResource(m_color_tex->GetTexture2D(), m_comp_tex);

    //unbind UAV
    ID3D11UnorderedAccessView* nullUAV[] = { NULL };
    context->CSSetUnorderedAccessViews(0, 1, nullUAV, 0);
    // Disable Compute Shader
    context->CSSetShader(nullptr, nullptr, 0);
    render_complete &= m_color_bar->Draw(context, model_mat);

    auto visibles = Manager::instance()->getOpacityWidgetVisibility();
    for (int i = 0; i < visibles->size(); i++) {
        if (!visibles->at(i)) continue;
        render_complete &= m_opacity_graphs[i]->Draw(context, model_mat);
    }

    //context->OMSetDepthStencilState(nullptr, 0);
    context->OMSetBlendState(nullptr, 0, 0xffffffff);
    return render_complete;
}