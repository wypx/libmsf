#ifndef P2P_BASE_ASYNC_STUN_TCP_SOCKET_H_
#define P2P_BASE_ASYNC_STUN_TCP_SOCKET_H_

#include <stddef.h>

#include "async_packet_socket.h"
#include "async_socket.h"
#include "async_tcp_socket.h"
#include "socket_address.h"
#include "noncopyable.h"

namespace MSF {

class AsyncStunTCPSocket : public AsyncTCPSocketBase {
 public:
  // Binds and connects |socket| and creates AsyncTCPSocket for
  // it. Takes ownership of |socket|. Returns NULL if bind() or
  // connect() fail (|socket| is destroyed in that case).
  static AsyncStunTCPSocket* Create(AsyncSocket* socket,
                                    const SocketAddress& bind_address,
                                    const SocketAddress& remote_address);

  AsyncStunTCPSocket(AsyncSocket* socket, bool listen);

  int Send(const void* pv, size_t cb, const PacketOptions& options) override;
  void ProcessInput(char* data, size_t* len) override;
  void HandleIncomingConnection(AsyncSocket* socket) override;

 private:
  // This method returns the message hdr + length written in the header.
  // This method also returns the number of padding bytes needed/added to the
  // turn message. |pad_bytes| should be used only when |is_turn| is true.
  size_t GetExpectedLength(const void* data, size_t len, int* pad_bytes);
};

}  // namespace MSF

#endif  // P2P_BASE_ASYNC_STUN_TCP_SOCKET_H_
