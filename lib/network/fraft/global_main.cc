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
#include <signal.h>
#include <stdio.h>
#include <base/logging.h>
#include <fraft/quorum_peer_manager.h>
#include <fraft/data_tree.h>
#include <fraft/quorum_peer_server.h>

#if 0
void Usage(char *argv[]) {
  LOG_ERROR() << "usage: " << argv[0] << "-f configfile";
}


int main_test(int argc, char *argv[]) {
  eventrpc::SetProgramName(argv[0]);
  if (argc != 3) {
    Usage(argv);
    exit(-1);
  }
  QuorumPeerManager peer_manager;
  if (!peer_manager.ParseConfigFile(argv[2])) {
    LOG_ERROR() << "parse config file " << argv[2] << " error";
    exit(-1);
  }

  eventrpc::Dispatcher dispatcher;
  dispatcher.Start();

  peer_manager.set_dispatcher(&dispatcher);
  DataTree data_tree;

  QuorumPeerServer server;
  server.set_quorumpeer_manager(&peer_manager);
  server.set_data_tree(&data_tree);
  server.set_dispatcher(&dispatcher);
  server.Start();

  sigset_t new_mask;
  sigfillset(&new_mask);
  sigset_t old_mask;
  sigset_t wait_mask;
  sigemptyset(&wait_mask);
  sigaddset(&wait_mask, SIGINT);
  sigaddset(&wait_mask, SIGQUIT);
  sigaddset(&wait_mask, SIGTERM);
  pthread_sigmask(SIG_BLOCK, &new_mask, &old_mask);
  pthread_sigmask(SIG_SETMASK, &old_mask, 0);
  pthread_sigmask(SIG_BLOCK, &wait_mask, 0);
  int sig = 0;
  sigwait(&wait_mask, &sig);
  return 0;
}
#endif