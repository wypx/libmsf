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
class FastRpcController;
class FastRpcRequest;

typedef std::shared_ptr<FastRpcRequest> FastRpcRequestPtr;
typedef std::shared_ptr<FastRpcController> FastRpcControllerPtr;
typedef std::shared_ptr<frpc::Message> FastRpcMessagePtr;
typedef std::shared_ptr<google::protobuf::Message> GoogleMessagePtr;

struct FastRpcRequest {
  static FastRpcRequestPtr NewFastRpcRequest(const ConnectionPtr& conn,
                                             const FastRpcControllerPtr& ctrl,
                                             const GoogleMessagePtr& request,
                                             const GoogleMessagePtr& response) {
    return std::make_shared<FastRpcRequest>(conn, ctrl, request, response);
  }

  FastRpcRequest(const ConnectionPtr& conn,
                 const FastRpcControllerPtr& controller,
                 const GoogleMessagePtr& request,
                 const GoogleMessagePtr& response)
      : conn_(conn),
        controller_(controller),
        request_(request),
        response_(response) {}

  ~FastRpcRequest() {}

  const ConnectionPtr& conn() { return conn_; }
  const FastRpcControllerPtr& controller() { return controller_; }
  const GoogleMessagePtr& request() { return request_; }
  const GoogleMessagePtr& response() { return response_; }

  ConnectionPtr conn_;
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

  void FastRPCReadCallback(const ConnectionPtr& conn);
  void FastRPCWriteCallback(const ConnectionPtr& conn);
  void FastRPCCloseCallback(const ConnectionPtr& conn);

  void Running() {}

 private:
  std::list<frpc::Message> ParseFrpcMessage(const ConnectionPtr& conn);
  void HandleMessage(const ConnectionPtr& conn, frpc::Message& msg);

 private:
  google::protobuf::Service* GetService(
      const google::protobuf::MethodDescriptor* method);

  void AddFastRpcRequest(const FastRpcRequestPtr& request);

  void RequestReceived(int client_id, const std::string& call_id_,
                       const google::protobuf::MethodDescriptor* method,
                       GoogleMessagePtr request);
  void CancelRequest(int client_id, const std::string& call_id);
  void SendResponse(int client_id, const std::string& call_id,
                    FastRpcController* controller,
                    google::protobuf::Message* response);

  FastRpcRequestPtr RemoveRequest(int client_id, const std::string& call_id);
  void RequestFinished(FastRpcRequestPtr request);

 private:
  bool stop_ = false;
  EventLoop* loop_;

  // Registration Service
  std::map<const google::protobuf::ServiceDescriptor*,
           google::protobuf::Service*> hook_services_;

  typedef std::pair<uint64_t, std::string> FastRpcRequestKey;
  std::map<FastRpcRequestKey, FastRpcRequestPtr> pending_request_;

  std::unique_ptr<FastServer> server_;
};
}
#endif
