#ifndef RPCHANDLER_H
#define RPCHANDLER_H

#include <string>
#include <vector>
#include <ppltasks.h>
#include <grpc/grpc.h>
#include <grpcpp/channel.h>
#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>

#include <proto/common.grpc.pb.h>
#include <proto/common.pb.h>
#include <proto/inspectorSync.grpc.pb.h>
#include <proto/inspectorSync.pb.h>
#include <proto/transManager.grpc.pb.h>
#include <proto/transManager.pb.h>
#include <utils/uiController.h>
#include <utils/dicomLoader.h>
#include <vrController.h>
#include <Common/Manager.h>

template <class T>
using RPCVector = google::protobuf::RepeatedPtrField<T>;
using helmsley::configResponse;
using helmsley::datasetResponse;
using helmsley::volumeResponse;

#define CLIENT_ID 6

class rpcHandler{
private:
    std::string host;
    std::shared_ptr<grpc::Channel> channel;

    std::string DATA_PATH = "dicom-data/";
    std::unique_ptr<helmsley::inspectorSync::Stub> syncer_;
    std::unique_ptr<helmsley::dataTransfer::Stub> stub_;
    helmsley::Request req;
    helmsley::FrameUpdateMsg update_msg;
    helmsley::DataMsg m_req_data;
    
    bool initialized = false;

    Manager* manager_ = nullptr;
    vrController* vr_ = nullptr;
    std::shared_ptr<dicomLoader> m_dicom_loader;
    uiController* ui_ = nullptr;

    std::vector<datasetResponse::datasetInfo> availableRemoteDatasets;
    std::vector<datasetResponse::datasetInfo> availableLocalDatasets;

    helmsley::FrameUpdateMsg getUpdates();

    void tackle_volume_msg(helmsley::DataMsg msg);
    void tackle_gesture_msg(const RPCVector<helmsley::GestureOp> ops);
    void tack_tune_msg(helmsley::TuneMsg msg);
    void tack_check_msg(helmsley::CheckMsg msg);
    void tack_mask_msg(helmsley::MaskMsg msg);
    void tackle_reset_msg(helmsley::ResetMsg msg);
    void receiver_register();
public:
    static bool new_data_request;
    rpcHandler(const std::string& host);
    const RPCVector<helmsley::GestureOp> getOperations();

    /*void setUIController(uiController* ui){ui_ = ui;}*/
    void setManager(Manager* manager) { manager_ = manager; }
    void setVRController(vrController* vr) { vr_ = vr; }
    void setDataLoader(const std::shared_ptr<dicomLoader>& loader) { m_dicom_loader = loader; }
    void setUIController(uiController* ui) { ui_ = ui; }
    void setDataPath(std::string path) { DATA_PATH = path; }
    void Run();

    void getVolumePose(glm::vec3& pos, glm::quat& rot, float& scale);
    void setVolumePose(glm::vec3& pos, glm::quat& rot, float& scale);

    void getRemoteDatasets(std::vector<datasetResponse::datasetInfo>& datasets);
    void getVolumeFromDataset(const std::string& dataset_name, std::vector<helmsley::volumeInfo>& ret);
    std::vector<configResponse::configInfo> getAvailableConfigFiles();
    void exportConfigs(std::string content);

    void DownloadVolume(const std::string& folder_path);
    Concurrency::task<void> DownloadVolumeAsync(const std::string& folder_path);
    Concurrency::task<void> DownloadMasksAndCenterlinesAsync(const std::string& folder_path);

    void DownloadMasksAndCenterlines(const std::string& folder_name);
    void DownloadCenterlines(helmsley::Request req);
    helmsley::DataMsg GetNewDataRequest() { return m_req_data; }
};
#endif