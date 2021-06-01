
#include "echo_server.h"

using namespace MSF;

namespace MSF {

EchoServiceImpl::EchoServiceImpl(int echo_delay, const std::string& fail_reason)
: echo_delay_(echo_delay), fail_reason_(fail_reason) {

}

void EchoServiceImpl::Echo(::google::protobuf::RpcController* controller,
        const ::echo::EchoRequest* request,
        ::echo::EchoResponse* response,
        ::google::protobuf::Closure* done) {
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


void StartEchoServer(EventLoop *loop, const std::string &addr, uint16_t port) {
  std::cout << "server port : " << port << std::endl;
  std::shared_ptr<ShmRpcServer> rpc_server(new ShmRpcServer(loop, "localhost"));
  EchoServiceAdapter<ShmRpcServer>::type test_server(rpc_server, 0);
  
  test_server.Start();
  
//  pbrpcpp::RpcController controller;
//  
//  string addr, port;
//  while( ! rpcServer->getLocalEndpoint( addr, port ) );
//  
//  shared_ptr<pbrpcpp::TcpRpcChannel> channel( new pbrpcpp::TcpRpcChannel( addr, port ) );
//  EchoTestClient client( channel );
//  echo::EchoRequest request;
//  echo::EchoResponse response;
//  
//  request.set_message("hello, world");
//  client.echo( &controller, &request,&response, NULL, 5000 );
//  
//  EXPECT_FALSE( controller.Failed() );
//  EXPECT_EQ( response.response(), "hello, world");
//   int temp;
//   std::cin >> temp;
  
  test_server.Stop();
}

}