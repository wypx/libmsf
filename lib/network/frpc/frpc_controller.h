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
#ifndef FRPC_CONTROLLER_H_
#define FRPC_CONTROLLER_H_

#include <google/protobuf/service.h>
#include <iostream>
#include <string>
#include <functional>
#include <mutex>
#include <set>

namespace MSF {

class FastRpcChannel;
class FastRpcServer;
class FastRpcUtil;

class FastRpcController;
typedef std::shared_ptr<FastRpcController> FastRpcControllerPtr;

class FastRpcController : public google::protobuf::RpcController {
 public:
  static FastRpcControllerPtr NewController();

 public:
  FastRpcController();

  // client side
  virtual void Reset();

  virtual bool Failed() const;
  virtual std::string ErrorText() const;
  virtual void StartCancel();

  // Server side method
  virtual void SetFailed(const std::string& reason);
  virtual bool IsCanceled() const;

  virtual void NotifyOnCancel(google::protobuf::Closure* callback);

 private:
  typedef std::function<void()> CancelCallback;
  void complete();
  void set_cancel_cb(const CancelCallback& cancel_cb) noexcept {
    cancel_cb_ = cancel_cb;
  }

 private:
  bool canceled_;
  // set to true if SetFailed is called
  bool failed_;
  std::mutex cancel_mutex_;
  // set by NotifyOnCancel() method
  std::set<google::protobuf::Closure*> cancel_cbs_;
  // Will be invoked if StartCancel is called
  CancelCallback cancel_cb_;
  // the failed reason set by SetFailed() method
  std::string failed_reason_;

  friend class FastRpcChannel;
  friend class FastRpcUtil;
  friend class FastRpcServer;
};
}

#endif