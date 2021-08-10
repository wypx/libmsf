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

#include <google/protobuf/message.h>
#include <google/protobuf/service.h>
#include <network/sock/tcp_server.h>
#include <network/sock/udp_server.h>

#include <list>
#include <unordered_map>

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

class FastRpcMethod : public noncopyable {
 public:
  FastRpcMethod(google::protobuf::Service* service,
                const google::protobuf::MethodDescriptor* method,
                const google::protobuf::Message* request,
                const google::protobuf::Message* response)
      : service_(service),
        method_(method),
        request_(request),
        response_(response) {}

  ~FastRpcMethod() = default;
  google::protobuf::Service* service() { return service_; }
  const google::protobuf::MethodDescriptor* method() { return method_; }
  const google::protobuf::Message* request() { return request_; };
  const google::protobuf::Message* response() { return response_; };

 private:
  google::protobuf::Service* service_;
  const google::protobuf::MethodDescriptor* method_;
  const google::protobuf::Message* request_;
  const google::protobuf::Message* response_;
};

typedef std::shared_ptr<FastRpcMethod> FastRpcMethodPtr;

class FastRpcServer : public noncopyable {
 public:
  FastRpcServer(EventLoop* loop, const InetAddress& addr);
  virtual ~FastRpcServer();
  void ActiveService(google::protobuf::Service* service) {}

  void InActiveService(google::protobuf::Service* service) {}

  void HookService(google::protobuf::Service* service);

  void Running() {}
  int ServiceCount();
  int ConnectionCount();
  bool IsListening();
  // Restart the listener.  It will restart listening anyway, even if it is
  // already in
  // listening.  Return false if the server is not started, or fail to restart
  // listening.
  bool RestartListen();

 private:
  void HandleFastRPCSucc(const ConnectionPtr& conn);
  void HandleFastRPCRead(const ConnectionPtr& conn);
  void HandleFastRPCWrite(const ConnectionPtr& conn);
  void HandleFastRPClose(const ConnectionPtr& conn);

  void HandleFrpcMessage(const ConnectionPtr& conn);

 private:
  const FastRpcMethodPtr& GetFastMethod(uint32_t opcode);

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
  std::unordered_map<uint32_t, FastRpcMethodPtr> hook_services_;

  std::set<FastRpcRequestPtr> pending_request_;
  std::unique_ptr<FastServer> server_;
};
}  // namespace MSF
#endif
