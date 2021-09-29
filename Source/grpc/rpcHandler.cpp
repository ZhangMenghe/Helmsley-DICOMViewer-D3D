#include "rpcHandler.h"
#include <ppltasks.h> // For create_task
#include <glm/gtc/type_ptr.hpp>
#include <Utils/dataManager.h>
using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::ClientReaderWriter;
using grpc::ClientWriter;
using grpc::Status;
using namespace helmsley;
using namespace std;
using namespace concurrency;

bool rpcHandler::new_data_request = false;
bool rpcHandler::G_JOIN_SYNC = false, rpcHandler::G_STATUS_SENDER=false, rpcHandler::G_FORCED_STOP_BROADCAST=false;

rpcHandler::rpcHandler(const std::string& host) {
    auto channel = grpc::CreateChannel(host, grpc::InsecureChannelCredentials());
    syncer_ = inspectorSync::NewStub(channel);
    stub_ = dataTransfer::NewStub(channel);

    req.set_client_id(CLIENT_ID);
    m_gesture_req.set_client_id(CLIENT_ID);
    m_volume_pose_req.set_client_id(CLIENT_ID);
}

FrameUpdateMsg rpcHandler::getUpdates() {
    ClientContext context;
    syncer_->getUpdates(&context, req, &update_msg);
    return update_msg;
}
const RPCVector<GestureOp> rpcHandler::getOperations() {
    ClientContext context;
    std::vector<GestureOp> op_pool;
    OperationBatch op_batch;
    syncer_->getOperations(&context, req, &op_batch);
    return op_batch.gesture_op();
}
void rpcHandler::receiver_register() {
    commonResponse resp;
    ClientContext context;
    syncer_->startReceiveBroadcast(&context, req, &resp);
    G_JOIN_SYNC = true;
}
void rpcHandler::onBroadCastChanged() {
    G_STATUS_SENDER = !G_STATUS_SENDER;
    
    ClientContext context;
    if (G_STATUS_SENDER) syncer_->startBroadcast(&context, req, &m_resp);
    else syncer_->startReceiveBroadcast(&context, req, &m_resp);
    
    m_condition_variable.notify_all();
}
void rpcHandler::setGestureOp(GestureOp::OPType type, float x, float y) {
    ClientContext context;
    m_gesture_req.set_gid((m_gid++)%10000);
    m_gesture_req.set_type(type);
    m_gesture_req.set_x(x); m_gesture_req.set_y(y);
    syncer_->setGestureOp(&context, m_gesture_req, &m_resp);
}
void rpcHandler::setVolumePose(helmsley::VPMsg::VPType type, float* values) {
    ClientContext context;
    m_volume_pose_req.set_gid((m_gid++) % 10000);
    m_volume_pose_req.set_volume_pose_type(type);
    int v_num = (type == helmsley::VPMsg::VPType::VPMsg_VPType_ROT) ? 16 : 3;
    for(int i=0; i<v_num; i++)
        m_volume_pose_req.add_values(values[i]);

    syncer_->setVolumePose(&context, m_volume_pose_req, &m_resp);
}
void rpcHandler::Run() {
    while (true) {
        if (G_STATUS_SENDER) {
            std::unique_lock<std::mutex> lk(m_cv_mutex);
            auto now = std::chrono::system_clock::now();
            if (m_condition_variable.wait_until(lk, now + 100ms, []() {return G_STATUS_SENDER; })) {
                //OutputDebugString(L"====GET NOTIFIED OF FALSE====\n ");
                StatusMsg smsg;
                ClientContext context;
                syncer_->getStatusMessage(&context, req, &smsg);
                if (smsg.host_id() != CLIENT_ID) { G_STATUS_SENDER = false; G_FORCED_STOP_BROADCAST = true; }
            }
        }
        //OutputDebugString(L"====GET UPDATES====\n ");
        if (ui_ == nullptr || manager_ == nullptr || vr_ == nullptr || m_dicom_loader == nullptr)
            continue;
        //debug only: start to listen directly
        if (!initialized) {
            receiver_register();
            initialized = true;
        }

        auto msg = getUpdates();
        int gid = 0, tid = 0, cid = 0;
        bool gesture_finished = false;

        for (auto type : msg.types()) {
            switch (type) {
            case FrameUpdateMsg_MsgType_GESTURE:
                if (!gesture_finished) {
                    tackle_gesture_msg(msg.gestures());
                    gesture_finished = true;
                }
                break;
            case FrameUpdateMsg_MsgType_TUNE:
                tack_tune_msg(msg.tunes().Get(tid++));
                break;
            case FrameUpdateMsg_MsgType_CHECK:
                tack_check_msg(msg.checks().Get(cid++));
                break;
            case FrameUpdateMsg_MsgType_MASK:
                tack_mask_msg(msg.mask_value());
                break;
            case FrameUpdateMsg_MsgType_RESET:
                tackle_reset_msg(msg.reset_value());
                break;
            case FrameUpdateMsg_MsgType_DATA:
                tackle_volume_msg(msg.data_value());
                break;
            default:
                std::cout << "UNKNOWN TYPE" << std::endl;
                break;
            }
        }
    }
}

void rpcHandler::getRemoteDatasets(std::vector<datasetResponse::datasetInfo>& datasets) {
    datasetResponse response;
    ClientContext context;
    stub_->getAvailableDatasets(&context, req, &response);
    datasets.clear();
    datasets.reserve(response.datasets_size());
    for (datasetResponse::datasetInfo ds : response.datasets())
        datasets.push_back(ds);
}

void rpcHandler::getVolumeFromDataset(const std::string& dataset_name, std::vector<helmsley::volumeInfo>& ret) {
    ret.clear();
    Request req;
    req.set_client_id(CLIENT_ID);
    req.set_req_msg(dataset_name);

    volumeResponse volume;
    ClientContext context;

    std::unique_ptr<ClientReader<volumeResponse>> volume_reader(
        stub_->getVolumeFromDataset(&context, req));
    while (volume_reader->Read(&volume)){
        std::cout << volume.volumes_size() << std::endl;

        for (auto vol : volume.volumes()){
            ret.push_back(vol);
        }
    }
    Status status = volume_reader->Finish();
    winrt::check_hresult(status.ok());
}

std::vector<configResponse::configInfo> rpcHandler::getAvailableConfigFiles(){
    std::vector<configResponse::configInfo> available_config_files;

    configResponse response;
    ClientContext context;

    stub_->getAvailableConfigs(&context, req, &response);

    for (configResponse::configInfo config : response.configs()){
        available_config_files.push_back(config);
    }

    return available_config_files;
}
void rpcHandler::exportConfigs(std::string content){
    if (content.empty()) return;

    ClientContext context;
    commonResponse response;
    Request creq;
    creq.set_client_id(CLIENT_ID);
    creq.set_req_msg(content);
    stub_->exportConfigs(&context, creq, &response);
}

void rpcHandler::DownloadVolume(const string& folder_path){
    RequestWholeVolume req;
    req.set_client_id(CLIENT_ID);
    req.set_req_msg(folder_path);
    req.set_unit_size(2);

    ClientContext context;
    volumeWholeResponse resData;

    std::unique_ptr<ClientReader<volumeWholeResponse>> data_reader(
        stub_->DownloadVolume(&context, req));

    int id = 0;
    while (data_reader->Read(&resData)){
        m_dicom_loader->send_dicom_data(LOAD_DICOM, id, resData.data().length(), 2, resData.data().c_str());
        id++;
    };

    Status status = data_reader->Finish();
    winrt::check_hresult(status.ok());
}
concurrency::task<void> rpcHandler::DownloadVolumeAsync(const std::string& folder_path) {
    using namespace concurrency;
    //return create_task([]() {return; });
    concurrency::task<void> Action = create_task([folder_path, this]() {
        RequestWholeVolume req;
        req.set_client_id(CLIENT_ID);
        req.set_req_msg(folder_path);
        req.set_unit_size(2);

        ClientContext context;
        volumeWholeResponse resData;

        std::unique_ptr<ClientReader<volumeWholeResponse>> data_reader(
            stub_->DownloadVolume(&context, req));

        int id = 0;
        while (data_reader->Read(&resData)){
            m_dicom_loader->send_dicom_data(LOAD_DICOM, id, resData.data().length(), 2, resData.data().c_str());
            id++;
        };
        Status status = data_reader->Finish();
        winrt::check_hresult(status.ok());
    });
    return create_task(Action);
}
concurrency::task<void> rpcHandler::DownloadMasksAndCenterlinesAsync(const std::string& folder_path) {
    using namespace Concurrency;
    //return create_task([]() {return; });
    concurrency::task<void> Action = create_task([folder_path, this]() {
        Request req;
        req.set_client_id(CLIENT_ID);
        req.set_req_msg(folder_path);

        ClientContext context;
        volumeWholeResponse resData;
        std::unique_ptr<ClientReader<volumeWholeResponse>> data_reader(
            stub_->DownloadMasksVolume(&context, req));

        int id = 0;
        while (data_reader->Read(&resData)){
            m_dicom_loader->send_dicom_data(LOAD_MASK, id, resData.data().length(), 2, resData.data().c_str());
            id++;
        };
        Status status = data_reader->Finish();
        winrt::check_hresult(status.ok());
        DownloadCenterlines(req);
        //todo: TRIGGER FAULT!!!!
        ////center line
        //centerlineData clData;
        //std::unique_ptr<ClientReader<centerlineData>> cl_reader(
        //    stub_->DownloadCenterLineData(&context, req));
        //while (cl_reader->Read(&clData)) {
        //    m_dicom_loader->sendDataFloats(0, clData.data().size(), std::vector<float>(clData.data().begin(), clData.data().end()));
        //    //todo: save to local?
        //};
        //status = cl_reader->Finish();
        //winrt::check_hresult(status.ok());
    });
    return create_task(Action);
}
void rpcHandler::DownloadMasksAndCenterlines(const std::string& folder_name) {
    Request req;
    req.set_client_id(CLIENT_ID);
    req.set_req_msg(folder_name);

    ClientContext context;
    volumeWholeResponse resData;
    std::unique_ptr<ClientReader<volumeWholeResponse>> data_reader(
        stub_->DownloadMasksVolume(&context, req));

    int id = 0;
    while (data_reader->Read(&resData)){
        m_dicom_loader->send_dicom_data(LOAD_MASK, id, resData.data().length(), 2, resData.data().c_str());
        id++;
    };
    Status status = data_reader->Finish();
    winrt::check_hresult(status.ok());
    DownloadCenterlines(req);
}
void rpcHandler::DownloadCenterlines(Request req){
    ClientContext context;
    centerlineData clData;
    std::unique_ptr<ClientReader<centerlineData>> cl_reader(
        stub_->DownloadCenterLineData(&context, req));
    while (cl_reader->Read(&clData)){
        m_dicom_loader->sendDataFloats(0, clData.data().size(), std::vector<float>(clData.data().begin(), clData.data().end()));
    };
    Status status = cl_reader->Finish();
    winrt::check_hresult(status.ok());
}

////////////////////////////
////////Inspectator/////////
///////////////////////////
void rpcHandler::tackle_gesture_msg(const RPCVector<helmsley::GestureOp> ops){
    for (auto op : ops){
        switch (op.type()){
        case GestureOp_OPType_TOUCH_DOWN:
            vr_->onSingleTouchDown(op.x(), op.y());
            // sp.notify();
            break;
        case GestureOp_OPType_TOUCH_UP:
            vr_->onTouchReleased();
            break;
        case GestureOp_OPType_TOUCH_MOVE:
            vr_->onTouchMove(op.x(), op.y());
            // sp.notify();
            break;
        case GestureOp_OPType_SCALE:
            vr_->onScale(op.x(), op.y());
            // sp.notify();
            break;
        case GestureOp_OPType_PAN:
            vr_->onPan(op.x(), op.y());
            // sp.notify();
            break;
        default:
            break;
        }
    }
}
void rpcHandler::tack_tune_msg(helmsley::TuneMsg msg) {
    google::protobuf::RepeatedField<float> f;
    switch (msg.type()) {
    case TuneMsg_TuneType_ADD_ONE:
        f = msg.values();
        ui_->addTuneParams(f.mutable_data(), f.size());
        break;
    case TuneMsg_TuneType_REMOVE_ONE:
        ui_->removeTuneWidgetById(msg.target());
        break;
    case TuneMsg_TuneType_REMOTE_ALL:
        ui_->removeAllTuneWidget();
        break;
    case TuneMsg_TuneType_SET_ONE:
        ui_->setTuneParamById(msg.target(), msg.sub_target(), msg.value());
        break;
    case TuneMsg_TuneType_SET_ALL:
        f = msg.values();
        ui_->setAllTuneParamById(msg.target(), std::vector<float>(f.begin(), f.end()));
        break;
    case TuneMsg_TuneType_SET_VISIBLE:
        ui_->setTuneWidgetVisibility(msg.target(), (msg.value() > 0) ? true : false);
        break;
    case TuneMsg_TuneType_SET_TARGET:
        if (msg.sub_target() == 0) ui_->setTuneWidgetById(msg.target());
        else if (msg.sub_target() == 1) vr_->switchCuttingPlane((dvr::PARAM_CUT_ID)msg.target());
        else if (msg.sub_target() == 2) Manager::setTraversalTargetId(msg.target());
        break;
    case TuneMsg_TuneType_CUT_PLANE:
        ui_->setCuttingPlane(msg.target(), msg.value());
        break;
    case TuneMsg_TuneType_COLOR_SCHEME:
        ui_->setColorScheme(msg.target());
        break;
    case TuneMsg_TuneType_RENDER_METHOD:
        ui_->setRenderingMethod(msg.target());
        break;
    default:
        break;
    }
}
void rpcHandler::tack_check_msg(helmsley::CheckMsg msg) {
    ui_->setCheck(msg.key(), msg.value());
}
void rpcHandler::tack_mask_msg(helmsley::MaskMsg msg) {
    ui_->setMaskBits(msg.num(), (unsigned int)msg.mbits());
}

void rpcHandler::tackle_volume_msg(helmsley::DataMsg msg) {
    m_req_data = msg;
    new_data_request = true;
}

void rpcHandler::tackle_reset_msg(helmsley::ResetMsg msg) {
    //manager_->onReset();

    //checks
    auto f = msg.check_keys();
    auto cvs = msg.check_values();
    manager_->InitCheckParams(std::vector<std::string>(f.begin(), f.end()), std::vector<bool>(cvs.begin(), cvs.end()));

    auto vps = msg.volume_pose();
    auto cps = msg.camera_pose();

    //glm::vec3 old_pos, old_scale;
    //vrController::instance()->getRPS(old_pos, old_scale);
    //TODO:RESOLVE THIS
    vrController::instance()->onReset(
       glm::vec3(vps[0], vps[1], vps[2]),
       glm::vec3(vps[3], vps[4], vps[5]),
       glm::make_mat4(vps.Mutable(6)),
       nullptr
        //new Camera(
        //    DirectX::XMFLOAT3(cps[0], cps[1], cps[2]),
        //    DirectX::XMFLOAT3(cps[3], cps[4], cps[5]),
        //    DirectX::XMFLOAT3(cps[6], cps[7], cps[8])
        //)
    );
}