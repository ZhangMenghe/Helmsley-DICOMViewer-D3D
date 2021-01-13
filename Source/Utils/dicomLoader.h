#ifndef UTILS_DICOMLOADER_H
#define UTILS_DICOMLOADER_H
#include "pch.h"
#include <cstddef>
#include <string>
#include <fstream>

/// <summary>
/// JNIInterface for data loading stuff
/// </summary>

typedef enum{
    LOAD_DICOM = 0,
    LOAD_MASK,
    LOAD_BOTH
}mLoadTarget;

class vrController;
class dicomLoader{
public:
    void sendDataPrepare(int height, int width, int dims, float sh, float sw, float sd, bool b_wmask);
    bool loadData(std::string filename, mLoadTarget target, bool b_from_asset, int unit_size=2);
    bool loadData(std::string dicom_path, std::string mask_path, bool b_from_asset, int data_unit_size=2, int mask_unit_size=2);
    bool loadData(std::string dirpath, bool wmask, bool b_from_asset);

    bool setupCenterLineData(vrController* controller, std::string filename);
    bool saveData(std::string vlpath);

    //setter
    int getChannelNum(){return CHANEL_NUM;}
    UCHAR* getVolumeData(){return g_VolumeTexData;}
    void reset(){
        // delete[] g_VolumeTexData;
        // g_VolumeTexData = nullptr;
        // for(auto& offset:n_data_offset) offset = 0;
    }
    void send_dicom_data(mLoadTarget target, int id, int chunk_size, int unit_size, const char* data);
    void sendDataFloats(int target, int chunk_size, std::vector<float> data);
    void sendDataDone();

private:
    int CHANEL_NUM = 4;
    UCHAR* g_VolumeTexData = nullptr;
    int g_img_h=0, g_img_w=0, g_img_d=0;
    float g_vol_h, g_vol_w, g_vol_depth = 0;
    size_t g_ssize = 0, g_vol_len;
    size_t n_data_offset[3] = {0};
    std::unordered_map<int, float*> centerline_map;
};
#endif