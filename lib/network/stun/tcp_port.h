/*
 *  Copyright 2004 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef P2P_BASE_TCP_PORT_H_
#define P2P_BASE_TCP_PORT_H_

#include <list>
#include <memory>
#include <string>

#include "connection.h"
#include "port.h"
#include "async_packet_socket.h"
#include "event_loop.h"

namespace MSF {

class TCPConnection;

// Communicates using a local TCP port.
//
// This class is designed to allow subclasses to take advantage of the
// connection management provided by this class.  A subclass should take of all
// packet sending and preparation, but when a packet is received, it should
// call this TCPPort::OnReadPacket (3 arg) to dispatch to a connection.
class TCPPort : public Port, public std::enable_shared_from_this<TCPPort> {
 public:
  static std::unique_ptr<TCPPort> Create(
      EventLoop* loop, PacketSocketFactory* factory, Network* network,
      uint16_t min_port, uint16_t max_port, const std::string& username,
      const std::string& password, bool allow_listen) {
    // Using `new` to access a non-public constructor.
    return std::make_unique<TCPPort>(loop, factory, network, min_port, max_port,
                                     username, password, allow_listen);
  }
  ~TCPPort() override;

  Connection* CreateConnection(const Candidate& address,
                               CandidateOrigin origin) override;

  void PrepareAddress() override;

  int GetOption(Socket::Option opt, int* value) override;
  int SetOption(Socket::Option opt, int value) override;
  int GetError() override;
  bool SupportsProtocol(const std::string& protocol) const override;
  ProtocolType GetProtocol() const override;

  TCPPort(EventLoop* loop, PacketSocketFactory* factory, Network* network,
          uint16_t min_port, uint16_t max_port, const std::string& username,
          const std::string& password, bool allow_listen);

 protected:
  // Handles sending using the local TCP socket.
  int SendTo(const void* data, size_t size, const SocketAddress& addr,
             const PacketOptions& options, bool payload) override;

  // Accepts incoming TCP connection.
  void OnNewConnection(AsyncPacketSocket* socket,
                       AsyncPacketSocket* new_socket);

 private:
  struct Incoming {
    SocketAddress addr;
    AsyncPacketSocket* socket;
  };

  void TryCreateServerSocket();

  AsyncPacketSocket* GetIncoming(const SocketAddress& addr,
                                 bool remove = false);

  // Receives packet signal from the local TCP Socket.
  void OnReadPacket(AsyncPacketSocket* socket, const char* data, size_t size,
                    const SocketAddress& remote_addr,
                    const int64_t& packet_time_us);

  void OnSentPacket(AsyncPacketSocket* socket,
                    const SentPacket& sent_packet) override;

  void OnReadyToSend(AsyncPacketSocket* socket);

  void OnAddressReady(AsyncPacketSocket* socket, const SocketAddress& address);

  bool allow_listen_;
  AsyncPacketSocket* socket_;
  int error_;
  std::list<Incoming> incoming_;

  friend class TCPConnection;
};

class TCPConnection : public Connection {
 public:
  // Connection is outgoing unless socket is specified
  TCPConnection(TCPPort* port, const Candidate& candidate,
                AsyncPacketSocket* socket = 0);
  ~TCPConnection() override;

  int Send(const void* data, size_t size,
           const PacketOptions& options) override;
  int GetError() override;

  AsyncPacketSocket* socket() { return socket_.get(); }

  void OnMessage(Message* pmsg) override;

  // Allow test cases to overwrite the default timeout period.
  int reconnection_timeout() const { return reconnection_timeout_; }
  void set_reconnection_timeout(int timeout_in_ms) {
    reconnection_timeout_ = timeout_in_ms;
  }

 protected:
  enum {
    MSG_TCPCONNECTION_DELAYED_ONCLOSE = Connection::MSG_FIRST_AVAILABLE,
    MSG_TCPCONNECTION_FAILED_CREATE_SOCKET,
  };

  // Set waiting_for_stun_binding_complete_ to false to allow data packets in
  // addition to what Port::OnConnectionRequestResponse does.
  void OnConnectionRequestResponse(ConnectionRequest* req,
                                   StunMessage* response) override;

 private:
  // Helper function to handle the case when Ping or Send fails with error
  // related to socket close.
  void MaybeReconnect();

  void CreateOutgoingTcpSocket();

  void ConnectSocketSignals(AsyncPacketSocket* socket);

  void OnConnect(AsyncPacketSocket* socket);
  void OnClose(AsyncPacketSocket* socket, int error);
  void OnReadPacket(AsyncPacketSocket* socket, const char* data, size_t size,
                    const SocketAddress& remote_addr,
                    const int64_t& packet_time_us);
  void OnReadyToSend(AsyncPacketSocket* socket);

  std::unique_ptr<AsyncPacketSocket> socket_;
  int error_;
  bool outgoing_;

  // Guard against multiple outgoing tcp connection during a reconnect.
  bool connection_pending_;

  // Guard against data packets sent when we reconnect a TCP connection. During
  // reconnecting, when a new tcp connection has being made, we can't send data
  // packets out until the STUN binding is completed (i.e. the write state is
  // set to WRITABLE again by Connection::OnConnectionRequestResponse). IPC
  // socket, when receiving data packets before that, will trigger OnError which
  // will terminate the newly created connection.
  bool pretending_to_be_writable_;

  // Allow test case to overwrite the default timeout period.
  int reconnection_timeout_;

  friend class TCPPort;
};

}  // namespace cricket

#endif  // P2P_BASE_TCP_PORT_H_
