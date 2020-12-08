#include "uiController.h"
#include <Common/Manager.h>
#include <vrController.h>
//#include <algorithm>
//#include <glm/gtc/type_ptr.hpp>

void uiController::InitAll(){
    AddTuneParams();
    InitAllTuneParam();
    InitCheckParam();
    setMaskBits(7,8);
    // setMaskBits(7,2+4+8+16+32+64);
    //vrController::instance()->onReset(
    //    glm::vec3(.0f),
    //    glm::vec3(1.0f),
    //    glm::mat4(1.0),
    //    new Camera(
    //            glm::vec3(.0f,.0f,3.0f),
    //            glm::vec3(.0,1.0f,.0f),
    //            glm::vec3(.0f,.0f,2.0f)
    //    ));
}

void uiController::AddTuneParams(){
    float opa_values[5] = {
        1.0f,
        .0f,
        2.0f,
        0.0f,
        1.0f
    };
    Manager::instance()->addOpacityWidget(5, opa_values);
    //overlayController::instance()->addWidget(std::vector<float>(opa_values, opa_values+5));
    //overlayController::instance()->setWidgetId(0);
}

void uiController::InitAllTuneParam(){
    float contrast_values[3] = {
        .0f,
        .8f,
        1.0f
    };
    Manager::instance()->setRenderParam(contrast_values);
}

void uiController::InitCheckParam(){
    const int pnum = 18;
    const char* keys[pnum] = {
        "Raycasting",
        "Overlays",
        "Cutting",
        "Freeze Volume",
        "Freeze Plane",
        "Cutting Plane Real Sample",
        "Center Line Travel",
        "Traversal View",
        "Apply",
        "Recolor",
        "Volume",
        "Mesh",
        "Wireframe",
        "Center Line",
        "Show",
        "AR Enabled",
        "Points",
        "Planes"
    };
    bool values[pnum] = {
    false, //"Raycasting",
    false, //"Overlays",

    //cutting
    false, //"Cutting",
    false, //"Freeze Volume",
    false, //"Freeze Plane",
    false, //"Cutting Plane Real Sample",
    false, //"Center Line Travel",
    false, //"Traversal View",
    
    //mask
    true, //"Apply",
    true, //"Recolor",
    false, //"Volume",
    false, //"Mesh",
    false, //"Wireframe",
    false, //"Center Line",
    
    //ar
    false, //"Show",
    false, //"AR Enabled",
    false, //"Points",
    false, //"Planes"
    };
    Manager::instance()->InitCheckParams(pnum, keys, values);
}

void uiController::setMaskBits(int num, unsigned int mbits){
    Manager::instance()->setMask(num, mbits);
}
void uiController::setCheck(std::string key, bool value){
    Manager::instance()->setCheck(key, value);
}
void uiController::addTuneParams(std::vector<float> values){
    //overlayController::instance()->addWidget(values);
}
void uiController::removeTuneWidgetById(int wid){
    //overlayController::instance()->removeWidget(wid);
}
void uiController::removeAllTuneWidget(){
    //overlayController::instance()->removeAll();
}
void uiController::setTuneParamById(int tid, int pid, float value){
    //if(tid == 0 && pid < dvr::TUNE_END)overlayController::instance()->setTuneParameter(pid, value);
    if(tid == 1) Manager::instance()->setRenderParam(pid, value);
}
void uiController::setAllTuneParamById(int id, std::vector<float> values){  
    if(id == 1)Manager::instance()->setRenderParam(&values[0]);
    //else if(id == 2)vrController::instance()->setCuttingPlane(glm::vec3(values[0], values[1], values[2]), glm::vec3(values[3], values[4],values[5]));
}
void uiController::setTuneWidgetVisibility(int wid, bool visibility){
    //overlayController::instance()->setWidgetsVisibility(wid, visibility);
    //Manager::baked_dirty_ = true;
}
void uiController::setTuneWidgetById(int id){
    //overlayController::instance()->setWidgetId(id);
}
void uiController::setCuttingPlane(int id, float value){
    //if(id<0)vrController::instance()->setCuttingPlane(value);
    //else vrController::instance()->setCuttingPlane(id, value);
}
void uiController::setColorScheme(int id){
    Manager::instance()->setColorScheme(id);
}
