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
#ifndef FRPC_SERVER_H_
#define FRPC_SERVER_H_

// https://github.com/xiongfengOrz/MiniRPC
// https://github.com/LiveMirror/nominate-gitee-feimat-fastrpc

#include <map>
#include <list>

#include <google/protobuf/service.h>
#include <google/protobuf/message.h>

#include <network/sock/tcp_server.h>
#include <network/sock/udp_server.h>

#include "frpc_handle.h"

namespace MSF {

class EventLoop;

typedef std::shared_ptr<frpc::FastMessage> FastRpcMessagePtr;

struct FastRpcRequest {
  static FastRpcRequestPtr NewFastRpcRequest(const ConnectionPtr& conn,
                                             const FastRpcMessagePtr& frpc,
                                             const FastRpcControllerPtr& ctrl,
                                             const GoogleMessagePtr& request,
                                             const GoogleMessagePtr& response) {
    return std::make_shared<FastRpcRequest>(conn, frpc, ctrl, request,
                                            response);
  }

  FastRpcRequest(const ConnectionPtr& conn, const FastRpcMessagePtr& frpc,
                 const FastRpcControllerPtr& controller,
                 const GoogleMessagePtr& request,
                 const GoogleMessagePtr& response)
      : conn_(conn),
        frpc_(frpc),
        controller_(controller),
        request_(request),
        response_(response) {}

  ~FastRpcRequest() {}

  const ConnectionPtr& conn() { return conn_; }
  FastRpcMessagePtr& frpc() { return frpc_; }
  const FastRpcControllerPtr& controller() { return controller_; }
  const GoogleMessagePtr& request() { return request_; }
  const GoogleMessagePtr& response() { return response_; }

  ConnectionPtr conn_;
  FastRpcMessagePtr frpc_;
  FastRpcControllerPtr controller_;
  GoogleMessagePtr request_;
  GoogleMessagePtr response_;
};

class FastRpcServer : public noncopyable {
 public:
  FastRpcServer(EventLoop* loop, const InetAddress& addr);
  virtual ~FastRpcServer();
  void HookService(google::protobuf::Service* service) {
    if (service) {
      hook_services_[service->GetDescriptor()] = service;
    }
  }

  void HandleFastRPCSucc(const ConnectionPtr& conn);
  void HandleFastRPCRead(const ConnectionPtr& conn);
  void HandleFastRPCWrite(const ConnectionPtr& conn);
  void HandleFastRPClose(const ConnectionPtr& conn);

  void Running() {}

 private:
  void HandleFrpcMessage(const ConnectionPtr& conn);

 private:
  google::protobuf::Service* GetService(
      const google::protobuf::MethodDescriptor* method);

  void RequestReceived(int client_id, const std::string& call_id_,
                       const google::protobuf::MethodDescriptor* method,
                       GoogleMessagePtr request);
  void CancelRequest(int client_id, const std::string& call_id);
  void SendResponse(int client_id, const std::string& call_id,
                    FastRpcController* controller,
                    google::protobuf::Message* response);

  void SendResponse(FastRpcRequestPtr msg);

 private:
  bool stop_ = false;
  EventLoop* loop_;

  // Registration Service
  std::map<const google::protobuf::ServiceDescriptor*,
           google::protobuf::Service*> hook_services_;

  // typedef std::pair<uint64_t, std::string> FastRpcRequestKey;
  // std::map<FastRpcRequestKey, FastRpcRequestPtr> pending_request_;
  std::set<FastRpcRequestPtr> pending_request_;
  std::unique_ptr<FastServer> server_;
};
}
#endif
