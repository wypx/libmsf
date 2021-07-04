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
#include "frpc_message.h"
#include "frpc_controller.h"
#include <base/utils.h>
#include <sstream>

namespace MSF {

FastRpcControllerPtr FastRpcController::NewController() {
  return std::make_shared<FastRpcController>();
}

FastRpcController::FastRpcController() : canceled_(false), failed_(false) {}

void FastRpcController::Reset() {
  canceled_ = false;
  failed_ = false;
  failed_reason_ = "";
  cancel_cb_ = nullptr;
  {
    std::lock_guard<std::mutex> guard(cancel_mutex_);
    cancel_cbs_.clear();
  }
}

bool FastRpcController::Failed() const { return failed_; }

std::string FastRpcController::ErrorText() const { return failed_reason_; }

void FastRpcController::StartCancel() {
  if (!cancel_cb_) {
    cancel_cb_();
  }
}

/**
  * Server side method
  */
void FastRpcController::SetFailed(const std::string& reason) {
  failed_ = true;
  failed_reason_ = reason;
}

bool FastRpcController::IsCanceled() const { return canceled_; }

void FastRpcController::NotifyOnCancel(google::protobuf::Closure* callback) {
  // return if callback is NULL
  if (!callback) {
    return;
  }

  // if already canceled, call immediately
  if (canceled_) {
    callback->Run();
  } else {
    std::lock_guard<std::mutex> guard(cancel_mutex_);
    cancel_cbs_.insert(callback);
  }
}

void FastRpcController::complete() {
  std::set<google::protobuf::Closure*> tmp;
  {
    std::lock_guard<std::mutex> guard(cancel_mutex_);
    tmp.swap(cancel_cbs_);
  }

  for (auto iter = tmp.begin(); iter != tmp.end(); iter++) {
    (*iter)->Run();
  }
}
}