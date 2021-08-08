
#include "proxy_server.h"

void ProxyServer::DebugInfo() {
  std::cout << std::endl;
  std::cout << "Agent Server Options:" << std::endl;
  std::cout << " -c, --conf=FILE        The configure file." << std::endl;
  std::cout << " -s, --host=HOST        The host that server will listen on"
            << std::endl;
  std::cout << " -p, --port=PORT        The port that server will listen on"
            << std::endl;
  std::cout << " -d, --daemon           Run as daemon." << std::endl;
  std::cout << " -g, --signal           restart|kill|reload" << std::endl;
  std::cout << "                        restart: restart process graceful"
            << std::endl;
  std::cout << "                        kill: fast shutdown graceful"
            << std::endl;
  std::cout << "                        reload: parse config file and reinit"
            << std::endl;
  std::cout << " -v, --version          Print the version." << std::endl;
  std::cout << " -h, --help             Print help info." << std::endl;
  std::cout << std::endl;

  std::cout << "Examples:" << std::endl;
  std::cout
      << " ./ProxyServer -c ProxyServer.conf -s 127.0.0.1 -p 8888 -d -g kill"
      << std::endl;

  std::cout << std::endl;
  std::cout << "Reports bugs to <luotang.me>" << std::endl;
  std::cout << "Commit issues to <https://github.com/wypx/libmsf>" << std::endl;
  std::cout << std::endl;
}
