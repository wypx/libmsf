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
#include "frpc_channel.h"
#include <network/event/event_loop.h>
#include <base/utils.h>
#include <base/gcc_attr.h>
#include <base/logging.h>
#include "frpc_message.h"
#include "frpc_controller.h"
#include "frpc.pb.h"
namespace MSF {

FastRpcChannel::FastRpcChannel(EventLoop* loop, const InetAddress& addr,
                               bool reconnect, int timeout)
    : loop_(loop), addr_(addr), reconnect_(reconnect), timeout_(timeout) {}

FastRpcChannel::~FastRpcChannel() {}

void FastRpcChannel::Connect() {
  loop_->RunInLoop([this] {
    ctor_.reset(new Connector(loop_, addr_));
    ctor_->Connect();
    auto conn = ctor_->conn();
    conn->SetConnSuccCb(
        std::bind(&FastRpcChannel::FastRPCSuccCallback, this, conn));
    conn->SetConnReadCb(
        std::bind(&FastRpcChannel::FastRPCReadCallback, this, conn));
    conn->SetConnWriteCb(
        std::bind(&FastRpcChannel::FastRPCWriteCallback, this, conn));
    conn->SetConnCloseCb(
        std::bind(&FastRpcChannel::FastRPCCloseCallback, this, conn));
    conn->AddGeneralEvent();
  });
}

void FastRpcChannel::Close() {
  loop_->RunInLoop([this] { ctor_.reset(); });
}

void FastRpcChannel::CallMethod(
    const google::protobuf::MethodDescriptor* method,
    google::protobuf::RpcController* controller,
    const google::protobuf::Message* request,
    google::protobuf::Message* response, google::protobuf::Closure* done) {

  // each call will allocate a unique call_id for parallel processing reason
  std::string call_id = NewUuid();

  if (controller) {
    FastRpcController* ctrl = dynamic_cast<FastRpcController*>(controller);
    if (ctrl) {
      ctrl->set_cancel_cb(
          std::bind(&FastRpcChannel::CancelCallMethod, this, call_id));
    }
  }

  bool completed = false;
  WaitingResponsePtr param(new WaitingResponse(controller, response, done,
                                               (done == 0) ? (&completed) : 0));

  waiting_responses_[call_id] = param;

  // Write uint32 to desc FastMessage length

  // serialize the message
  frpc::FastMessage frpc;
  frpc.set_version(123);
  frpc.set_magic(456);
  frpc.set_type(frpc::FRPC_MESSAGE_REQUEST);
  frpc.set_length(request->ByteSizeLong());
  frpc.set_call_id(call_id);
  frpc.set_service(request->GetDescriptor()->full_name());
  frpc.set_method(method->full_name());
  frpc.set_opcode(std::hash<std::string>()(method->full_name()));

  LOG(INFO) << "send message " << frpc.ByteSizeLong() << " " << frpc.length();

  LOG(INFO) << frpc.DebugString();
  LOG(INFO) << request->DebugString();

  loop_->RunInLoop([this, frpc, request] {
    if (!ctor_) {
      Connect();
    }

    uint32_t frpc_len = frpc.ByteSizeLong();

    char* buffer = static_cast<char*>(
        ctor_->conn()->Malloc(sizeof(uint32_t) + frpc_len + frpc.length(), 64));
    if (!buffer) {
      LOG(ERROR) << "conn malloc buffer failed";
      // no mem in pool
      return;
    }
    ::memcpy(buffer, &frpc_len, sizeof(uint32_t));

    frpc.SerializeToArray(buffer + sizeof(uint32_t), frpc_len);
    request->SerializeToArray(buffer + sizeof(uint32_t) + frpc_len,
                              frpc.length());

    ctor_->conn()->SubmitWriteBufferSafe(
        buffer, sizeof(uint32_t) + frpc_len + frpc.length());
    ctor_->conn()->AddWriteEvent();

    // start a timer if timeout is enabled
    if (timeout_ > 0) {
      loop_->RunAfter(timeout_, std::bind(&FastRpcChannel::HandleRequestTimeout,
                                          this, frpc.call_id()));
    }
  });

  // if sync call, waiting for complete
  // if (!done) {
  //   std::unique_lock<std::mutex> lock(mutex_);
  //   while (!completed) {
  //     cond_.wait(lock);
  //   }
  // }
}

/**
 * The callback for the the SendMessage() method
 *
 * @param success true if the message is sent to server successfully,
 *                false if the message is failed to send to server
 * @param reason  the reason if the message is failed to send to server
 * @param call_id which call the message is sent about
 */
std::list<GoogleMessage*> FastRpcChannel::ParseFrpcMessage(
    const ConnectionPtr& conn) {
  std::list<GoogleMessage*> messages;

  char buffer[2048];
  size_t avaliable_len;
  uint32_t length = 0;

  while (true) {
    avaliable_len = conn->ReadableLength();
    if (unlikely(avaliable_len <= sizeof(uint32_t))) {
      break;
    }
    conn->ReceiveData(&length, sizeof(uint32_t));
    if (unlikely(avaliable_len < sizeof(uint32_t) + length)) {
      break;
    }
    conn->ReceiveData(buffer, sizeof(uint32_t) + length);
    frpc::FastMessage frpc;
    frpc.ParseFromArray(buffer + sizeof(uint32_t), length);

    LOG(INFO) << frpc.DebugString();
    if (unlikely(avaliable_len < sizeof(uint32_t) + length + frpc.length())) {
      break;
    }
    conn->RemoveData(buffer, sizeof(uint32_t) + length + frpc.length());
    switch (frpc.type()) {
      case frpc::FRPC_MESSAGE_RESPONSE: {
        LOG(INFO) << "a response is received";
        // auto desc = FastRPCMessage::GetMessageTypeByName(frpc.service());
        // auto response = FastRPCMessage::NewGoogleMessage(desc);

        auto it = waiting_responses_.find(frpc.call_id());
        if (it != waiting_responses_.end()) {
          const WaitingResponsePtr& ptr = it->second;
          if (ptr->response_->ParseFromArray(buffer + sizeof(uint32_t) + length,
                                             frpc.length())) {
            LOG(INFO) << ptr->response_->DebugString();
            // if (ptr->done_) {
            //   ptr->done_->Run();
            // } else {
            //   cond_.notify_all();
            // }
          } else {
            LOG(ERROR) << "frpc parse failed";
            conn->CloseConn();
          }
        }
        break;
      }

      default:
        break;
    }
  }
  return messages;
}

void FastRpcChannel::HandleMessage(const ConnectionPtr& conn,
                                   GoogleMessage* msg) {
#if 0
  auto ctrl = FastRpcController::NewController();

  switch (msg.head().type()) {
    case frpc::FRPC_MESSAGE_REQUEST: {

      LOG(INFO) << "a request is received";
      // auto handle = FastRpcHandleFactory::Instance()->CreateHandle(
      //     frpc::FRPC_MESSAGE_REQUEST);
      // handle->EntryInit(msg);

      const frpc::FrpcRequest& frpc_request = msg.body().frpc_request();
      std::string call_id = frpc_request.call_id();
      auto method = FastRPCMessage::GetMethodDescriptor(frpc_request.method());
      auto service = GetService(method);
      auto desc = FastRPCMessage::GetMessageTypeByName(msg.full_name());
      auto request = FastRPCMessage::NewGoogleMessage(desc);

      if (service) {
        LOG(INFO) << "find service by method:" << method->full_name()
                  << ", execute request with call_id:"
                  << frpc_request.service();

        GoogleMessagePtr rsp(service->GetResponsePrototype(method).New());

        auto request =
            FastRpcRequest::NewFastRpcRequest(conn, ctrl, request, rsp);
        AddFastRpcRequest(frpc_request);

        // Can't use references.
        auto done = google::protobuf::NewCallback(
            this, &FastRpcServer::RequestFinished, request);
        service->CallMethod(method, ctrl.get(), request.get(), rsp.get(), done);
      } else {
        LOG(INFO) << "fail to find service by method:" << method->full_name();
        ctrl->SetFailed("fail to find the service in the server");

        std::ostringstream out;
        FastRpcMessage::SerializeResponse("call_id", *ctrl.get(), nullptr, out);
        conn->SubmitWriteBufferSafe(const_cast<char*>(out.str().c_str()),
                                    out.str().size());
      }

      break;
    }
    case frpc::FRPC_MESSAGE_RESPONSE: {
      const frpc::FrpcResponse& frpc_reponse = msg.body().frpc_response();

      // parse the response
      std::string call_id = frpc_reponse.call_id();
      LOG(INFO) << "received a response message, call_id:" << call_id;
      // if the response is still not timeout
      auto it = waiting_responses_.find(call_id);
      if (it != waiting_responses_.end()) {
        const WaitingResponsePtr& param = it->second;

        std::string call_id = frpc_reponse.call_id();
        auto desc = FastRPCMessage::GetMessageTypeByName(
            msg.GetDescriptor()->full_name());
        auto msg = FastRPCMessage::NewGoogleMessage(desc);

        // TODO
        // timer_->cancel(call_id);
        // loop_->cancel(call_id);
        FastRpcController* ctrl =
            dynamic_cast<FastRpcController*>(param->controller_);

        // ctrl parse error

        if (param->response_ && msg) {
          CopyMessage(*(param->response_), *msg);
        }

        if (param->completed_) {
          {
            std::lock_guard<std::mutex> lock(mutex_);
            *(param->completed_) = true;
          }
          cond_.notify_all();
        }

        if (param->done_) {
          param->done_->Run();
        }

        if (ctrl) {
          ctrl->complete();
        }
        waiting_responses_.erase(call_id);
      }
      break;
    }
    case frpc::FRPC_MESSAGE_CANCEL: {
      // frpc::FrpcCancel cancel = msg.body().frpc_cancel();
      // LOG(INFO) << "a cancel request is received";
      // FastRpcRequestPtr request = RemoveRequest(1, "call_id");
      // if (request) {
      //   LOG(INFO) << "cancel request with call_id ";
      //   FastRpcController ctrl;
      //   ctrl.canceled_ = true;
      //   // SendResponse("client_id", "call_id", &ctrl, 0);
      // } else {
      //   LOG(INFO) << "the request is already finished, cancel call";
      // }
      break;
    }
    default:
      break;
  }
#endif
}
void FastRpcChannel::FastRPCSuccCallback(const ConnectionPtr& conn) {
  LOG(INFO) << "conn succ cb";
}

void FastRpcChannel::FastRPCReadCallback(const ConnectionPtr& conn) {
  auto messages = ParseFrpcMessage(conn);
  // for (auto message : messages) {
  //   HandleMessage(conn, message);
  // }
}

void FastRpcChannel::FastRPCWriteCallback(const ConnectionPtr& conn) {
  LOG(INFO) << "conn write cb";
}

void FastRpcChannel::FastRPCCloseCallback(const ConnectionPtr& conn) {
  LOG(INFO) << "conn Close cb";
  if (reconnect_ && !connecting_) {
    connecting_ = true;
    Connect();
    connecting_ = false;
  }
}

void FastRpcChannel::CancelCallMethod(const std::string& call_id) {
  // if the request may still on-going, send cancel request to server
  loop_->RunInLoop([this, call_id] {
    if (waiting_responses_.find(call_id) != waiting_responses_.end()) {
      // frpc::Message message;
      // auto head = message.mutable_head();
      // head->set_version(123);
      // head->set_magic(456);
      // head->set_type(frpc::FRPC_MESSAGE_CANCEL);
      // head->set_msid(100);

      // auto cancel = message.mutable_body()->mutable_frpc_cancel();
      // cancel->set_service("echo");
      // cancel->set_method("echo_impl");
      // cancel->set_call_id(call_id);

      // head->set_length(cancel->ByteSizeLong());

      // char buffer[1024] = {0};
      // message.SerializeToArray(buffer, message.ByteSizeLong());

      // ctor_->conn()->SubmitWriteBufferSafe(buffer, message.ByteSizeLong());
    }
  });
}

void FastRpcChannel::CopyMessage(google::protobuf::Message& dest,
                                 const google::protobuf::Message& src) {
  if (dest.GetDescriptor() == src.GetDescriptor()) {
    dest.CopyFrom(src);
  }
}

bool FastRpcChannel::IsCallCompleted(const std::string& call_id) {
  return (waiting_responses_.find(call_id) == waiting_responses_.end());
}

void FastRpcChannel::HandleRequestTimeout(const std::string& call_id) {
  LOG(INFO) << "timeout: " << call_id;
  auto it = waiting_responses_.find(call_id);
  if (it == waiting_responses_.end()) {
    return;
  }
  const WaitingResponsePtr& param = it->second;
  if (param) {
    FastRpcController* ctrl =
        dynamic_cast<FastRpcController*>(param->controller_);
    if (ctrl) {
      ctrl->SetFailed("request timeout");
    }

    if (param->completed_) {
      {
        std::lock_guard<std::mutex> lock(mutex_);
        *(param->completed_) = true;
      }
      cond_.notify_all();
    }

    if (param->done_) {
      param->done_->Run();
    }

    if (ctrl) {
      ctrl->complete();
    }
  }
  waiting_responses_.erase(call_id);
}
}