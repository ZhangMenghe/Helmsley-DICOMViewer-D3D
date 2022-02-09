#include "uiController.h"
#include <Common/Manager.h>
#include <vrController.h>

void uiController::InitAll(){
    AddTuneParams();
    InitAllTuneParam();
    InitCheckParam();
    setRenderingMethod(0);
    setMaskBits(7,8);
    setColorScheme(0);
    setAnnotationMixedRate(0.8f);
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
    //debug
    //vrController::instance()->switchCuttingPlane(dvr::CUT_TRAVERSAL);
}

void uiController::AddTuneParams(){
    float opa_values[5] = {
        1.0f,
         .0f,
        2.0f,
        0.0f,
        1.0f
    };
    Manager::instance()->addOpacityWidget(opa_values, 5);
    Manager::instance()->setOpacityWidgetId(0);
}

void uiController::InitAllTuneParam(){
    float contrast_values[4] = {
        .0f,
        1.0f,
        0.5f,
        0.65f
    };
    Manager::instance()->setRenderParam(contrast_values);
}

void uiController::InitCheckParam(){
    std::vector<std::string> keys{
        "Raycasting",
        "Overlays",
        "Apply CLAHE",
        "Cutting",
        "Freeze Volume",
        "Show Plane",
        "Real Value",
        "Traversal",
        "Traversal View",
        "Apply",
        "Recolor",
        "Volume",
        "Mesh",
        "Wireframe",
        "Center Line",
        "AR Enabled",
        "Use ARCore",
        "Points",
        "Planes",
        "3D Pointer"
    };
    std::vector<bool> values{
        false, //"Raycasting",
        true, //"Overlays",//android is false
        false, //"Apply CLAHE",

        //cutting
        false, //"Cutting",
        false, //"Freeze Volume",
        false, //"Show Plane",
        false, //"Real Value",
        false, //"Traversal",
        false, //"Traversal View",
        
        //mask
        false, //"Apply",
        true, //"Recolor",
        true, //"Volume",
        false, //"Mesh",
        false, //"Wireframe",
        false, //"Center Line",

        //AR
        false, //"AR Enabled",
        false, //"Use ARCore",
        true, //"Points",
        true, //"Planes",
        false, //"3D Pointer"
    };
    Manager::instance()->InitCheckParams(keys, values);
}

void uiController::setMaskBits(int num, unsigned int mbits){
    Manager::instance()->setMask(num, mbits);
    vrController::instance()->setMask(num, mbits);
}
void uiController::setCheck(std::string key, bool value){
    Manager::instance()->setCheck(key, value);
}
void uiController::addTuneParams(float* values, int num){
    Manager::instance()->addOpacityWidget(values, num);
}
void uiController::removeTuneWidgetById(int wid){
    Manager::instance()->removeOpacityWidget(wid);
}
void uiController::removeAllTuneWidget(){
    Manager::instance()->removeAllOpacityWidgets();
}
void uiController::setTuneParamById(int tid, int pid, float value){
    if (tid == dvr::TID_OPACITY) {
        if (pid < dvr::TUNE_END)Manager::instance()->setOpacityValue(pid, value);
    }
    else if(tid == dvr::TID_CONTRAST) Manager::instance()->setRenderParam(pid, value);
    else {
        float tmp[] = { value };
        vrController::instance()->setRenderingParameters(dvr::RENDER_METHOD(tid - 2), tmp);
    }
}
void uiController::setAllTuneParamById(int tid, std::vector<float> values){
    if (tid >= dvr::TID_END) return;

    if(tid == dvr::TID_CONTRAST)
        Manager::instance()->setRenderParam(&values[0]);
    else if(tid == dvr::TID_CUTTING_PLANE)
        vrController::instance()->setCuttingPlane(glm::vec3(values[0], values[1], values[2]), glm::vec3(values[3], values[4],values[5]));
    else
        vrController::instance()->setRenderingParameters(dvr::RENDER_METHOD(tid - 2), values.data());
}
void uiController::setTuneWidgetVisibility(int wid, bool visibility){
    Manager::instance()->setOpacityWidgetVisibility(wid, visibility);
}
void uiController::setTuneWidgetById(int id){
    Manager::instance()->setOpacityWidgetId(id);
}
void uiController::setCuttingPlane(int id, float value){
    if(id<0)vrController::instance()->setCuttingPlane(value);
    else vrController::instance()->setCuttingPlane(id, value);
}
void uiController::setColorScheme(int id){
    Manager::instance()->setColorScheme(id);
}
void uiController::setAnnotationMixedRate(float ratio) {
    Manager::instance()->setAnnotationMixedRate(ratio);
}
void uiController::setRenderingMethod(int id) {
    vrController::instance()->setRenderingMethod(dvr::RENDER_METHOD(id));
}