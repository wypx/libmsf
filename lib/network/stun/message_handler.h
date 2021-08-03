/*
 *  Copyright 2004 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef RTC_BASE_MESSAGE_HANDLER_H_
#define RTC_BASE_MESSAGE_HANDLER_H_

#include <utility>
#include "noncopyable.h"

namespace MSF {

struct Message;

// Messages get dispatched to a MessageHandler
class MessageHandler : public noncopyable {
 public:
  virtual ~MessageHandler();
  virtual void OnMessage(Message* msg) = 0;

 protected:
  MessageHandler() {}
};

}  // namespace rtc

#endif  // RTC_BASE_MESSAGE_HANDLER_H_
