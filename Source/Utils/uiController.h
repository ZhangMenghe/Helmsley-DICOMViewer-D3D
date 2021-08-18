#ifndef UTILS_UICONTROLLER_H
#define UTILS_UICONTROLLER_H
#include <string>
#include <vector>
/// <summary>
/// JUIInterface for application parameters
/// </summary>
class uiController{
public:
    void AddTuneParams();
    void InitAll();
    void InitAllTuneParam();

    void InitCheckParam();

    void setMaskBits(int num, unsigned int mbits);
    void setCheck(std::string key, bool value);

    void addTuneParams(float* values, int num);
    void removeTuneWidgetById(int id);
    void removeAllTuneWidget();
    void setTuneParamById(int tid, int pid, float value);
    void setAllTuneParamById(int tid, std::vector<float> values);
    void setTuneWidgetVisibility(int wid, bool visibility);
    void setTuneWidgetById(int id);
    void setCuttingPlane(int id, float value);
    void setColorScheme(int id);
    void setRenderingMethod(int id);
};
#endif