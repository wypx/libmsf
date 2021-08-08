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

#include "frpc.pb.h"

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

  // -------- used only by client side ---------
  // These calls should be made from the client side only.  Their results are
  // undefined on the server side (may crash).

  // Resets the RpcController to its initial state so that it may be reused
  // in a new call. Must not be called while it is in use.
  virtual void Reset();

  // Set expect timeout in milli-seconds of the call.  If timeout is not set
  // or set no more than 0, actual timeout will be taken from proto options.
  void SetTimeout(int64_t timeout_in_ms);

  // Get the actual timeout in milli-seconds.
  //
  // The actual timeout takes effect in the following order:
  // * set in RpcController (take effective only when timeout > 0)
  // * set in the method proto options (default not set)
  // * set in the service proto options (default value is 10 seconds)
  int64_t Timeout() const;

  // Set compress type of the request message.
  // Supported types:
  //   CompressTypeNone
  //   CompressTypeGzip
  //   CompressTypeZlib
  //   CompressTypeSnappy
  //   CompressTypeLZ4
  void SetRequestCompressType(frpc::CompressType compress_type);

  // Set expected compress type of the response message.
  // Supported types:
  //   CompressTypeNone
  //   CompressTypeGzip
  //   CompressTypeZlib
  //   CompressTypeSnappy
  //   CompressTypeLZ4
  void SetResponseCompressType(frpc::CompressType compress_type);

  // After a call has finished, returns true if the call failed.  The possible
  // reasons for failure depend on the RPC implementation.  Failed() must not
  // be called before a call has finished.  If Failed() returns true, the
  // contents of the response message are undefined.
  //
  // This method can only be called after the call has finished.
  virtual bool Failed() const;

  // If Failed() is true, returns error code which identities the reason.
  // The error code is of type RpcErrorCode.
  //
  // This method can only be called after the call has finished.
  virtual int ErrorCode() const;

  // If Failed() is true, returns a human-readable description of the error.
  // This can only be called after the call has finished.
  //
  // This method can only be called after the call has finished.
  virtual std::string ErrorText() const;

  // If the request has already been set to the remote server, returns true;
  // otherwise returns false.
  //
  // This method can only be called after the call has finished.
  bool IsRequestSent() const;

  // If IsRequestSent() is true, returns sent bytes, including the rpc header.
  //
  // This method can only be called after the call has finished.
  int64_t SentBytes() const;

  // Advises the RPC system that the caller desires that the RPC call be
  // canceled.  The RPC system may cancel it immediately, may wait awhile and
  // then cancel it, or may not even cancel the call at all.  If the call is
  // canceled, the "done" callback will still be called and the RpcController
  // will indicate that the call failed at that time.
  //
  // Not supported now.
  virtual void StartCancel();

  // Causes Failed() to return true on the client side.  "reason" will be
  // incorporated into the message returned by ErrorText().  If you find
  // you need to return machine-readable information about failures, you
  // should incorporate it into your response protocol buffer and should
  // NOT call SetFailed().
  virtual void SetFailed(const std::string& reason);

  // If true, indicates that the client canceled the RPC, so the server may
  // as well give up on replying to it.  The server should still call the
  // final "done" callback.
  virtual bool IsCanceled() const;

  // Asks that the given callback be called when the RPC is canceled.  The
  // callback will always be called exactly once.  If the RPC completes without
  // being canceled, the callback will be called after completion.  If the RPC
  // has already been canceled when NotifyOnCancel() is called, the callback
  // will be called immediately.
  //
  // NotifyOnCancel() must be called no more than once per request.
  virtual void NotifyOnCancel(google::protobuf::Closure* callback);

  // -------- used by both client and server side ---------
  // These calls can be made from both client side and server side.

  // Get the local address in format of "ip:port".
  //
  // For client:
  // This method can only be called after the call has finished.  If
  // IsRequestSent() is true, returns the local address used for sending
  // the message; else, the return value is undefined.
  //
  // For server:
  // This method returns the local address where the message received from.
  std::string LocalAddress() const;

  // Get the remote address in format of "ip:port".
  //
  // For client:
  // This method returns the remote address where the messsage sent to.
  //
  // For server:
  // This method returns the remote address where the message received from.
  std::string RemoteAddress() const;

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