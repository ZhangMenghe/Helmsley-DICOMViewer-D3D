#include "pch.h"
#include "dicomLoader.h"
#include <vrController.h>
#include <cstring> //memset
#include <codecvt>
#include <locale>
#include <string>
#include <sstream> 
void dicomLoader::sendDataPrepare(int height, int width, int dims, float sh, float sw, float sd, bool b_wmask){
    CHANEL_NUM = b_wmask? 4:2;
    g_img_h = height; g_img_w = width; g_img_d = dims;
    g_ssize = CHANEL_NUM * width * height;
    g_vol_len = g_ssize* dims;
    g_vol_h=sh; g_vol_w=sw; g_vol_depth=sd;
    if(g_VolumeTexData!= nullptr){delete[]g_VolumeTexData; g_VolumeTexData = nullptr;}
    g_VolumeTexData = new UCHAR[g_vol_len];
    memset(g_VolumeTexData, 0x00, g_vol_len * sizeof(UCHAR));
}
bool dicomLoader::loadData(std::string filename, int h, int w, int d){
    if(g_maskTexData!=nullptr){delete[]g_maskTexData;g_maskTexData=nullptr;}
    g_maskTexData = new UCHAR[h*w*d];

    char buffer[1024];

    std::ifstream inFile("Assets/" + filename, std::ios::in | std::ios::binary);

    if(!inFile.is_open()) 
        return false;

    auto offset = 0;

    for(int id = 0; !inFile.eof(); id++){
        inFile.read(buffer, 1024);
        std::streamsize len = inFile.gcount();
        if(len == 0) continue;

        UCHAR* tb = g_maskTexData+offset;
        memcpy(tb, buffer, len);
        offset+=len;
    }
    return true;
}
bool dicomLoader::loadData(std::string dicom_path, std::string mask_path, int data_unit_size, int mask_unit_size){
    return (loadData(dicom_path, LOAD_DICOM, data_unit_size)
    && loadData(mask_path, LOAD_MASK, mask_unit_size));
}

bool dicomLoader::loadData(std::string filename, mLoadTarget target, int unit_size){
    char buffer[1024];
    //#ifdef RESOURCE_DESKTOP_DIR
    //std::ifstream inFile (PATH(filename), std::ios::in | std::ios::binary);
    //#else
    std::ifstream inFile ("Assets/" + filename, std::ios::in | std::ios::binary);
    //#endif

    if(!inFile.is_open()) 
        return false;
    
    for(int id = 0; !inFile.eof(); id++){
        inFile.read(buffer, 1024);
        std::streamsize len = inFile.gcount();
        if(len == 0) continue;
        send_dicom_data(target, id, len, unit_size, buffer);
    }
    n_data_offset[(int)target] = 0;
    return true;
}
bool dicomLoader::setupCenterLineData(vrController* controller, std::string filename){
    //std::vector<int>ids;
    //std::vector<float*> cline_data;

    int cidx = 0;
    float* data = nullptr;
    std::ifstream inFile("Assets/" + filename, std::ios::in);
    if (!inFile.is_open())
        return false;

    std::string line = "", substr;
    int idx;
    while (getline(inFile, line)) {
        if (line.length() < 3) {
            //cline_data.push_back(new float[4000 * 3]);
            //data = cline_data.back();
            data = new float[4000 * 3];
            centerline_map[std::stoi(line)] = data;
            idx = 0;
            continue;
        }
        std::stringstream ss(line);
        while (ss.good()) {
            getline(ss, substr, ',');
            if(data != nullptr)data[idx++] = std::stof(substr);
        }
    }
    inFile.close();
    return true;
}

void dicomLoader::send_dicom_data(mLoadTarget target, int id, int chunk_size, int unit_size, const char* data){
    //check initialization
    if(!g_VolumeTexData) return;
    UCHAR* buffer = g_VolumeTexData+n_data_offset[(int)target];
    if(chunk_size !=0 && unit_size == 4) memcpy(buffer, data, chunk_size);
    else{
        int num = (chunk_size==0)? (g_img_h*g_img_w) : chunk_size / unit_size;
        if(target == LOAD_DICOM){
            for(auto idx = 0; idx<num; idx++){
                buffer[CHANEL_NUM* idx] = UCHAR(data[2*idx]);
                buffer[CHANEL_NUM* idx + 1] = UCHAR(data[2*idx+1]);
            }
        }else{
            for(auto idx = 0; idx<num; idx++){
                buffer[CHANEL_NUM* idx + 2] = UCHAR(data[2*idx]);
                buffer[CHANEL_NUM* idx + 3] = UCHAR(data[2*idx+1]);
            }
        }
    }
   n_data_offset[target] += CHANEL_NUM / unit_size * chunk_size;   
}
void dicomLoader::sendDataFloats(int target, int chunk_size, std::vector<float> data) {
    float* cdata = new float[chunk_size - 1];
    memcpy(cdata, &data[1], (chunk_size - 1) * sizeof(float));
    centerline_map[(int)data[0]] = cdata;
}
void dicomLoader::sendDataDone(){
    for (int i = 0; i < 3; i++) {
        if (n_data_offset[i] != 0) {
            vrController::instance()->assembleTexture(i, g_img_h, g_img_w, g_img_d, g_vol_h, g_vol_w, g_vol_depth, g_VolumeTexData, CHANEL_NUM);
            n_data_offset[i] = 0;
            break;
        }
    }
    if (!centerline_map.empty()) {
        for (auto inst : centerline_map) {
            vrController::instance()->setupCenterLine(inst.first, inst.second);
            delete inst.second;
            inst.second = nullptr;
        }
        centerline_map.clear();
    }
}
