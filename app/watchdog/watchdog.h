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
#ifndef GUARD_GUARD_H
#define GUARD_GUARD_H

#include <Client/AgentClient.h>
#include <event/event_loop.h>
#include <event/event_stack.h>

#include <list>

struct Process;

using namespace MSF;

namespace MSF {

class Guard {
 private:
  EventStack* stack_;
  // AgentClient* agent_;
  std::string logFile_;
  // std::list<Process*> processes_;
 private:
  bool loadConfig();

 public:
  Guard();
  ~Guard();
  void start();
  void sendPdu();
};

}  // namespace MSF
#endif