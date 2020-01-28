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
#ifndef __MSF_Connector_H__
#define __MSF_Connector_H__

#include <deque>
#include <vector>
#include <mutex>
#include <memory>
#include <functional>

#include <sock/Socket.h>
#include <event/Event.h>
#include <event/EventLoop.h>

using namespace MSF::SOCK;
using namespace MSF::EVENT;

namespace MSF {
namespace SOCK {

//http://itindex.net/detail/53322-rpc-%E6%A1%86%E6%9E%B6-bangerlee
/**
 * load balance: retry for timeout
 * client maintain a star mark for server(ip and port)
 * when request fail or timeout, dec the star by one,
 * since the star eaual as zero, add the ip, port, time
 * into the local info, realse the restict some times later
 * 
 * server info can upload to zookeeper, then share in cluster
 * when new a server node, can distribute this to clients.
 */

enum ConnState {
    CONN_STATE_DEFAULT           = 1,
    CONN_STATE_UNAUTHORIZED      = 2,
    CONN_STATE_AUTHORIZED        = 3,
    CONN_STATE_LOGINED           = 4,
    CONN_STATE_UPGRADE           = 5,
    CONN_STATE_CLOSED            = 6,
    CONN_STATE_BUTT
};

class Connector;
typedef std::shared_ptr<Connector> ConnectionPtr;

class Connector : public std::enable_shared_from_this<Connector> 
{
public:
    Connector();
    virtual ~Connector();

    void close();
    bool connect(const std::string &host,
                const uint16_t port,
                const uint32_t proto);

    bool connect(const std::string & srvPath, const std::string & cliPath);

    /* Connector and Accpetor connection use common structure */
    void init(EventLoop *loop, const int fd = -1)
    {
      fd_ = fd;
      event_.init(loop, fd);
    }

    void setSuccCallback(const EventCallback & cb)
    { 
      event_.setSuccCallback(cb);
    }
    void setReadCallback(const EventCallback & cb)
    { 
      event_.setReadCallback(cb);
    }
    void setWriteCallback(const EventCallback & cb)
    { 
      event_.setWriteCallback(cb);
    }
    void setCloseCallback(const EventCallback & cb)
    { 
      event_.setCloseCallback(cb);
    }
    void setErrorCallback(const EventCallback & cb)
    { 
      event_.setErrorCallback(cb);
    }

    void enableEvent()
    {
      event_.enableReading();
      event_.enableClosing();
    }

    void enableWriting()
    {
      event_.enableWriting();
    }
    void disableWriting()
    {
      event_.disableWriting();
    }
  
    const int fd() const { return fd_; }
    void setCid(const uint32_t cid) { cid_ = cid; }
    const uint32_t cid() const { return cid_; }

  protected:
    Event             event_;

    std::string       name_;
    /*key is used to find conn by cid*/
    std::string       key_;
    /* unique identifier given by agent server when login */
    uint32_t          cid_;
    int               fd_;
    uint32_t          state_;

    uint64_t          lastActiveTime_;
    // AgentBhs          bhs_;
    void updateActiveTime();
};

}
}
#endif
