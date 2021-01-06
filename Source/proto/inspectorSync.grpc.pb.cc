// Generated by the gRPC C++ plugin.
// If you make any local change, they will be lost.
// source: inspectorSync.proto

#include "inspectorSync.pb.h"
#include "inspectorSync.grpc.pb.h"

#include <functional>
#include <grpcpp/impl/codegen/async_stream.h>
#include <grpcpp/impl/codegen/async_unary_call.h>
#include <grpcpp/impl/codegen/channel_interface.h>
#include <grpcpp/impl/codegen/client_unary_call.h>
#include <grpcpp/impl/codegen/client_callback.h>
#include <grpcpp/impl/codegen/message_allocator.h>
#include <grpcpp/impl/codegen/method_handler.h>
#include <grpcpp/impl/codegen/rpc_service_method.h>
#include <grpcpp/impl/codegen/server_callback.h>
#include <grpcpp/impl/codegen/server_callback_handlers.h>
#include <grpcpp/impl/codegen/server_context.h>
#include <grpcpp/impl/codegen/service_type.h>
#include <grpcpp/impl/codegen/sync_stream.h>
namespace helmsley {

static const char* inspectorSync_method_names[] = {
  "/helmsley.inspectorSync/startBroadcast",
  "/helmsley.inspectorSync/startReceiveBroadcast",
  "/helmsley.inspectorSync/gsVolumePose",
  "/helmsley.inspectorSync/getOperations",
  "/helmsley.inspectorSync/getUpdates",
  "/helmsley.inspectorSync/reqestReset",
  "/helmsley.inspectorSync/setGestureOp",
  "/helmsley.inspectorSync/setTuneParams",
  "/helmsley.inspectorSync/setCheckParams",
  "/helmsley.inspectorSync/setMaskParams",
  "/helmsley.inspectorSync/setDisplayVolume",
};

std::unique_ptr< inspectorSync::Stub> inspectorSync::NewStub(const std::shared_ptr< ::grpc::ChannelInterface>& channel, const ::grpc::StubOptions& options) {
  (void)options;
  std::unique_ptr< inspectorSync::Stub> stub(new inspectorSync::Stub(channel));
  return stub;
}

inspectorSync::Stub::Stub(const std::shared_ptr< ::grpc::ChannelInterface>& channel)
  : channel_(channel), rpcmethod_startBroadcast_(inspectorSync_method_names[0], ::grpc::internal::RpcMethod::NORMAL_RPC, channel)
  , rpcmethod_startReceiveBroadcast_(inspectorSync_method_names[1], ::grpc::internal::RpcMethod::NORMAL_RPC, channel)
  , rpcmethod_gsVolumePose_(inspectorSync_method_names[2], ::grpc::internal::RpcMethod::NORMAL_RPC, channel)
  , rpcmethod_getOperations_(inspectorSync_method_names[3], ::grpc::internal::RpcMethod::NORMAL_RPC, channel)
  , rpcmethod_getUpdates_(inspectorSync_method_names[4], ::grpc::internal::RpcMethod::NORMAL_RPC, channel)
  , rpcmethod_reqestReset_(inspectorSync_method_names[5], ::grpc::internal::RpcMethod::NORMAL_RPC, channel)
  , rpcmethod_setGestureOp_(inspectorSync_method_names[6], ::grpc::internal::RpcMethod::NORMAL_RPC, channel)
  , rpcmethod_setTuneParams_(inspectorSync_method_names[7], ::grpc::internal::RpcMethod::NORMAL_RPC, channel)
  , rpcmethod_setCheckParams_(inspectorSync_method_names[8], ::grpc::internal::RpcMethod::NORMAL_RPC, channel)
  , rpcmethod_setMaskParams_(inspectorSync_method_names[9], ::grpc::internal::RpcMethod::NORMAL_RPC, channel)
  , rpcmethod_setDisplayVolume_(inspectorSync_method_names[10], ::grpc::internal::RpcMethod::NORMAL_RPC, channel)
  {}

::grpc::Status inspectorSync::Stub::startBroadcast(::grpc::ClientContext* context, const ::Request& request, ::commonResponse* response) {
  return ::grpc::internal::BlockingUnaryCall(channel_.get(), rpcmethod_startBroadcast_, context, request, response);
}

void inspectorSync::Stub::experimental_async::startBroadcast(::grpc::ClientContext* context, const ::Request* request, ::commonResponse* response, std::function<void(::grpc::Status)> f) {
  ::grpc::internal::CallbackUnaryCall(stub_->channel_.get(), stub_->rpcmethod_startBroadcast_, context, request, response, std::move(f));
}

void inspectorSync::Stub::experimental_async::startBroadcast(::grpc::ClientContext* context, const ::Request* request, ::commonResponse* response, ::grpc::experimental::ClientUnaryReactor* reactor) {
  ::grpc::internal::ClientCallbackUnaryFactory::Create(stub_->channel_.get(), stub_->rpcmethod_startBroadcast_, context, request, response, reactor);
}

::grpc::ClientAsyncResponseReader< ::commonResponse>* inspectorSync::Stub::PrepareAsyncstartBroadcastRaw(::grpc::ClientContext* context, const ::Request& request, ::grpc::CompletionQueue* cq) {
  return ::grpc::internal::ClientAsyncResponseReaderFactory< ::commonResponse>::Create(channel_.get(), cq, rpcmethod_startBroadcast_, context, request, false);
}

::grpc::ClientAsyncResponseReader< ::commonResponse>* inspectorSync::Stub::AsyncstartBroadcastRaw(::grpc::ClientContext* context, const ::Request& request, ::grpc::CompletionQueue* cq) {
  auto* result =
    this->PrepareAsyncstartBroadcastRaw(context, request, cq);
  result->StartCall();
  return result;
}

::grpc::Status inspectorSync::Stub::startReceiveBroadcast(::grpc::ClientContext* context, const ::Request& request, ::commonResponse* response) {
  return ::grpc::internal::BlockingUnaryCall(channel_.get(), rpcmethod_startReceiveBroadcast_, context, request, response);
}

void inspectorSync::Stub::experimental_async::startReceiveBroadcast(::grpc::ClientContext* context, const ::Request* request, ::commonResponse* response, std::function<void(::grpc::Status)> f) {
  ::grpc::internal::CallbackUnaryCall(stub_->channel_.get(), stub_->rpcmethod_startReceiveBroadcast_, context, request, response, std::move(f));
}

void inspectorSync::Stub::experimental_async::startReceiveBroadcast(::grpc::ClientContext* context, const ::Request* request, ::commonResponse* response, ::grpc::experimental::ClientUnaryReactor* reactor) {
  ::grpc::internal::ClientCallbackUnaryFactory::Create(stub_->channel_.get(), stub_->rpcmethod_startReceiveBroadcast_, context, request, response, reactor);
}

::grpc::ClientAsyncResponseReader< ::commonResponse>* inspectorSync::Stub::PrepareAsyncstartReceiveBroadcastRaw(::grpc::ClientContext* context, const ::Request& request, ::grpc::CompletionQueue* cq) {
  return ::grpc::internal::ClientAsyncResponseReaderFactory< ::commonResponse>::Create(channel_.get(), cq, rpcmethod_startReceiveBroadcast_, context, request, false);
}

::grpc::ClientAsyncResponseReader< ::commonResponse>* inspectorSync::Stub::AsyncstartReceiveBroadcastRaw(::grpc::ClientContext* context, const ::Request& request, ::grpc::CompletionQueue* cq) {
  auto* result =
    this->PrepareAsyncstartReceiveBroadcastRaw(context, request, cq);
  result->StartCall();
  return result;
}

::grpc::Status inspectorSync::Stub::gsVolumePose(::grpc::ClientContext* context, const ::helmsley::VPMsg& request, ::commonResponse* response) {
  return ::grpc::internal::BlockingUnaryCall(channel_.get(), rpcmethod_gsVolumePose_, context, request, response);
}

void inspectorSync::Stub::experimental_async::gsVolumePose(::grpc::ClientContext* context, const ::helmsley::VPMsg* request, ::commonResponse* response, std::function<void(::grpc::Status)> f) {
  ::grpc::internal::CallbackUnaryCall(stub_->channel_.get(), stub_->rpcmethod_gsVolumePose_, context, request, response, std::move(f));
}

void inspectorSync::Stub::experimental_async::gsVolumePose(::grpc::ClientContext* context, const ::helmsley::VPMsg* request, ::commonResponse* response, ::grpc::experimental::ClientUnaryReactor* reactor) {
  ::grpc::internal::ClientCallbackUnaryFactory::Create(stub_->channel_.get(), stub_->rpcmethod_gsVolumePose_, context, request, response, reactor);
}

::grpc::ClientAsyncResponseReader< ::commonResponse>* inspectorSync::Stub::PrepareAsyncgsVolumePoseRaw(::grpc::ClientContext* context, const ::helmsley::VPMsg& request, ::grpc::CompletionQueue* cq) {
  return ::grpc::internal::ClientAsyncResponseReaderFactory< ::commonResponse>::Create(channel_.get(), cq, rpcmethod_gsVolumePose_, context, request, false);
}

::grpc::ClientAsyncResponseReader< ::commonResponse>* inspectorSync::Stub::AsyncgsVolumePoseRaw(::grpc::ClientContext* context, const ::helmsley::VPMsg& request, ::grpc::CompletionQueue* cq) {
  auto* result =
    this->PrepareAsyncgsVolumePoseRaw(context, request, cq);
  result->StartCall();
  return result;
}

::grpc::Status inspectorSync::Stub::getOperations(::grpc::ClientContext* context, const ::Request& request, ::helmsley::OperationBatch* response) {
  return ::grpc::internal::BlockingUnaryCall(channel_.get(), rpcmethod_getOperations_, context, request, response);
}

void inspectorSync::Stub::experimental_async::getOperations(::grpc::ClientContext* context, const ::Request* request, ::helmsley::OperationBatch* response, std::function<void(::grpc::Status)> f) {
  ::grpc::internal::CallbackUnaryCall(stub_->channel_.get(), stub_->rpcmethod_getOperations_, context, request, response, std::move(f));
}

void inspectorSync::Stub::experimental_async::getOperations(::grpc::ClientContext* context, const ::Request* request, ::helmsley::OperationBatch* response, ::grpc::experimental::ClientUnaryReactor* reactor) {
  ::grpc::internal::ClientCallbackUnaryFactory::Create(stub_->channel_.get(), stub_->rpcmethod_getOperations_, context, request, response, reactor);
}

::grpc::ClientAsyncResponseReader< ::helmsley::OperationBatch>* inspectorSync::Stub::PrepareAsyncgetOperationsRaw(::grpc::ClientContext* context, const ::Request& request, ::grpc::CompletionQueue* cq) {
  return ::grpc::internal::ClientAsyncResponseReaderFactory< ::helmsley::OperationBatch>::Create(channel_.get(), cq, rpcmethod_getOperations_, context, request, false);
}

::grpc::ClientAsyncResponseReader< ::helmsley::OperationBatch>* inspectorSync::Stub::AsyncgetOperationsRaw(::grpc::ClientContext* context, const ::Request& request, ::grpc::CompletionQueue* cq) {
  auto* result =
    this->PrepareAsyncgetOperationsRaw(context, request, cq);
  result->StartCall();
  return result;
}

::grpc::Status inspectorSync::Stub::getUpdates(::grpc::ClientContext* context, const ::Request& request, ::helmsley::FrameUpdateMsg* response) {
  return ::grpc::internal::BlockingUnaryCall(channel_.get(), rpcmethod_getUpdates_, context, request, response);
}

void inspectorSync::Stub::experimental_async::getUpdates(::grpc::ClientContext* context, const ::Request* request, ::helmsley::FrameUpdateMsg* response, std::function<void(::grpc::Status)> f) {
  ::grpc::internal::CallbackUnaryCall(stub_->channel_.get(), stub_->rpcmethod_getUpdates_, context, request, response, std::move(f));
}

void inspectorSync::Stub::experimental_async::getUpdates(::grpc::ClientContext* context, const ::Request* request, ::helmsley::FrameUpdateMsg* response, ::grpc::experimental::ClientUnaryReactor* reactor) {
  ::grpc::internal::ClientCallbackUnaryFactory::Create(stub_->channel_.get(), stub_->rpcmethod_getUpdates_, context, request, response, reactor);
}

::grpc::ClientAsyncResponseReader< ::helmsley::FrameUpdateMsg>* inspectorSync::Stub::PrepareAsyncgetUpdatesRaw(::grpc::ClientContext* context, const ::Request& request, ::grpc::CompletionQueue* cq) {
  return ::grpc::internal::ClientAsyncResponseReaderFactory< ::helmsley::FrameUpdateMsg>::Create(channel_.get(), cq, rpcmethod_getUpdates_, context, request, false);
}

::grpc::ClientAsyncResponseReader< ::helmsley::FrameUpdateMsg>* inspectorSync::Stub::AsyncgetUpdatesRaw(::grpc::ClientContext* context, const ::Request& request, ::grpc::CompletionQueue* cq) {
  auto* result =
    this->PrepareAsyncgetUpdatesRaw(context, request, cq);
  result->StartCall();
  return result;
}

::grpc::Status inspectorSync::Stub::reqestReset(::grpc::ClientContext* context, const ::helmsley::ResetMsg& request, ::commonResponse* response) {
  return ::grpc::internal::BlockingUnaryCall(channel_.get(), rpcmethod_reqestReset_, context, request, response);
}

void inspectorSync::Stub::experimental_async::reqestReset(::grpc::ClientContext* context, const ::helmsley::ResetMsg* request, ::commonResponse* response, std::function<void(::grpc::Status)> f) {
  ::grpc::internal::CallbackUnaryCall(stub_->channel_.get(), stub_->rpcmethod_reqestReset_, context, request, response, std::move(f));
}

void inspectorSync::Stub::experimental_async::reqestReset(::grpc::ClientContext* context, const ::helmsley::ResetMsg* request, ::commonResponse* response, ::grpc::experimental::ClientUnaryReactor* reactor) {
  ::grpc::internal::ClientCallbackUnaryFactory::Create(stub_->channel_.get(), stub_->rpcmethod_reqestReset_, context, request, response, reactor);
}

::grpc::ClientAsyncResponseReader< ::commonResponse>* inspectorSync::Stub::PrepareAsyncreqestResetRaw(::grpc::ClientContext* context, const ::helmsley::ResetMsg& request, ::grpc::CompletionQueue* cq) {
  return ::grpc::internal::ClientAsyncResponseReaderFactory< ::commonResponse>::Create(channel_.get(), cq, rpcmethod_reqestReset_, context, request, false);
}

::grpc::ClientAsyncResponseReader< ::commonResponse>* inspectorSync::Stub::AsyncreqestResetRaw(::grpc::ClientContext* context, const ::helmsley::ResetMsg& request, ::grpc::CompletionQueue* cq) {
  auto* result =
    this->PrepareAsyncreqestResetRaw(context, request, cq);
  result->StartCall();
  return result;
}

::grpc::Status inspectorSync::Stub::setGestureOp(::grpc::ClientContext* context, const ::helmsley::GestureOp& request, ::commonResponse* response) {
  return ::grpc::internal::BlockingUnaryCall(channel_.get(), rpcmethod_setGestureOp_, context, request, response);
}

void inspectorSync::Stub::experimental_async::setGestureOp(::grpc::ClientContext* context, const ::helmsley::GestureOp* request, ::commonResponse* response, std::function<void(::grpc::Status)> f) {
  ::grpc::internal::CallbackUnaryCall(stub_->channel_.get(), stub_->rpcmethod_setGestureOp_, context, request, response, std::move(f));
}

void inspectorSync::Stub::experimental_async::setGestureOp(::grpc::ClientContext* context, const ::helmsley::GestureOp* request, ::commonResponse* response, ::grpc::experimental::ClientUnaryReactor* reactor) {
  ::grpc::internal::ClientCallbackUnaryFactory::Create(stub_->channel_.get(), stub_->rpcmethod_setGestureOp_, context, request, response, reactor);
}

::grpc::ClientAsyncResponseReader< ::commonResponse>* inspectorSync::Stub::PrepareAsyncsetGestureOpRaw(::grpc::ClientContext* context, const ::helmsley::GestureOp& request, ::grpc::CompletionQueue* cq) {
  return ::grpc::internal::ClientAsyncResponseReaderFactory< ::commonResponse>::Create(channel_.get(), cq, rpcmethod_setGestureOp_, context, request, false);
}

::grpc::ClientAsyncResponseReader< ::commonResponse>* inspectorSync::Stub::AsyncsetGestureOpRaw(::grpc::ClientContext* context, const ::helmsley::GestureOp& request, ::grpc::CompletionQueue* cq) {
  auto* result =
    this->PrepareAsyncsetGestureOpRaw(context, request, cq);
  result->StartCall();
  return result;
}

::grpc::Status inspectorSync::Stub::setTuneParams(::grpc::ClientContext* context, const ::helmsley::TuneMsg& request, ::commonResponse* response) {
  return ::grpc::internal::BlockingUnaryCall(channel_.get(), rpcmethod_setTuneParams_, context, request, response);
}

void inspectorSync::Stub::experimental_async::setTuneParams(::grpc::ClientContext* context, const ::helmsley::TuneMsg* request, ::commonResponse* response, std::function<void(::grpc::Status)> f) {
  ::grpc::internal::CallbackUnaryCall(stub_->channel_.get(), stub_->rpcmethod_setTuneParams_, context, request, response, std::move(f));
}

void inspectorSync::Stub::experimental_async::setTuneParams(::grpc::ClientContext* context, const ::helmsley::TuneMsg* request, ::commonResponse* response, ::grpc::experimental::ClientUnaryReactor* reactor) {
  ::grpc::internal::ClientCallbackUnaryFactory::Create(stub_->channel_.get(), stub_->rpcmethod_setTuneParams_, context, request, response, reactor);
}

::grpc::ClientAsyncResponseReader< ::commonResponse>* inspectorSync::Stub::PrepareAsyncsetTuneParamsRaw(::grpc::ClientContext* context, const ::helmsley::TuneMsg& request, ::grpc::CompletionQueue* cq) {
  return ::grpc::internal::ClientAsyncResponseReaderFactory< ::commonResponse>::Create(channel_.get(), cq, rpcmethod_setTuneParams_, context, request, false);
}

::grpc::ClientAsyncResponseReader< ::commonResponse>* inspectorSync::Stub::AsyncsetTuneParamsRaw(::grpc::ClientContext* context, const ::helmsley::TuneMsg& request, ::grpc::CompletionQueue* cq) {
  auto* result =
    this->PrepareAsyncsetTuneParamsRaw(context, request, cq);
  result->StartCall();
  return result;
}

::grpc::Status inspectorSync::Stub::setCheckParams(::grpc::ClientContext* context, const ::helmsley::CheckMsg& request, ::commonResponse* response) {
  return ::grpc::internal::BlockingUnaryCall(channel_.get(), rpcmethod_setCheckParams_, context, request, response);
}

void inspectorSync::Stub::experimental_async::setCheckParams(::grpc::ClientContext* context, const ::helmsley::CheckMsg* request, ::commonResponse* response, std::function<void(::grpc::Status)> f) {
  ::grpc::internal::CallbackUnaryCall(stub_->channel_.get(), stub_->rpcmethod_setCheckParams_, context, request, response, std::move(f));
}

void inspectorSync::Stub::experimental_async::setCheckParams(::grpc::ClientContext* context, const ::helmsley::CheckMsg* request, ::commonResponse* response, ::grpc::experimental::ClientUnaryReactor* reactor) {
  ::grpc::internal::ClientCallbackUnaryFactory::Create(stub_->channel_.get(), stub_->rpcmethod_setCheckParams_, context, request, response, reactor);
}

::grpc::ClientAsyncResponseReader< ::commonResponse>* inspectorSync::Stub::PrepareAsyncsetCheckParamsRaw(::grpc::ClientContext* context, const ::helmsley::CheckMsg& request, ::grpc::CompletionQueue* cq) {
  return ::grpc::internal::ClientAsyncResponseReaderFactory< ::commonResponse>::Create(channel_.get(), cq, rpcmethod_setCheckParams_, context, request, false);
}

::grpc::ClientAsyncResponseReader< ::commonResponse>* inspectorSync::Stub::AsyncsetCheckParamsRaw(::grpc::ClientContext* context, const ::helmsley::CheckMsg& request, ::grpc::CompletionQueue* cq) {
  auto* result =
    this->PrepareAsyncsetCheckParamsRaw(context, request, cq);
  result->StartCall();
  return result;
}

::grpc::Status inspectorSync::Stub::setMaskParams(::grpc::ClientContext* context, const ::helmsley::MaskMsg& request, ::commonResponse* response) {
  return ::grpc::internal::BlockingUnaryCall(channel_.get(), rpcmethod_setMaskParams_, context, request, response);
}

void inspectorSync::Stub::experimental_async::setMaskParams(::grpc::ClientContext* context, const ::helmsley::MaskMsg* request, ::commonResponse* response, std::function<void(::grpc::Status)> f) {
  ::grpc::internal::CallbackUnaryCall(stub_->channel_.get(), stub_->rpcmethod_setMaskParams_, context, request, response, std::move(f));
}

void inspectorSync::Stub::experimental_async::setMaskParams(::grpc::ClientContext* context, const ::helmsley::MaskMsg* request, ::commonResponse* response, ::grpc::experimental::ClientUnaryReactor* reactor) {
  ::grpc::internal::ClientCallbackUnaryFactory::Create(stub_->channel_.get(), stub_->rpcmethod_setMaskParams_, context, request, response, reactor);
}

::grpc::ClientAsyncResponseReader< ::commonResponse>* inspectorSync::Stub::PrepareAsyncsetMaskParamsRaw(::grpc::ClientContext* context, const ::helmsley::MaskMsg& request, ::grpc::CompletionQueue* cq) {
  return ::grpc::internal::ClientAsyncResponseReaderFactory< ::commonResponse>::Create(channel_.get(), cq, rpcmethod_setMaskParams_, context, request, false);
}

::grpc::ClientAsyncResponseReader< ::commonResponse>* inspectorSync::Stub::AsyncsetMaskParamsRaw(::grpc::ClientContext* context, const ::helmsley::MaskMsg& request, ::grpc::CompletionQueue* cq) {
  auto* result =
    this->PrepareAsyncsetMaskParamsRaw(context, request, cq);
  result->StartCall();
  return result;
}

::grpc::Status inspectorSync::Stub::setDisplayVolume(::grpc::ClientContext* context, const ::helmsley::volumeConcise& request, ::commonResponse* response) {
  return ::grpc::internal::BlockingUnaryCall(channel_.get(), rpcmethod_setDisplayVolume_, context, request, response);
}

void inspectorSync::Stub::experimental_async::setDisplayVolume(::grpc::ClientContext* context, const ::helmsley::volumeConcise* request, ::commonResponse* response, std::function<void(::grpc::Status)> f) {
  ::grpc::internal::CallbackUnaryCall(stub_->channel_.get(), stub_->rpcmethod_setDisplayVolume_, context, request, response, std::move(f));
}

void inspectorSync::Stub::experimental_async::setDisplayVolume(::grpc::ClientContext* context, const ::helmsley::volumeConcise* request, ::commonResponse* response, ::grpc::experimental::ClientUnaryReactor* reactor) {
  ::grpc::internal::ClientCallbackUnaryFactory::Create(stub_->channel_.get(), stub_->rpcmethod_setDisplayVolume_, context, request, response, reactor);
}

::grpc::ClientAsyncResponseReader< ::commonResponse>* inspectorSync::Stub::PrepareAsyncsetDisplayVolumeRaw(::grpc::ClientContext* context, const ::helmsley::volumeConcise& request, ::grpc::CompletionQueue* cq) {
  return ::grpc::internal::ClientAsyncResponseReaderFactory< ::commonResponse>::Create(channel_.get(), cq, rpcmethod_setDisplayVolume_, context, request, false);
}

::grpc::ClientAsyncResponseReader< ::commonResponse>* inspectorSync::Stub::AsyncsetDisplayVolumeRaw(::grpc::ClientContext* context, const ::helmsley::volumeConcise& request, ::grpc::CompletionQueue* cq) {
  auto* result =
    this->PrepareAsyncsetDisplayVolumeRaw(context, request, cq);
  result->StartCall();
  return result;
}

inspectorSync::Service::Service() {
  AddMethod(new ::grpc::internal::RpcServiceMethod(
      inspectorSync_method_names[0],
      ::grpc::internal::RpcMethod::NORMAL_RPC,
      new ::grpc::internal::RpcMethodHandler< inspectorSync::Service, ::Request, ::commonResponse>(
          [](inspectorSync::Service* service,
             ::grpc::ServerContext* ctx,
             const ::Request* req,
             ::commonResponse* resp) {
               return service->startBroadcast(ctx, req, resp);
             }, this)));
  AddMethod(new ::grpc::internal::RpcServiceMethod(
      inspectorSync_method_names[1],
      ::grpc::internal::RpcMethod::NORMAL_RPC,
      new ::grpc::internal::RpcMethodHandler< inspectorSync::Service, ::Request, ::commonResponse>(
          [](inspectorSync::Service* service,
             ::grpc::ServerContext* ctx,
             const ::Request* req,
             ::commonResponse* resp) {
               return service->startReceiveBroadcast(ctx, req, resp);
             }, this)));
  AddMethod(new ::grpc::internal::RpcServiceMethod(
      inspectorSync_method_names[2],
      ::grpc::internal::RpcMethod::NORMAL_RPC,
      new ::grpc::internal::RpcMethodHandler< inspectorSync::Service, ::helmsley::VPMsg, ::commonResponse>(
          [](inspectorSync::Service* service,
             ::grpc::ServerContext* ctx,
             const ::helmsley::VPMsg* req,
             ::commonResponse* resp) {
               return service->gsVolumePose(ctx, req, resp);
             }, this)));
  AddMethod(new ::grpc::internal::RpcServiceMethod(
      inspectorSync_method_names[3],
      ::grpc::internal::RpcMethod::NORMAL_RPC,
      new ::grpc::internal::RpcMethodHandler< inspectorSync::Service, ::Request, ::helmsley::OperationBatch>(
          [](inspectorSync::Service* service,
             ::grpc::ServerContext* ctx,
             const ::Request* req,
             ::helmsley::OperationBatch* resp) {
               return service->getOperations(ctx, req, resp);
             }, this)));
  AddMethod(new ::grpc::internal::RpcServiceMethod(
      inspectorSync_method_names[4],
      ::grpc::internal::RpcMethod::NORMAL_RPC,
      new ::grpc::internal::RpcMethodHandler< inspectorSync::Service, ::Request, ::helmsley::FrameUpdateMsg>(
          [](inspectorSync::Service* service,
             ::grpc::ServerContext* ctx,
             const ::Request* req,
             ::helmsley::FrameUpdateMsg* resp) {
               return service->getUpdates(ctx, req, resp);
             }, this)));
  AddMethod(new ::grpc::internal::RpcServiceMethod(
      inspectorSync_method_names[5],
      ::grpc::internal::RpcMethod::NORMAL_RPC,
      new ::grpc::internal::RpcMethodHandler< inspectorSync::Service, ::helmsley::ResetMsg, ::commonResponse>(
          [](inspectorSync::Service* service,
             ::grpc::ServerContext* ctx,
             const ::helmsley::ResetMsg* req,
             ::commonResponse* resp) {
               return service->reqestReset(ctx, req, resp);
             }, this)));
  AddMethod(new ::grpc::internal::RpcServiceMethod(
      inspectorSync_method_names[6],
      ::grpc::internal::RpcMethod::NORMAL_RPC,
      new ::grpc::internal::RpcMethodHandler< inspectorSync::Service, ::helmsley::GestureOp, ::commonResponse>(
          [](inspectorSync::Service* service,
             ::grpc::ServerContext* ctx,
             const ::helmsley::GestureOp* req,
             ::commonResponse* resp) {
               return service->setGestureOp(ctx, req, resp);
             }, this)));
  AddMethod(new ::grpc::internal::RpcServiceMethod(
      inspectorSync_method_names[7],
      ::grpc::internal::RpcMethod::NORMAL_RPC,
      new ::grpc::internal::RpcMethodHandler< inspectorSync::Service, ::helmsley::TuneMsg, ::commonResponse>(
          [](inspectorSync::Service* service,
             ::grpc::ServerContext* ctx,
             const ::helmsley::TuneMsg* req,
             ::commonResponse* resp) {
               return service->setTuneParams(ctx, req, resp);
             }, this)));
  AddMethod(new ::grpc::internal::RpcServiceMethod(
      inspectorSync_method_names[8],
      ::grpc::internal::RpcMethod::NORMAL_RPC,
      new ::grpc::internal::RpcMethodHandler< inspectorSync::Service, ::helmsley::CheckMsg, ::commonResponse>(
          [](inspectorSync::Service* service,
             ::grpc::ServerContext* ctx,
             const ::helmsley::CheckMsg* req,
             ::commonResponse* resp) {
               return service->setCheckParams(ctx, req, resp);
             }, this)));
  AddMethod(new ::grpc::internal::RpcServiceMethod(
      inspectorSync_method_names[9],
      ::grpc::internal::RpcMethod::NORMAL_RPC,
      new ::grpc::internal::RpcMethodHandler< inspectorSync::Service, ::helmsley::MaskMsg, ::commonResponse>(
          [](inspectorSync::Service* service,
             ::grpc::ServerContext* ctx,
             const ::helmsley::MaskMsg* req,
             ::commonResponse* resp) {
               return service->setMaskParams(ctx, req, resp);
             }, this)));
  AddMethod(new ::grpc::internal::RpcServiceMethod(
      inspectorSync_method_names[10],
      ::grpc::internal::RpcMethod::NORMAL_RPC,
      new ::grpc::internal::RpcMethodHandler< inspectorSync::Service, ::helmsley::volumeConcise, ::commonResponse>(
          [](inspectorSync::Service* service,
             ::grpc::ServerContext* ctx,
             const ::helmsley::volumeConcise* req,
             ::commonResponse* resp) {
               return service->setDisplayVolume(ctx, req, resp);
             }, this)));
}

inspectorSync::Service::~Service() {
}

::grpc::Status inspectorSync::Service::startBroadcast(::grpc::ServerContext* context, const ::Request* request, ::commonResponse* response) {
  (void) context;
  (void) request;
  (void) response;
  return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
}

::grpc::Status inspectorSync::Service::startReceiveBroadcast(::grpc::ServerContext* context, const ::Request* request, ::commonResponse* response) {
  (void) context;
  (void) request;
  (void) response;
  return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
}

::grpc::Status inspectorSync::Service::gsVolumePose(::grpc::ServerContext* context, const ::helmsley::VPMsg* request, ::commonResponse* response) {
  (void) context;
  (void) request;
  (void) response;
  return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
}

::grpc::Status inspectorSync::Service::getOperations(::grpc::ServerContext* context, const ::Request* request, ::helmsley::OperationBatch* response) {
  (void) context;
  (void) request;
  (void) response;
  return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
}

::grpc::Status inspectorSync::Service::getUpdates(::grpc::ServerContext* context, const ::Request* request, ::helmsley::FrameUpdateMsg* response) {
  (void) context;
  (void) request;
  (void) response;
  return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
}

::grpc::Status inspectorSync::Service::reqestReset(::grpc::ServerContext* context, const ::helmsley::ResetMsg* request, ::commonResponse* response) {
  (void) context;
  (void) request;
  (void) response;
  return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
}

::grpc::Status inspectorSync::Service::setGestureOp(::grpc::ServerContext* context, const ::helmsley::GestureOp* request, ::commonResponse* response) {
  (void) context;
  (void) request;
  (void) response;
  return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
}

::grpc::Status inspectorSync::Service::setTuneParams(::grpc::ServerContext* context, const ::helmsley::TuneMsg* request, ::commonResponse* response) {
  (void) context;
  (void) request;
  (void) response;
  return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
}

::grpc::Status inspectorSync::Service::setCheckParams(::grpc::ServerContext* context, const ::helmsley::CheckMsg* request, ::commonResponse* response) {
  (void) context;
  (void) request;
  (void) response;
  return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
}

::grpc::Status inspectorSync::Service::setMaskParams(::grpc::ServerContext* context, const ::helmsley::MaskMsg* request, ::commonResponse* response) {
  (void) context;
  (void) request;
  (void) response;
  return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
}

::grpc::Status inspectorSync::Service::setDisplayVolume(::grpc::ServerContext* context, const ::helmsley::volumeConcise* request, ::commonResponse* response) {
  (void) context;
  (void) request;
  (void) response;
  return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
}


}  // namespace helmsley

