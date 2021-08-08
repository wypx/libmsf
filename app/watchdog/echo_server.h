#ifndef ECHOTESTSERVER_HPP
#define ECHOTESTSERVER_HPP

#include <string>

#include "echo.pb.h"
#include "frpc/frpc_server.h"

using namespace MSF;

namespace MSF {

class EchoServiceImpl : public echo::EchoService {
 public:
  EchoServiceImpl(int echo_delay,
                  const std::string& fail_reason = std::string());
  virtual void Echo(google::protobuf::RpcController* controller,
                    const echo::EchoRequest* request,
                    echo::EchoResponse* response,
                    google::protobuf::Closure* done);

 private:
  // echo delay in seconds
  int echo_delay_;

  // if not empty, send an error response
  std::string fail_reason_;
};
}

#endif /* ECHOTESTSERVER_HPP */
