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
//#include <utils/uiController.h>
#include <utils/dicomLoader.h>
//#include <Manager.h>
#include <vrController.h>
#include <vector>

template<class T>
using RPCVector = google::protobuf::RepeatedPtrField<T>;
using helmsley::datasetResponse;
using helmsley::volumeResponse;
//todo: try to implement operation merge

#define CLIENT_ID 5

class rpcHandler{
private:
    std::string DATA_PATH = "dicom-data/";
    std::unique_ptr<helmsley::inspectorSync::Stub> syncer_;
    std::unique_ptr<helmsley::dataTransfer::Stub> stub_;
	  Request req;
	  helmsley::FrameUpdateMsg update_msg;

    //uiController* ui_ = nullptr;
    //Manager* manager_ = nullptr;
    vrController* vr_ = nullptr;
    dicomLoader* loader_ = nullptr;

    helmsley::FrameUpdateMsg getUpdates();
    /*void tackle_gesture_msg(const RPCVector<helmsley::GestureOp> ops);
    void tack_tune_msg(helmsley::TuneMsg msg);*/
    void tackle_volume_msg(helmsley::volumeConcise msg);
    /*void tackle_reset_msg(helmsley::ResetMsg msg);*/

    std::vector<datasetResponse::datasetInfo> availableRemoteDatasets;
    std::vector<datasetResponse::datasetInfo> availableLocalDatasets;


public:
    rpcHandler(const std::string& host);
    ~rpcHandler();
    const RPCVector<helmsley::GestureOp> getOperations();
    
    /*void setUIController(uiController* ui){ui_ = ui;}
    void setManager(Manager* manager){manager_ = manager;}*/
    void setLoader(dicomLoader* loader) { loader_ = loader; }
    void setVRController(vrController* vr){vr_ = vr;}
    void setDataLoader(dicomLoader* loader){loader_ = loader;}
    void setDataPath(std::string path){DATA_PATH = path;}

    void Run();

    //void getAvailableConfigs();
    //void exportConfigs();
    std::vector<datasetResponse::datasetInfo> getAvailableDatasets(bool isLocal);
    std::vector<volumeResponse::volumeInfo> getVolumeFromDataset(const std::string & dataset_name, bool isLocal);
    bool Download(const std::string & ds_name, volumeResponse::volumeInfo target_volume);
    void DownloadVolume(const std::string & folder_path);
    void DownloadMasks(const std::string & folder_name);
    void DownloadMasksVolume();
    void DownloadCenterLineData();

    datasetResponse::datasetInfo target_ds;
};
#endif