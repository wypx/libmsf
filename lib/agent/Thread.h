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
#ifndef __MSF_CLI_THREAD_H__
#define __MSF_CLI_THREAD_H__

#include <iostream>
#include <string.h>
#include <list>
#include <mutex>
#include <condition_variable>
#include <agent/Protocol.h>

using namespace MSF::AGENT;

namespace MSF {
namespace AGENT {


class CliRxThread 
{
    public:
        CliRxThread() = default;
        ~CliRxThread();

    private:
        struct AgentBhs bhs;
        void readBhs();
        void handleBhs();
        void handleRet();
        void handleReq(struct cmd *_cmd);
        void handleRsp(struct cmd *_cmd);
        void handleAckData(struct cmd *_cmd);
        void handleAckRsp(struct cmd *_cmd);
};

class CliTxThread 
{
    public:

    private:
        pthread_t       tid;        /* unique ID id of this thread */
        int             idx;
        std::string     name;

        std::mutex      tx_cmd_lock;
        std::condition_variable sem;
        std::list<struct cmd *> tx_cmd_list; /* queue of tx cmd to handle*/

        void handleRet(int iRet)
        {
            
        }
        void addCmd2TxList(struct cmd *_cmd);
        void handleTxList();

};

}
}
#endif