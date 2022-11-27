// Generated by the gRPC C++ plugin.
// If you make any local change, they will be lost.
// source: declvol/v1/declvol.proto

#include "declvol/v1/declvol.pb.h"
#include "declvol/v1/declvol.grpc.pb.h"

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
namespace declvol {
namespace v1 {

static const char* Declvol_method_names[] = {
  "/declvol.v1.Declvol/SwitchProfile",
};

std::unique_ptr< Declvol::Stub> Declvol::NewStub(const std::shared_ptr< ::grpc::ChannelInterface>& channel, const ::grpc::StubOptions& options) {
  (void)options;
  std::unique_ptr< Declvol::Stub> stub(new Declvol::Stub(channel, options));
  return stub;
}

Declvol::Stub::Stub(const std::shared_ptr< ::grpc::ChannelInterface>& channel, const ::grpc::StubOptions& options)
  : channel_(channel), rpcmethod_SwitchProfile_(Declvol_method_names[0], options.suffix_for_stats(),::grpc::internal::RpcMethod::NORMAL_RPC, channel)
  {}

::grpc::Status Declvol::Stub::SwitchProfile(::grpc::ClientContext* context, const ::declvol::v1::SwitchProfileRequest& request, ::declvol::v1::SwitchProfileResponse* response) {
  return ::grpc::internal::BlockingUnaryCall< ::declvol::v1::SwitchProfileRequest, ::declvol::v1::SwitchProfileResponse, ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(channel_.get(), rpcmethod_SwitchProfile_, context, request, response);
}

void Declvol::Stub::async::SwitchProfile(::grpc::ClientContext* context, const ::declvol::v1::SwitchProfileRequest* request, ::declvol::v1::SwitchProfileResponse* response, std::function<void(::grpc::Status)> f) {
  ::grpc::internal::CallbackUnaryCall< ::declvol::v1::SwitchProfileRequest, ::declvol::v1::SwitchProfileResponse, ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(stub_->channel_.get(), stub_->rpcmethod_SwitchProfile_, context, request, response, std::move(f));
}

void Declvol::Stub::async::SwitchProfile(::grpc::ClientContext* context, const ::declvol::v1::SwitchProfileRequest* request, ::declvol::v1::SwitchProfileResponse* response, ::grpc::ClientUnaryReactor* reactor) {
  ::grpc::internal::ClientCallbackUnaryFactory::Create< ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(stub_->channel_.get(), stub_->rpcmethod_SwitchProfile_, context, request, response, reactor);
}

::grpc::ClientAsyncResponseReader< ::declvol::v1::SwitchProfileResponse>* Declvol::Stub::PrepareAsyncSwitchProfileRaw(::grpc::ClientContext* context, const ::declvol::v1::SwitchProfileRequest& request, ::grpc::CompletionQueue* cq) {
  return ::grpc::internal::ClientAsyncResponseReaderHelper::Create< ::declvol::v1::SwitchProfileResponse, ::declvol::v1::SwitchProfileRequest, ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(channel_.get(), cq, rpcmethod_SwitchProfile_, context, request);
}

::grpc::ClientAsyncResponseReader< ::declvol::v1::SwitchProfileResponse>* Declvol::Stub::AsyncSwitchProfileRaw(::grpc::ClientContext* context, const ::declvol::v1::SwitchProfileRequest& request, ::grpc::CompletionQueue* cq) {
  auto* result =
    this->PrepareAsyncSwitchProfileRaw(context, request, cq);
  result->StartCall();
  return result;
}

Declvol::Service::Service() {
  AddMethod(new ::grpc::internal::RpcServiceMethod(
      Declvol_method_names[0],
      ::grpc::internal::RpcMethod::NORMAL_RPC,
      new ::grpc::internal::RpcMethodHandler< Declvol::Service, ::declvol::v1::SwitchProfileRequest, ::declvol::v1::SwitchProfileResponse, ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(
          [](Declvol::Service* service,
             ::grpc::ServerContext* ctx,
             const ::declvol::v1::SwitchProfileRequest* req,
             ::declvol::v1::SwitchProfileResponse* resp) {
               return service->SwitchProfile(ctx, req, resp);
             }, this)));
}

Declvol::Service::~Service() {
}

::grpc::Status Declvol::Service::SwitchProfile(::grpc::ServerContext* context, const ::declvol::v1::SwitchProfileRequest* request, ::declvol::v1::SwitchProfileResponse* response) {
  (void) context;
  (void) request;
  (void) response;
  return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
}


}  // namespace declvol
}  // namespace v1

