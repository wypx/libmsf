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
#ifndef P2P_SERVER_SERVER_H
#define P2P_SERVER_SERVER_H

#include <base/Noncopyable.h>
#include <base/Plugin.h>
#include <base/ThreadPool.h>
#include <base/mem/Buffer.h>
#include <base/mem/MemPool.h>
#include <event/Event.h>
#include <event/EventLoop.h>
#include <event/EventStack.h>
#include <agent/client/AgentConn.h>
#include <sock/Acceptor.h>

using namespace MSF::BASE;
using namespace MSF::SOCK;
using namespace MSF::PLUGIN;
using namespace MSF::EVENT;
using namespace MSF::AGENT;

namespace MSF {
namespace P2P {

class TurnSess;
typedef std::unique_ptr<TurnSess> TurnSessPtr;

class TurnTransport {

};

class TurnRelay {
    /** Transport type. */
    int tpType_;
    std::string user_; /** Username used in credential */
    std::string	realm_; /** Realm used in credential. */
    uint32_t lifeTime_; /** Lifetime, in seconds. */
    uint32_t timerId_; /** Relay/allocation expiration timer id */
    int fd_; /** Transport/relay socket */
};

class TurnAlloction {
  private:
    std::string info_;      /** Client info (IP address and port) */
    TurnTransport *trans_;

    TurnRelay relay_;  /** The relay resource for this allocation. */
    TurnRelay *resv_;  /** Relay resource reserved by this allocation, if any */
    TurnSessPtr sess_;
    // pj_stun_auth_cred	 cred; /** Credential for this STUN session. */

    uint32_t bandWidth_;       /** Requested bandwidth */
};

class TurnServer {
  private:
    bool started_;
    bool quit_;
    MemPool *mpool_;
    ThreadPool *pool_;
    EventStack *stack_;

    std::list<AcceptorPtr> actors_;

    /* Mutiple connections supported, such as tcp, udp, unix, event fd and etc*/
    std::map<Agent::AppId, TurnSessPtr> activeConns_;

    /* The default initial STUN round-trip time estimation in msecs.
     * The value normally is PJ_STUN_RTO_VALUE.*/
    uint32_t rtoMsec_;

    /* The interval to cache outgoing  STUN response in the STUN session,
     * in miliseconds. Default 10000 (10 seconds).*/
    uint32_t resCacheMsec_;

    /** Minimum UDP port number. */
    uint16_t	    minUdpPort_;
    /** Maximum UDP port number. */
    uint16_t	    maxUdpPort_;
    /** Next UDP port number. */
    uint16_t	    nextUdpPort_;
    /** Minimum TCP port number. */
    uint16_t	    minTcpPort_;
    /** Maximum TCP port number. */
    uint16_t	    maxTcpPort_;
    /** Next TCP port number. */
    uint16_t	    nextTcpPort_;

  public:
    bool init();
    bool stop();
    bool registAllocation(TurnAlloction *alloc);
    bool unregistAllocation(TurnAlloction *alloc);
};

}
}
#endif