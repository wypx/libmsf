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
#ifndef FRPC_CHANNEL_H_
#define FRPC_CHANNEL_H_

#include <string>
#include <mutex>
#include <list>
#include <iostream>
#include <sstream>
#include <condition_variable>
#include <google/protobuf/service.h>

#include "noncopyable.h"
#include "connector.h"
#include "frpc.pb.h"

using namespace MSF;

namespace MSF {

static const size_t kFastRpcMessageHeaderLength = 32;

class EventLoop;
class FastRpcController;

// 基于protobuf的RPC实现
// https://www.cnblogs.com/mengfanrong/p/4724731.html
// https://blog.csdn.net/qq_41681241/article/details/99756740
// http://www.yidianzixun.com/article/0URBE1SL?s=mochuang&appid=s3rd_mochuang
// https://www.cnblogs.com/mq0036/p/14699050.html
// https://izualzhy.cn/demo-protobuf-rpc
class FastRpcChannel : public google::protobuf::RpcChannel, public noncopyable {
 public:
  typedef std::function<void(bool, const std::string&)> ResponseCallback;

 public:
  FastRpcChannel(EventLoop* loop, const InetAddress& addr, int timeout = 0);
  virtual ~FastRpcChannel();

  // Inherited from google::protobuf::RpcChannel
  virtual void CallMethod(const google::protobuf::MethodDescriptor* method,
                          google::protobuf::RpcController* controller,
                          const google::protobuf::Message* request,
                          google::protobuf::Message* response,
                          google::protobuf::Closure* done);

 private:
  struct WaitingResponse {
    WaitingResponse(google::protobuf::RpcController* controller,
                    google::protobuf::Message* response,
                    google::protobuf::Closure* done, bool* completed)
        : controller_(controller),
          response_(response),
          done_(done),
          completed_(completed) {}
    google::protobuf::RpcController* controller_;
    google::protobuf::Message* response_;
    google::protobuf::Closure* done_;
    bool* completed_;
  };

  void ProcessResponse(const std::string& response_msg);

  void SendMessageCb(bool success, const std::string& reason,
                     std::string call_id);

  void CancelCallMethod(const std::string& call_id);
  void CopyMessage(google::protobuf::Message& dest,
                   const google::protobuf::Message& src);
  bool IsCallCompleted(const std::string& call_id);
  void HandleRequestTimeout(const std::string& call_id);

  void SendMessage(const void* buffer, size_t len, const ResponseCallback& cb);

  void FastRPCReadCallback(const ConnectionPtr& conn);
  void FastRPCWriteCallback(const ConnectionPtr& conn);
  void FastRPCCloseCallback(const ConnectionPtr& conn);

  std::list<frpc::Message> ParseFrpcMessage(const ConnectionPtr& conn);
  void HandleMessage(const ConnectionPtr& conn, const frpc::Message& msg);

 private:
  void connect();
  void close();

 private:
  EventLoop* loop_;
  InetAddress addr_;

  ConnectorPtr ctor_;

  // <= 0, no request timeout, > 0, request timeout in milliseconds
  int timeout_ = 0;

  // map for waiting response: key is request call_id,
  // value is the parameters in the CallMethod())
  typedef std::shared_ptr<WaitingResponse> WaitingResponsePtr;
  std::unordered_map<std::string, WaitingResponsePtr> waiting_responses_;

  std::mutex mutex_;
  std::condition_variable cond_;
};

typedef std::shared_ptr<FastRpcChannel> FastRpcChannelPtr;
}
#endif
