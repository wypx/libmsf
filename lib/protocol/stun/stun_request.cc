/*
 *  Copyright 2004 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "stun_request.h"

#include <algorithm>
#include <memory>
#include <vector>
#include <butil/logging.h>

#include "helpers.h"
#include "string_encode.h"
#include "time_utils.h"  // For TimeMillis
#include "random.h"
// #include "field_trial.h"

namespace MSF {

const uint32_t MSG_STUN_SEND = 1;

// RFC 5389 says SHOULD be 500ms.
// For years, this was 100ms, but for networks that
// experience moments of high RTT (such as 2G networks), this doesn't
// work well.
const int STUN_INITIAL_RTO = 250;  // milliseconds

// The timeout doubles each retransmission, up to this many times
// RFC 5389 says SHOULD retransmit 7 times.
// This has been 8 for years (not sure why).
const int STUN_MAX_RETRANSMISSIONS = 8;  // Total sends: 9

// We also cap the doubling, even though the standard doesn't say to.
// This has been 1.6 seconds for years, but for networks that
// experience moments of high RTT (such as 2G networks), this doesn't
// work well.
const int STUN_MAX_RTO = 8000;  // milliseconds, or 5 doublings

StunRequestManager::StunRequestManager(EventLoop* loop) {}

StunRequestManager::~StunRequestManager() {
  while (requests_.begin() != requests_.end()) {
    StunRequest* request = requests_.begin()->second;
    requests_.erase(requests_.begin());
    delete request;
  }
}

void StunRequestManager::Send(StunRequest* request) { SendDelayed(request, 0); }

void StunRequestManager::SendDelayed(StunRequest* request, int delay) {
  request->set_manager(this);
  if (requests_.find(request->id()) != requests_.end()) {
    LOG(WARNING) << "Stun request id is duplicate, ignore";
    return;
  }
  request->set_origin(origin_);
  request->Construct();
  requests_[request->id()] = request;
  if (delay > 0) {
    // thread_->PostDelayed(RTC_FROM_HERE, delay, request, MSG_STUN_SEND, NULL);
  } else {
    // thread_->Send(RTC_FROM_HERE, request, MSG_STUN_SEND, NULL);
    request->send_stun_message();
  }
}

void StunRequestManager::Flush(int msg_type) {
  for (const auto& kv : requests_) {
    StunRequest* request = kv.second;
    if (msg_type == kAllRequests || msg_type == request->type()) {
      // thread_->Clear(request, MSG_STUN_SEND);
      // thread_->Send(RTC_FROM_HERE, request, MSG_STUN_SEND, NULL);
    }
  }
}

bool StunRequestManager::HasRequest(int msg_type) {
  for (const auto& kv : requests_) {
    StunRequest* request = kv.second;
    if (msg_type == kAllRequests || msg_type == request->type()) {
      return true;
    }
  }
  return false;
}

void StunRequestManager::Remove(StunRequest* request) {
  assert(request->manager() == this);
  RequestMap::iterator iter = requests_.find(request->id());
  if (iter != requests_.end()) {
    assert(iter->second == request);
    requests_.erase(iter);
    // thread_->Clear(request);
  }
}

void StunRequestManager::Clear() {
  std::vector<StunRequest*> requests;
  for (RequestMap::iterator i = requests_.begin(); i != requests_.end(); ++i)
    requests.push_back(i->second);

  for (uint32_t i = 0; i < requests.size(); ++i) {
    // StunRequest destructor calls Remove() which deletes requests
    // from |requests_|.
    delete requests[i];
  }
}

bool StunRequestManager::CheckResponse(StunMessage* msg) {
  RequestMap::iterator iter = requests_.find(msg->transaction_id());
  if (iter == requests_.end()) {
    // TODO(pthatcher): Log unknown responses without being too spammy
    // in the logs.
    return false;
  }

  StunRequest* request = iter->second;
  if (!msg->GetNonComprehendedAttributes().empty()) {
    // If a response contains unknown comprehension-required attributes, it's
    // simply discarded and the transaction is considered failed. See RFC5389
    // sections 7.3.3 and 7.3.4.
    LOG(ERROR) << ": Discarding response due to unknown "
                  "comprehension-required attribute.";
    delete request;
    return false;
  } else if (msg->type() == GetStunSuccessResponseType(request->type())) {
    request->OnResponse(msg);
  } else if (msg->type() == GetStunErrorResponseType(request->type())) {
    request->OnErrorResponse(msg);
  } else {
    LOG(ERROR) << "Received response with wrong type: " << msg->type()
               << " (expecting " << GetStunSuccessResponseType(request->type())
               << ")";
    return false;
  }

  delete request;
  return true;
}

bool StunRequestManager::CheckResponse(const char* data, size_t size) {
  // Check the appropriate bytes of the stream to see if they match the
  // transaction ID of a response we are expecting.

  if (size < 20) return false;

  std::string id;
  id.append(data + kStunTransactionIdOffset, kStunTransactionIdLength);

  RequestMap::iterator iter = requests_.find(id);
  if (iter == requests_.end()) {
    // TODO(pthatcher): Log unknown responses without being too spammy
    // in the logs.
    return false;
  }

  // Parse the STUN message and continue processing as usual.

  ByteBufferReader buf(data, size);
  std::unique_ptr<StunMessage> response(iter->second->msg_->CreateNew());
  if (!response->Read(&buf)) {
    LOG(WARNING) << "Failed to read STUN response " << hex_encode(id);
    return false;
  }

  return CheckResponse(response.get());
}

void resend_cb(EventLoop* loop, TimerId timeid, void* data) {
  (void)loop;
  (void)timeid;
  if (!data) {
    return;
  }

  StunRequest* r = (StunRequest*)(data);
  r->send_stun_message();
}

StunRequest::StunRequest(EventLoop* loop)
    : count_(0),
      timeout_(false),
      manager_(0),
      msg_(new StunMessage()),
      tstamp_(0),
      loop_(loop) {

  msg_->SetTransactionID(CreateRandomString(kStunTransactionIdLength));
}

StunRequest::StunRequest(EventLoop* loop, StunMessage* request)
    : count_(0),
      timeout_(false),
      manager_(0),
      msg_(request),
      tstamp_(0),
      loop_(loop) {
  msg_->SetTransactionID(CreateRandomString(kStunTransactionIdLength));
}

StunRequest::~StunRequest() {
  assert(manager_ != NULL);
  if (manager_) {
    manager_->Remove(this);
    // manager_->thread_->Clear(this);
  }
  delete msg_;
}

void StunRequest::Construct() {
  if (msg_->type() == 0) {
    if (!origin_.empty()) {
      msg_->AddAttribute(
          std::make_unique<StunByteStringAttribute>(kStunAttrOrigin, origin_));
    }
    Prepare(msg_);
    assert(msg_->type() != 0);
  }
}

int StunRequest::type() {
  assert(msg_ != NULL);
  return msg_->type();
}

const StunMessage* StunRequest::msg() const { return msg_; }

StunMessage* StunRequest::mutable_msg() { return msg_; }

int StunRequest::Elapsed() const {
  return static_cast<int>(TimeMillis() - tstamp_);
}

void StunRequest::set_manager(StunRequestManager* manager) {
  assert(!manager_);
  manager_ = manager;
}

void StunRequest::send_stun_message() {
  assert(manager_ != NULL);

  if (timeout_) {
    OnTimeout();
    delete this;
    return;
  }

  tstamp_ = TimeMillis();

  ByteBufferWriter buf;
  msg_->Write(&buf);
  manager_->SignalSendPacket(buf.Data(), buf.Length(), this);

  OnSent();
  loop_->runAfter(resend_delay() / 1000.0,
                  std::bind(&StunRequest::send_stun_message, this));
}

void StunRequest::OnSent() {
  count_ += 1;
  int retransmissions = (count_ - 1);
  if (retransmissions >= STUN_MAX_RETRANSMISSIONS) {
    timeout_ = true;
  }
  LOG(VERBOSE) << "Sent STUN request " << count_
               << "; resend delay = " << resend_delay();
}

int StunRequest::resend_delay() {
  if (count_ == 0) {
    return 0;
  }
  int retransmissions = (count_ - 1);
  int rto = STUN_INITIAL_RTO << retransmissions;
  return std::min(rto, STUN_MAX_RTO);
}

void StunRequest::OnMessage(Message* pmsg) {}

}  // namespace cricket
