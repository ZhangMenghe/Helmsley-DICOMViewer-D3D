#ifndef RPCHANDLER_H
#define RPCHANDLER_H

#include <string>
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
#include <vector>

template<class T>
using RPCVector = google::protobuf::RepeatedPtrField<T>;
using helmsley::datasetResponse;
using helmsley::volumeResponse;

#define CLIENT_ID 6

class rpcHandler{
private:
    std::string DATA_PATH = "dicom-data/";
    std::unique_ptr<helmsley::inspectorSync::Stub> syncer_;
    std::unique_ptr<helmsley::dataTransfer::Stub> stub_;
	Request req;
	helmsley::FrameUpdateMsg update_msg;
    bool initialized = false;

    Manager* manager_ = nullptr;
    vrController* vr_ = nullptr;
    dicomLoader* loader_ = nullptr;
    uiController* ui_ = nullptr;

    std::vector<datasetResponse::datasetInfo> availableRemoteDatasets;
    std::vector<datasetResponse::datasetInfo> availableLocalDatasets;
    
    helmsley::FrameUpdateMsg getUpdates();

    void tackle_volume_msg(helmsley::volumeConcise msg);
    void tackle_gesture_msg(const RPCVector<helmsley::GestureOp> ops);
    void tack_tune_msg(helmsley::TuneMsg msg);
    void tack_check_msg(helmsley::CheckMsg msg);
    void tack_mask_msg(helmsley::MaskMsg msg);
    void tackle_reset_msg(helmsley::ResetMsg msg);
    void receiver_register();
public:
    rpcHandler(const std::string& host);
    ~rpcHandler();
    const RPCVector<helmsley::GestureOp> getOperations();
    
    /*void setUIController(uiController* ui){ui_ = ui;}*/
    void setManager(Manager* manager){manager_ = manager;}
    void setVRController(vrController* vr){vr_ = vr;}
    void setDataLoader(dicomLoader* loader){loader_ = loader;}
    void setUIController(uiController* ui) { ui_ = ui; }
    void setDataPath(std::string path){DATA_PATH = path;}

    void Run();

    //void getAvailableConfigs();
    //void exportConfigs();
    std::vector<datasetResponse::datasetInfo> getAvailableDatasets(bool isLocal);
    std::vector<volumeResponse::volumeInfo> getVolumeFromDataset(const std::string & dataset_name, bool isLocal);
    void DownloadVolume(const std::string & folder_path);
    void DownloadMasksAndCenterlines(const std::string & folder_name);
    void DownloadCenterlines(Request req);

    datasetResponse::datasetInfo target_ds;
};
#endif