// /**************************************************************************
//  *
//  * Copyright (c) 2017-2021, luotang.me <wypx520@gmail.com>, China.
//  * All rights reserved.
//  *
//  * Distributed under the terms of the GNU General Public License v2.
//  *
//  * This software is provided 'as is' with no explicit or implied warranties
//  * in respect of its properties, including, but not limited to, correctness
//  * and/or fitness for purpose.
//  *
//  **************************************************************************/
// #include "ssl_server.h"
// #include "logging.h"

// namespace MSF {

// BIO* errBio;
// SSL_CTX* g_sslCtx;

// FastTcpServer::FastTcpServer(EventLoop *loop, const InetAddress &addr)
//     : FastServer(loop, addr) {
//       initSSL();
// }

// FastTcpServer::~FastTcpServer() {
//     BIO_free(errBio);
//     SSL_CTX_free(g_sslCtx);
//     ERR_free_strings();
// }

// void initSSL() {
//     SSL_load_error_strings ();
//     int r = SSL_library_init ();
//     check0(!r, "SSL_library_init failed");
//     g_sslCtx = SSL_CTX_new (SSLv23_method ());
//     check0(g_sslCtx == NULL, "SSL_CTX_new failed");
//     errBio = BIO_new_fd(2, BIO_NOCLOSE);
//     string cert = "server.pem", key = "server.pem";
//     r = SSL_CTX_use_certificate_file(g_sslCtx, cert.c_str(),
//     SSL_FILETYPE_PEM); check0(r<=0, "SSL_CTX_use_certificate_file %s failed",
//     cert.c_str()); r = SSL_CTX_use_PrivateKey_file(g_sslCtx, key.c_str(),
//     SSL_FILETYPE_PEM); check0(r<=0, "SSL_CTX_use_PrivateKey_file %s failed",
//     key.c_str()); r = SSL_CTX_check_private_key(g_sslCtx); check0(!r,
//     "SSL_CTX_check_private_key failed"); log("SSL inited\n");
// }

// void handleHandshake(Channel* ch) {
//     if (!ch->tcpConnected_) {
//         struct pollfd pfd;
//         pfd.fd = ch->fd_;
//         pfd.events = POLLOUT | POLLERR;
//         int r = poll(&pfd, 1, 0);
//         if (r == 1 && pfd.revents == POLLOUT) {
//             log("tcp connected fd %d\n", ch->fd_);
//             ch->tcpConnected_ = true;
//             ch->events_ = EPOLLIN | EPOLLOUT | EPOLLERR;
//             ch->update();
//         } else {
//             log("poll fd %d return %d revents %d\n", ch->fd_, r,
//             pfd.revents); delete ch; return;
//         }
//     }
//     if (ch->ssl_ == NULL) {
//         ch->ssl_ = SSL_new (g_sslCtx);
//         check0(ch->ssl_ == NULL, "SSL_new failed");
//         int r = SSL_set_fd(ch->ssl_, ch->fd_);
//         check0(!r, "SSL_set_fd failed");
//         log("SSL_set_accept_state for fd %d\n", ch->fd_);
//         SSL_set_accept_state(ch->ssl_);
//     }
//     int r = SSL_do_handshake(ch->ssl_);
//     if (r == 1) {
//         ch->sslConnected_ = true;
//         log("ssl connected fd %d\n", ch->fd_);
//         return;
//     }
//     int err = SSL_get_error(ch->ssl_, r);
//     int oldev = ch->events_;
//     if (err == SSL_ERROR_WANT_WRITE) {
//         ch->events_ |= EPOLLOUT;
//         ch->events_ &= ~EPOLLIN;
//         log("return want write set events %d\n", ch->events_);
//         if (oldev == ch->events_) return;
//         ch->update();
//     } else if (err == SSL_ERROR_WANT_READ) {
//         ch->events_ |= EPOLLIN;
//         ch->events_ &= ~EPOLLOUT;
//         log("return want read set events %d\n", ch->events_);
//         if (oldev == ch->events_) return;
//         ch->update();
//     } else {
//         log("SSL_do_handshake return %d error %d errno %d msg %s\n", r, err,
//         errno, strerror(errno)); ERR_print_errors(errBio); delete ch;
//     }
// }

// void handleDataRead(Channel* ch) {
//     char buf[4096];
//     int rd = SSL_read(ch->ssl_, buf, sizeof buf);
//     int ssle = SSL_get_error(ch->ssl_, rd);
//     if (rd > 0) {
//         const char* cont = "HTTP/1.1 200 OK\r\nConnection: Close\r\n\r\n";
//         int len1 = strlen(cont);
//         int wd = SSL_write(ch->ssl_, cont, len1);
//         log("SSL_write %d bytes\n", wd);
//         delete ch;
//     }
//     if (rd < 0 && ssle != SSL_ERROR_WANT_READ) {
//         log("SSL_read return %d error %d errno %d msg %s", rd, ssle, errno,
//         strerror(errno)); delete ch; return;
//     }
//     if (rd == 0) {
//         if (ssle == SSL_ERROR_ZERO_RETURN)
//             log("SSL has been shutdown.\n");
//         else
//             log("Connection has been aborted.\n");
//         delete ch;
//     }
// }

// void handleRead(Channel* ch) {
//     if (ch->fd_ == listenfd) {
//         return handleAccept();
//     }
//     if (ch->sslConnected_) {
//         return handleDataRead(ch);
//     }
//     handleHandshake(ch);
// }

// void handleWrite(Channel* ch) {
//     if (!ch->sslConnected_) {
//         return handleHandshake(ch);
//     }
//     log("handle write fd %d\n", ch->fd_);
//     ch->events_ &= ~EPOLLOUT;
//     ch->update();
// }

// void FastTcpServer::ConnNewCallback(const int fd) {
//   LOG(INFO) << "new conn coming fd: " << fd;
//   auto conn = std::make_shared<TCPConnection>(loop(), fd);

//   static uint64_t g_conn_id = 0;
//   conn->set_cid(g_conn_id++);
//   conn->set_state(Connection::kStateConnected);
//   conn->SetConnSuccCb(std::bind(&FastTcpServer::ConnSuccCallback, this,
//   conn)); conn->SetConnReadCb(std::bind(&FastTcpServer::ConnReadCallback,
//   this, conn)); conn->SetConnWriteCb(
//       std::bind(&FastTcpServer::ConnWriteCallback, this, conn));
//   conn->SetConnCloseCb(
//       std::bind(&FastTcpServer::ConnCloseCallback, this, conn));
//   conn->AddGeneralEvent();

//   ConnSuccCallback(conn);

//   if (connected_) {

//   }
// }
// }
