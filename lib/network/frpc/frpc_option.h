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
#ifndef FRPC_OPTION_H_
#define FRPC_OPTION_H_

namespace MSF {

struct RPCTaskParams {
  int send_timeout_;
  int watch_timeout_;
  int keep_alive_timeout_;
  int retry_max_;
  int compress_type_;  // FRPCCompressType
  int data_type;       // FRPCDataType
};

struct RPCClientParams {
  RPCTaskParams task_params;
  _
      // host + port + is_ssl
      std::string host_;
  unsigned short port_;
  bool is_ssl_;
  // or URL
  std::string url_;
};

struct RpcChannelOptions {
  // Connect timeout (in seconds).
  // If a request can't get an healthy connection after a connect_timeout
  // milliseconds, it will fail and return to the caller.
  //
  // Not supported now.
  int64_t connect_timeout;

  //////////// The following options are only useful for multiple servers.
  /////////////
  // Server load capacity, which limits the max request count pending on one
  // channel.
  // If all servers exceed the limit, calling will returns
  // RPC_ERROR_SERVER_BUSY.
  // Value 0 means no limit, default value is 0.
  uint32_t server_load_capacity;

  // If initialize the RpcChannel in construct function, default is true.
  // If create_with_init is false, RpcChannel should be initialized by calling
  // Init().
  bool create_with_init;

  RpcChannelOptions()
      : connect_timeout(10), server_load_capacity(0), create_with_init(true) {}
};
}
#endif