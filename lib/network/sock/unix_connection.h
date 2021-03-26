
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

#include "connection.h"

namespace MSF {

class UNIXConnection : public Connection {
  UNIXConnection() = default;
  ~UNIXConnection() = default;

  bool HandleReadEvent() override;
  bool HandleWriteEvent() override;
  void HandleErrorEvent() override;

  void CloseConn() override;
};

}  // namespace MSF