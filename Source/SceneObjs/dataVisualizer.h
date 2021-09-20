#ifndef DATA_VISUALIZER_H
#define DATA_VISUALIZER_H

#include <vector>
#include <unordered_map>
#include <Common/ConstantAndStruct.h>
#include <Renderers/quadRenderer.h>

class dataBoard{
public:
    //static dataBoard* instance();
    dataBoard(ID3D11Device* device);
    ~dataBoard();
    void onReset(ID3D11Device* device);
    void Update(ID3D11Device* device, ID3D11DeviceContext* context);
    bool Draw(ID3D11DeviceContext* context, DirectX::XMMATRIX, bool is_front);
    void onViewChange(int width, int height);
    //void addOpacityInstance(ID3D11Device* device);
    //void setOpacityValue();
    //getter
    //const float* getCurrentWidgetPoints(){return widget_points_[widget_id];}
    //void getWidgetFlatPoints(float*& data, int& num){
    //    //data = u_opacity_data_; num = widget_points_.size();
    //}
    //std::vector<bool> getWidgetVisibilities(){return widget_visibilities_;}

private:
    //static dataBoard* myPtr_;
    bool m_initialized = false;

    //background quad
    quadRenderer* m_board_quad;
    
    //colorbar
    quadRenderer* m_color_bar;
    float* m_color_vertices;
    
    //compute shader
    ID3D11ComputeShader* m_color_comp_shader;
    ID3D11Texture2D* m_comp_tex = nullptr;
    Texture* m_color_tex;
    ID3D11UnorderedAccessView* m_textureUAV;
    ID3D11Buffer* m_compute_constbuff = nullptr;

    //opacity polygons
    std::vector<quadRenderer*> m_opacity_graphs;
    std::vector<float*> m_opacity_vertices;
    //USHORT m_opacity_indices[12] = { 0,2,1,0,5,2,0,4,5,0,3,4 };
    USHORT m_opacity_indices[12] = { 
        0,1,2,
        0,2,5,
        0,5,4,
        0,4,3 };

    //rendering
    //ID3D11DepthStencilState* m_DepthStencilState;
    ID3D11BlendState* d3dBlendState;

    float* m_default_opacity_data = nullptr;
    void setup_opacity_widget_vertcies(float* in_data, float*& out_data, int pos);
};
#endif