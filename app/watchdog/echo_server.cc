
#include "echo_server.h"
#include <base/logging.h>

using namespace MSF;

namespace MSF {

EchoServiceImpl::EchoServiceImpl(int echo_delay, const std::string& fail_reason)
    : echo_delay_(echo_delay), fail_reason_(fail_reason) {}

void EchoServiceImpl::Echo(::google::protobuf::RpcController* controller,
                           const ::echo::EchoRequest* request,
                           ::echo::EchoResponse* response,
                           ::google::protobuf::Closure* done) {
  LOG(INFO) << "echo ===>" << request->message();
  if (fail_reason_.empty()) {
    response->set_response(request->message());
  } else {
    controller->SetFailed(fail_reason_);
  }

  if (echo_delay_ > 0) {
    // ::sleep(echo_delay_);
  }

  done->Run();
}
}