#include <base/os.h>
#include <base/signal_manager.h>
#include <base/thread.h>
#include <base/version.h>
#include <event/event_loop.h>
#include <event/event_stack.h>

#include <cassert>
#include <base/logging.h>

#include <event/timer.h>

#include "../watchdog/echo.pb.h"
#include <network/event/event_loop.h>
#include <network/frpc/frpc_controller.h>
#include <network/frpc/frpc_channel.h>
#include <upnp/upnp.h>

using namespace MSF;

/* 这个表示一个方法的返回值只由参数决定, 如果参数不变的话,
    就不再调用此函数，直接返回值.经过我的尝试发现还是调用了,
    后又经查资料发现要给gcc加一个-O的参数才可以.
    是对函数调用的一种优化*/
__attribute__((const)) int test2() { return 5; }

/* 表示函数的返回值必须被检查或使用,否则会警告*/
__attribute__((unused)) int test3() { return 5; }

/* 这段代码能够保证代码是内联的,因为你如果只定义内联的话,
        编译器并不一定会以内联的方式调用,
        如果代码太多你就算用了内联也不一定会内联用了这个的话会强制内联*/
static inline __attribute__((always_inline)) void test5() {}

__attribute__((destructor)) void after_main() {
  //    ("after main.");
}

void fun1() {
  LOG(FATAL) << "Hello Timer 1";
  return;
}

void fun2() {
  LOG(INFO) << "Hello Timer 2";
  return;
}

#include <chrono>
#include <ctime>
#include <iostream>
#include <ratio>



int EchoTest() {
  EventStack *stack = new EventStack();

  std::vector<struct StackArg> args;
  args.push_back(std::move(StackArg("frpc")));
  stack->SetStackArgs(args);
  stack->StartStack();

  EventLoop *base_loop = stack->GetBaseLoop();
    
  InetAddress addr("127.0.0.1", 9999, AF_INET, SOCK_STREAM);
  FastRpcChannelPtr channel(new FastRpcChannel(base_loop, addr));

  echo::EchoRequest request;
  echo::EchoResponse response;

  request.set_message("hello word");

  echo::EchoService::Stub stub(channel.get());

  FastRpcController controller;
  stub.Echo(&controller, &request, &response, nullptr);

  if (controller.Failed()) {
    LOG(ERROR) << "failed to send echo";
    return -1;
  }

  LOG(INFO) << "Received: " << response.response().size() << " bytes";
  LOG(INFO) << "Received: " << response.response();

  UPnPManager upnp;
  upnp.Pluse(1111, true, true);

  stack->StartMain();
  return 0;
}

int main(int argc, char* argv[]) {
  MSF::BuildInfo();

  EchoTest();

  // std::string logFile_ = "/var/log/luotang.me/Shell.log";
  // assert(Logger::getLogger().init(logFile_.c_str()));

  LOG(INFO) << "Hello world1."
            << "yyyyyyyy"
            << ":" << 2;

  LOG(TRACE) << "Hello world2.";
  LOG(WARNING) << "Hello world3.";
  LOG(ERROR) << "Hello world4.";
  LOG(FATAL) << "Hello world5.";

  // msf_os_init();

  GetExecuteTime([]() { sleep(5); });

  MSF::msf_time_init();
  // LOG_WARN << "Time str: " << MSF::TIME::GetCurrentTimeString(NULL);
  // EventLoop loop = EventLoop();

  // loop.runAfter(2000, fun1);

  // loop.runEvery(5000, fun2);

  // loop.runAfter(2000, fun1);

  // loop.runAfter(2000, fun1);

  // loop.loop();

  return 0;
}
