/**************************************************************************
 *
 * Copyright (c) 2017-2021, luotang.me <wypx520@gmail.com>, China.
 * All rights reserved.
 *
 * Distributed under the terms of the GNU General Public License v2.
 *
 * This software is provided 'as is' with no explicit or implied warranties
 * in respect of its properties, including, but not limited to, correctness
 * and/or fitness for purpose.
 *
 **************************************************************************/
#include "frpc_controller.h"
#include "frpc_server.h"
#include "frpc_message.h"
#include "frpc_channel.h"

#include <sstream>

#include <base/logging.h>
#include <base/utils.h>
#include <base/gcc_attr.h>
#include <network/event/event_loop.h>

namespace MSF {

FastRpcServer::FastRpcServer(EventLoop* loop, const InetAddress& addr)
    : loop_(loop) {
  server_.reset(new FastTcpServer(loop, addr));
  server_->StartAccept();
}

FastRpcServer::~FastRpcServer() { stop_ = true; }

std::list<frpc::Message> FastRpcServer::ParseFrpcMessage(
    const ConnectionPtr& conn) {
  std::list<frpc::Message> messages;
  char buffer[2048];
  size_t avaliable_len;

  while (true) {
    avaliable_len = conn->ReadableLength();
    if (unlikely(avaliable_len <= kFastRpcMessageHeaderLength)) {
      break;
    }
    conn->ReceiveData(buffer, kFastRpcMessageHeaderLength);
    frpc::Message message;
    frpc::MessageHead* head = message.mutable_head();
    head->ParseFromArray(buffer, kFastRpcMessageHeaderLength);
    size_t body_len = head->length();
    if (avaliable_len < kFastRpcMessageHeaderLength + body_len) {
      break;
    }
    conn->RemoveData(buffer, kFastRpcMessageHeaderLength + body_len);
    message.Clear();
    message.ParseFromArray(buffer, body_len);
    messages.push_back(std::move(message));
  }
  return messages;
}

void FastRpcServer::HandleMessage(const ConnectionPtr& conn,
                                  frpc::Message& msg) {
  auto ctrl = FastRpcController::NewController();
  const frpc::FrpcRequest& req = msg.body().frpc_request();

  switch (msg.head().type()) {
    case frpc::FRPC_MESSAGE_REQUEST: {
      LOG(INFO) << "a request is received";
      // auto handle = FastRpcHandleFactory::Instance()->CreateHandle(
      //     frpc::FRPC_MESSAGE_REQUEST);
      // handle->EntryInit(msg);

      auto method = FastRPCMessage::GetMethodDescriptor(req.method());
      auto service = GetService(method);
      auto desc = FastRPCMessage::GetMessageTypeByName(
          msg.GetDescriptor()->full_name());
      auto request = FastRPCMessage::NewGoogleMessage(desc);

      if (service) {
        LOG(INFO) << "find service by method:" << method->full_name()
                  << ", execute request with call_id:" << req.service();

        GoogleMessagePtr rsp(service->GetResponsePrototype(method).New());

        auto frpc_request = FastRpcRequest::NewFastRpcRequest(
            conn, ctrl, GoogleMessagePtr(request), rsp);
        AddFastRpcRequest(frpc_request);

        // Can't use references.
        auto done = google::protobuf::NewCallback(
            this, &FastRpcServer::RequestFinished, frpc_request);
        service->CallMethod(method, ctrl.get(), request, rsp.get(), done);
      } else {
        LOG(INFO) << "fail to find service by method:" << method->full_name();
        ctrl->SetFailed("fail to find the service in the server");

        std::string call_id = req.call_id();
        auto head = msg.head();
        head.set_version(123);
        head.set_magic(456);
        head.set_type(frpc::FRPC_MESSAGE_RESPONSE);
        head.set_msid(100);

        msg.clear_body();
        auto frpc_response = msg.mutable_body()->mutable_frpc_response();

        frpc_response->mutable_rc()->set_msg("no service");
        frpc_response->mutable_rc()->set_rc(-1);

        frpc_response->set_call_id(call_id);

        head.set_length(frpc_response->ByteSizeLong());

        char buffer[1024] = {0};
        msg.SerializeToArray(buffer, msg.ByteSizeLong());

        conn->SubmitWriteBufferSafe(buffer, msg.ByteSizeLong());
      }
      break;
    }
    case frpc::FRPC_MESSAGE_RESPONSE:
      break;
    case frpc::FRPC_MESSAGE_CANCEL: {
      frpc::FrpcCancel cancel = msg.body().frpc_cancel();
      LOG(INFO) << "a cancel request is received";
      FastRpcRequestPtr request = RemoveRequest(1, "call_id");
      if (request) {
        LOG(INFO) << "cancel request with call_id ";
        FastRpcController ctrl;
        ctrl.canceled_ = true;
        // SendResponse("client_id", "call_id", &ctrl, 0);
      } else {
        LOG(INFO) << "the request is already finished, cancel call";
      }
      break;
    }
    default:
      break;
  }
}

void FastRpcServer::FastRPCReadCallback(const ConnectionPtr& conn) {
  auto messages = ParseFrpcMessage(conn);
  for (auto message : messages) {
    HandleMessage(conn, message);
  }
}

void FastRpcServer::FastRPCWriteCallback(const ConnectionPtr& conn) {
  LOG(L_DEBUG) << "conn write cb";
}

void FastRpcServer::FastRPCCloseCallback(const ConnectionPtr& conn) {
  LOG(INFO) << "conn close cb";
}

void FastRpcServer::CancelRequest(int client_id, const std::string& call_id) {}

/**
 * get the registered service by method.
 * The service is registered by HookService() method
 * @param method
 * @return the Service if it is exported by HookService() call,
 *         NULL if the service is not exported
 */
google::protobuf::Service* FastRpcServer::GetService(
    const google::protobuf::MethodDescriptor* method) {
  return hook_services_[method->service()];
}

void FastRpcServer::RequestFinished(FastRpcRequestPtr request) {
#if 0
  if (request) {
    if (RemoveRequest(request->client_id_, request->call_id_) == request) {
      LOG(INFO)
          << "request process is finished, start to send rsp of request "
          << request->call_id_ << " to client " << request->client_id_;
      // SendResponse(request->client_id_, request->call_id_,
      //              request->controller_.get(), request->response_.get());
    } else {
      LOG(ERROR)
          << "request process is finished, but can't find the request with id "
          << request->call_id_ << ","
          << " request is running";
    }
  }
#endif
}

void FastRpcServer::AddFastRpcRequest(const FastRpcRequestPtr& request) {
  // pending_request_.insert(std::pair(
  //     FastRpcRequestKey(request->client_id_, request->call_id_), request));
}

FastRpcRequestPtr FastRpcServer::RemoveRequest(int client_id,
                                               const std::string& call_id) {
  auto key = FastRpcRequestKey(client_id, call_id);
  auto it = pending_request_.find(key);
  if (it == pending_request_.end()) {
    return nullptr;
  }
  FastRpcRequestPtr request = it->second;
  pending_request_.erase(it);
  return request;
}

}  // end name space MSF
