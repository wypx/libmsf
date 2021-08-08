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
#ifndef FRPC_MESSAGE_H_
#define FRPC_MESSAGE_H_

#include <google/protobuf/service.h>
#include <google/protobuf/message.h>
#include <google/protobuf/descriptor.h>
#include <string>
#include <iostream>

#include "frpc.pb.h"

namespace MSF {

class FastRPCMessage {
 public:
  static const google::protobuf::MethodDescriptor* GetMethodDescriptor(
      const std::string& name) {
    return google::protobuf::DescriptorPool::generated_pool()->FindMethodByName(
        name);
  }

  static const google::protobuf::Descriptor* GetMessageTypeByName(
      const std::string& name) {
    return google::protobuf::DescriptorPool::generated_pool()
        ->FindMessageTypeByName(name);
  }

  static google::protobuf::Message* NewGoogleMessage(
      const google::protobuf::Descriptor* desc) {
    auto msg = google::protobuf::MessageFactory::generated_factory()
                   ->GetPrototype(desc)
                   ->New();
    return msg;
  }
};
}
#endif