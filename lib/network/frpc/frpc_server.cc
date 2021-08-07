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

FastRpcServer::FastRpcServer(EventLoop* loop, const InetAddress& addr) {
  // Verify that the version of the library that we linked against is
  // compatible with the version of the headers we compiled against.
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  server_.reset(new FastTcpServer(loop, addr));
  server_->StartAccept();
  server_->SetCallback(
      std::bind(&FastRpcServer::HandleFastRPCSucc, this, std::placeholders::_1),
      std::bind(&FastRpcServer::HandleFastRPCRead, this, std::placeholders::_1),
      std::bind(&FastRpcServer::HandleFastRPCWrite, this,
                std::placeholders::_1),
      std::bind(&FastRpcServer::HandleFastRPClose, this,
                std::placeholders::_1));
}

FastRpcServer::~FastRpcServer() {
  stop_ = true;
  // Optional:  Delete all global objects allocated by libprotobuf.
  google::protobuf::ShutdownProtobufLibrary();
}

void FastRpcServer::HandleFrpcMessage(const ConnectionPtr& conn) {
  char* buffer;
  size_t avaliable_len;
  uint32_t length = 0;

  while (true) {
    // receive 4 bytes for frpc message length
    avaliable_len = conn->ReadableLength();
    if (unlikely(avaliable_len <= sizeof(uint32_t))) {
      break;
    }
    conn->ReceiveData(&length, sizeof(uint32_t));

    // receive length bytes for frpc message
    if (unlikely(avaliable_len < sizeof(uint32_t) + length)) {
      break;
    }
    buffer = static_cast<char*>(conn->Malloc(sizeof(uint32_t) + length, 64));
    if (!buffer) {
      // no mem in pool
      break;
    }
    conn->ReceiveData(buffer, sizeof(uint32_t) + length);
    FastRpcMessagePtr frpc(new frpc::FastMessage());
    if (!frpc->ParseFromArray(buffer + sizeof(uint32_t), length)) {
      conn->Free(buffer);
      conn->CloseConn();
      return;
    }
    conn->Free(buffer);

    // receive frpc->length() bytes for spcific bussiness message
    if (unlikely(avaliable_len < sizeof(uint32_t) + length + frpc->length())) {
      break;
    }
    buffer = static_cast<char*>(
        conn->Malloc(sizeof(uint32_t) + length + frpc->length(), 64));
    if (!buffer) {
      // no mem in pool
      break;
    }
    conn->RemoveData(buffer, sizeof(uint32_t) + length + frpc->length());

    switch (frpc->type()) {
      case frpc::FRPC_MESSAGE_REQUEST: {
        LOG(INFO) << "request received: \n" << frpc->DebugString();

        // construct service by method name
        auto method_ptr = GetFastMethod(frpc->opcode());
        if (!method_ptr) {
          conn->Free(buffer);
          conn->CloseConn();
          return;
        }
        // https://izualzhy.cn/demo-protobuf-rpc
        // construct request message by service name
        // auto desc = FastRPCMessage::GetMessageTypeByName(frpc->service());
        // auto request = FastRPCMessage::NewGoogleMessage(desc);

        auto request = method_ptr->request()->New();
        auto response = method_ptr->response()->New();

        if (!request->ParseFromArray(buffer + sizeof(uint32_t) + length,
                                     frpc->length())) {
          conn->Free(buffer);
          conn->CloseConn();
          return;
        }
        conn->Free(buffer);

        LOG(INFO) << request->DebugString();

        LOG(INFO) << "find service by method:" << frpc->method()
                  << ", execute request:" << frpc->service();

        auto ctrl = FastRpcController::NewController();

        auto pending_request = FastRpcRequest::NewFastRpcRequest(
            conn, frpc, ctrl, GoogleMessagePtr(request),
            GoogleMessagePtr(response));

        pending_request_.insert(pending_request);

        // Can't use references.
        auto done = google::protobuf::NewCallback(
            this, &FastRpcServer::SendResponse, pending_request);
        method_ptr->service()->CallMethod(method_ptr->method(), ctrl.get(),
                                          request, response, done);
        break;
      }

      case frpc::FRPC_MESSAGE_RESPONSE: { break; }

      case frpc::FRPC_MESSAGE_CANCEL: { break; }

      default:
        break;
    }
  }
}

void FastRpcServer::HandleFastRPCSucc(const ConnectionPtr& conn) {}

void FastRpcServer::HandleFastRPCRead(const ConnectionPtr& conn) {
  HandleFrpcMessage(conn);
}

void FastRpcServer::HandleFastRPCWrite(const ConnectionPtr& conn) {}

void FastRpcServer::HandleFastRPClose(const ConnectionPtr& conn) {}

void FastRpcServer::CancelRequest(int client_id, const std::string& call_id) {}

void FastRpcServer::HookService(google::protobuf::Service* service) {
  const ::google::protobuf::ServiceDescriptor* descriptor =
      service->GetDescriptor();
  for (int i = 0; i < descriptor->method_count(); ++i) {
    const ::google::protobuf::MethodDescriptor* method = descriptor->method(i);
    const ::google::protobuf::Message* request =
        &service->GetRequestPrototype(method);
    const ::google::protobuf::Message* response =
        &service->GetResponsePrototype(method);

    FastRpcMethodPtr method_ptr(
        new FastRpcMethod(service, method, request, response));
    uint32_t opcode = std::hash<std::string>()(method->full_name());
    // find the method by method full_name
    // auto method = FastRPCMessage::GetMethodDescriptor(frpc->method());

    LOG(INFO) << "register service: " << method->full_name()
              << ", opcode: " << opcode;
    hook_services_[opcode] = method_ptr;
  }
}

/**
 * get the registered service by method.
 * The service is registered by HookService() method
 * @param method
 * @return the Service if it is exported by HookService() call,
 *         NULL if the service is not exported
 */
static const FastRpcMethodPtr kEmptyFastRpcMethodPtr;
const FastRpcMethodPtr& FastRpcServer::GetFastMethod(uint32_t opcode) {
  auto it = hook_services_.find(opcode);
  if (it == hook_services_.end()) {
    return kEmptyFastRpcMethodPtr;
  }
  return it->second;
}

void FastRpcServer::SendResponse(FastRpcRequestPtr msg) {
  auto& response = msg->response();
  auto& frpc = msg->frpc();

  // Write uint32 to desc FastMessage length

  // serialize the message
  frpc->set_type(frpc::FRPC_MESSAGE_RESPONSE);
  frpc->set_length(response->ByteSizeLong());
  frpc->set_service(response->GetDescriptor()->full_name());

  LOG(INFO) << "send response: \n" << frpc->DebugString();

  uint32_t frpc_len = frpc->ByteSizeLong();
  char* buffer = static_cast<char*>(
      msg->conn()->Malloc(sizeof(uint32_t) + frpc_len + frpc->length(), 64));
  if (!buffer) {
    // no mem in pool
    return;
  }
  ::memcpy(buffer, &frpc_len, sizeof(uint32_t));

  frpc->SerializeToArray(buffer + sizeof(uint32_t), frpc_len);
  response->SerializeToArray(buffer + sizeof(uint32_t) + frpc_len,
                             frpc->length());

  msg->conn()->SubmitWriteBufferSafe(
      buffer, sizeof(uint32_t) + frpc_len + frpc->length());
  msg->conn()->AddWriteEvent();

  pending_request_.erase(msg);
}
}  // end name space MSF
