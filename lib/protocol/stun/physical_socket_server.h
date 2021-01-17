/*
 *  Copyright 2004 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef RTC_BASE_PHYSICAL_SOCKET_SERVER_H_
#define RTC_BASE_PHYSICAL_SOCKET_SERVER_H_

#include <memory>
#include <set>
#include <vector>
#include <mutex>
#include <sys/epoll.h>
#define HAVE_USE_EPOLL 1

#include "net_helpers.h"
#include "socket_server.h"

#if defined(__linux__)
typedef int SOCKET;
#endif

namespace MSF {

// Event constants for the Dispatcher class.
enum DispatcherEvent {
  DE_READ = 0x0001,
  DE_WRITE = 0x0002,
  DE_CONNECT = 0x0004,
  DE_CLOSE = 0x0008,
  DE_ACCEPT = 0x0010,
};

class Signaler;
#if defined(__linux__)
class PosixSignalDispatcher;
#endif

class Dispatcher {
 public:
  virtual ~Dispatcher() {}
  virtual uint32_t GetRequestedEvents() = 0;
  virtual void OnPreEvent(uint32_t ff) = 0;
  virtual void OnEvent(uint32_t ff, int err) = 0;
#if defined(WIN32)
  virtual WSAEVENT GetWSAEvent() = 0;
  virtual SOCKET GetSocket() = 0;
  virtual bool CheckSignalClose() = 0;
#elif defined(__linux__)
  virtual int GetDescriptor() = 0;
  virtual bool IsDescriptorClosed() = 0;
#endif
};

// A socket server that provides the real sockets of the underlying OS.
class PhysicalSocketServer : public SocketServer {
 public:
  PhysicalSocketServer();
  ~PhysicalSocketServer() override;

  // SocketFactory:
  Socket* CreateSocket(int family, int type) override;
  AsyncSocket* CreateAsyncSocket(int family, int type) override;

  // Internal Factory for Accept (virtual so it can be overwritten in tests).
  virtual AsyncSocket* WrapSocket(SOCKET s);

  // SocketServer:
  bool Wait(int cms, bool process_io) override;
  void WakeUp() override;

  void Add(Dispatcher* dispatcher);
  void Remove(Dispatcher* dispatcher);
  void Update(Dispatcher* dispatcher);

#if defined(__linux__)
  // Sets the function to be executed in response to the specified POSIX signal.
  // The function is executed from inside Wait() using the "self-pipe trick"--
  // regardless of which thread receives the signal--and hence can safely
  // manipulate user-level data structures.
  // "handler" may be SIG_IGN, SIG_DFL, or a user-specified function, just like
  // with signal(2).
  // Only one PhysicalSocketServer should have user-level signal handlers.
  // Dispatching signals on multiple PhysicalSocketServers is not reliable.
  // The signal mask is not modified. It is the caller's responsibily to
  // maintain it as desired.
  virtual bool SetPosixSignalHandler(int signum, void (*handler)(int));

 protected:
  Dispatcher* signal_dispatcher();
#endif

 private:
  typedef std::set<Dispatcher*> DispatcherSet;

  void AddRemovePendingDispatchers();

#if defined(__linux__)
  bool WaitSelect(int cms, bool process_io);
  static bool InstallSignal(int signum, void (*handler)(int));

  std::unique_ptr<PosixSignalDispatcher> signal_dispatcher_;
#endif  // __linux__
#if defined(HAVE_USE_EPOLL)
  void AddEpoll(Dispatcher* dispatcher);
  void RemoveEpoll(Dispatcher* dispatcher);
  void UpdateEpoll(Dispatcher* dispatcher);
  bool WaitEpoll(int cms);
  bool WaitPoll(int cms, Dispatcher* dispatcher);

  int epoll_fd_ = -1;
  std::vector<struct epoll_event> epoll_events_;
#endif  // HAVE_USE_EPOLL
  DispatcherSet dispatchers_;
  DispatcherSet pending_add_dispatchers_;
  DispatcherSet pending_remove_dispatchers_;
  bool processing_dispatchers_ = false;
  Signaler* signal_wakeup_;
  // CriticalSection crit_;
  std::mutex mutex_;
  bool fWait_;
#if defined(WIN32)
  WSAEVENT socket_ev_;
#endif
};

class PhysicalSocket : public AsyncSocket, public has_slots<> {
 public:
  PhysicalSocket(PhysicalSocketServer* ss, SOCKET s = -1);
  ~PhysicalSocket() override;

  // Creates the underlying OS socket (same as the "socket" function).
  virtual bool Create(int family, int type);

  SocketAddress GetLocalAddress() const override;
  SocketAddress GetRemoteAddress() const override;

  int Bind(const SocketAddress& bind_addr) override;
  int Connect(const SocketAddress& addr) override;

  int GetError() const override;
  void SetError(int error) override;

  ConnState GetState() const override;

  int GetOption(Option opt, int* value) override;
  int SetOption(Option opt, int value) override;

  int Send(const void* pv, size_t cb) override;
  int SendTo(const void* buffer, size_t length,
             const SocketAddress& addr) override;

  int Recv(void* buffer, size_t length, int64_t* timestamp) override;
  int RecvFrom(void* buffer, size_t length, SocketAddress* out_addr,
               int64_t* timestamp) override;

  int Listen(int backlog) override;
  AsyncSocket* Accept(SocketAddress* out_addr) override;

  int Close() override;

  SocketServer* socketserver() { return ss_; }

 protected:
  int DoConnect(const SocketAddress& connect_addr);

  // Make virtual so ::accept can be overwritten in tests.
  virtual SOCKET DoAccept(SOCKET socket, sockaddr* addr, socklen_t* addrlen);

  // Make virtual so ::send can be overwritten in tests.
  virtual int DoSend(SOCKET socket, const char* buf, int len, int flags);

  // Make virtual so ::sendto can be overwritten in tests.
  virtual int DoSendTo(SOCKET socket, const char* buf, int len, int flags,
                       const struct sockaddr* dest_addr, socklen_t addrlen);

  // void OnResolveResult(AsyncResolverInterface* resolver);

  void UpdateLastError();
  void MaybeRemapSendError();

  uint8_t enabled_events() const { return enabled_events_; }
  virtual void SetEnabledEvents(uint8_t events);
  virtual void EnableEvents(uint8_t events);
  virtual void DisableEvents(uint8_t events);

  static int TranslateOption(Option opt, int* slevel, int* sopt);

  PhysicalSocketServer* ss_;
  SOCKET s_;
  bool udp_;
  mutable std::mutex mutex_;
  // CriticalSection crit_;
  int error_;
  ConnState state_;
// AsyncResolver* resolver_;

#if !defined(NDEBUG)
  std::string dbg_addr_;
#endif

 private:
  uint8_t enabled_events_ = 0;
};

class SocketDispatcher : public Dispatcher, public PhysicalSocket {
 public:
  explicit SocketDispatcher(PhysicalSocketServer* ss);
  SocketDispatcher(SOCKET s, PhysicalSocketServer* ss);
  ~SocketDispatcher() override;

  bool Initialize();

  virtual bool Create(int type);
  bool Create(int family, int type) override;

#if defined(WIN32)
  WSAEVENT GetWSAEvent() override;
  SOCKET GetSocket() override;
  bool CheckSignalClose() override;
#elif defined(__linux__)
  int GetDescriptor() override;
  bool IsDescriptorClosed() override;
#endif

  uint32_t GetRequestedEvents() override;
  void OnPreEvent(uint32_t ff) override;
  void OnEvent(uint32_t ff, int err) override;

  int Close() override;

#if defined(HAVE_USE_EPOLL)
 protected:
  void StartBatchedEventUpdates();
  void FinishBatchedEventUpdates();

  void SetEnabledEvents(uint8_t events) override;
  void EnableEvents(uint8_t events) override;
  void DisableEvents(uint8_t events) override;
#endif

 private:
#if defined(WIN32)
  static int next_id_;
  int id_;
  bool signal_close_;
  int signal_err_;
#endif  // WIN32

#if defined(HAVE_USE_EPOLL)
  void MaybeUpdateDispatcher(uint8_t old_events);

  int saved_enabled_events_ = -1;
#endif
};

}  // namespace rtc

#endif  // RTC_BASE_PHYSICAL_SOCKET_SERVER_H_
