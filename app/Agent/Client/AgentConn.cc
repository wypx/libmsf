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
#include <butil/logging.h>
#include "AgentConn.h"

namespace MSF {

AgentConn::AgentConn(/* args */)
    : pduCount_(0), mutex_(), txQequeSize_(0), txNeedSend_(0) {}

AgentConn::~AgentConn() {}

bool AgentConn::doRecvBhs() {
  LOG(INFO) << "Recv msg from fd: " << fd_;
  uint32_t count = 0;
  void *head = mpool_->Alloc(AGENT_HEAD_LEN);
  assert(head);
  struct iovec iov = {head, AGENT_HEAD_LEN};
  int ret = RecvMsg(fd_, &iov, 1, MSG_NOSIGNAL | MSG_DONTWAIT);
  LOG(INFO) << "Recv msg ret: " << ret;
  if (unlikely(ret <= 0)) {
    /* Maybe there is no data anymore */
    if (ret < 0 &&
        (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)) {
      if ((++count) > 4) {
        return true;
      }
      // continue;
    }
    LOG(ERROR) << "Recv buffer failed for fd: " << fd_;
    return false;
  }
  assert(ret == AGENT_HEAD_LEN);
  rxQequeIov_.push_back(std::move(iov));

  Agent::AgentBhs bhs;
  bhs.ParseFromArray(head, AGENT_HEAD_LEN);
  proto_->debugBhs(bhs);

  uint32_t len = proto_->pduLen(bhs);
  if (len) {
    void *body = mpool_->Alloc(len);
    assert(body);
    rxState_ = kRecvPdu;
    iov.iov_base = body;
    iov.iov_len = len;
    ret = RecvMsg(fd_, &iov, 1, MSG_NOSIGNAL | MSG_DONTWAIT);
    if (unlikely(ret <= 0)) {
      /* Maybe there is no data anymore */
      if (ret < 0 &&
          (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)) {
        if ((++count) > 4) {
          return true;
        }
        // continue;
      }
      LOG(ERROR) << "Recv buffer failed for fd: " << fd_;
      return false;
    }
    // assert(ret == len);
    rxQequeIov_.push_back(std::move(iov));
    pduCount_++;
  }
  rxState_ = kRecvBhs;
  pduCount_++;
  return true;
}

bool AgentConn::doRecvPdu() { return true; }

bool AgentConn::doConnRead() {
  if (unlikely(fd_ < 0)) {
    LOG(TRACE) << "Conn has been closed, cannot read buffer";
    return false;
  }
  return doRecvBhs();
  // int ret;
  // int count = 0;
  // do {
  //   switch (rxState_)
  //   {
  //     case kRecvBhs:
  //       // doRecvBhs();
  //       return;
  //     case kRecvPdu:
  //       doRecvPdu();
  //       break;
  //     default:
  //       break;
  //   }
  // } while (true);
  // readCb_();
}

bool AgentConn::writeIovec(struct iovec iov) {
  if (fd_ < 0) {
    LOG(TRACE) << "Conn has been closed, cannot send buffer";
    return false;
  }
  std::lock_guard<std::mutex> lock(mutex_);
  txQequeIov_.push_back(std::move(iov));
  txQequeSize_ += iov.iov_len;
  return true;
}

bool AgentConn::writeBuffer(char *data, const uint32_t len) {
  if (fd_ < 0) {
    LOG(WARNING) << "Conn has been closed, cannot send buffer";
    return false;
  }
  std::lock_guard<std::mutex> lock(mutex_);
  struct iovec iov = {data, len};
  txQequeIov_.push_back(std::move(iov));
  txQequeSize_ += len;
  return true;
}

bool AgentConn::writeBuffer(void *data, const uint32_t len) {
  return writeBuffer(static_cast<char *>(data), len);
}

// https://www.icode9.com/content-4-484721.html
//
void AgentConn::updateBusyIov() {
  std::lock_guard<std::mutex> lock(mutex_);
  if (txQequeIov_.size() > 0) {
    LOG(INFO) << "txQequeIov_ size1: " << txQequeIov_.size();
    LOG(INFO) << "txBusyIov_ size1: " << txBusyIov_.size();
    txBusyIov_.insert(txBusyIov_.end(),
                      std::make_move_iterator(txQequeIov_.begin()),
                      std::make_move_iterator(txQequeIov_.end()));
    // std::move(txQequeIov_.begin(), txQequeIov_.end(),
    // back_inserter(txBusyIov_));
    txQequeIov_.clear();
    LOG(INFO) << "txQequeIov_ size1: " << txQequeIov_.size();
    LOG(INFO) << "txBusyIov_ size1: " << txBusyIov_.size();
    txNeedSend_ += txQequeSize_;
    txQequeSize_ = 0;
  }
}

bool AgentConn::updateTxOffset(const int ret) {
  txTotalSend_ += ret;
  txNeedSend_ -= ret;
  // https://blog.csdn.net/strengthennn/article/details/97645912
  // https://blog.csdn.net/yang3wei/article/details/7589344
  // https://www.cnblogs.com/dabaopku/p/3912662.html
  /* Half send */
  uint32_t txIovOffset = ret;
  LOG(INFO) << "txBusyIov_ size: " << txBusyIov_.size();
  auto iter = txBusyIov_.begin();
  while (iter != txBusyIov_.end()) {
    if (txIovOffset > iter->iov_len) {
      LOG(INFO) << "iter->iov_len1: " << iter->iov_len
                << " txIovOffset:" << txIovOffset;
      txIovOffset -= iter->iov_len;
      mpool_->Free(iter->iov_base);
      iter = txBusyIov_.erase(iter);  // return next item
    } else if (txIovOffset == iter->iov_len) {
      LOG(INFO) << "iter->iov_len2: " << iter->iov_len
                << " txIovOffset:" << txIovOffset;
      mpool_->Free(iter->iov_base);
      iter = txBusyIov_.erase(iter);  // return next item
      break;
    } else {
      LOG(INFO) << "iter->iov_len3: " << iter->iov_len
                << " txIovOffset:" << txIovOffset;
      iter->iov_base = static_cast<char *>(iter->iov_base) + txIovOffset;
      iter->iov_len -= txIovOffset;
      // ++iter;
      break;
    }
  }
  LOG(INFO) << "txBusyIov_ size: " << txBusyIov_.size();
  if (unlikely(txNeedSend_ > 0)) {
    return false;
  }
  return true;
}

bool AgentConn::doConnWrite() {
  if (unlikely(fd_ < 0)) {
    return false;
  }
  updateBusyIov();

  int ret;
  int count = 0;
  do {
    ret = SendMsg(fd_, &*txBusyIov_.begin(), txBusyIov_.size(),
                  MSG_NOSIGNAL | MSG_DONTWAIT);
    LOG(INFO) << "Send ret: " << ret << " needed: " << txNeedSend_;
    LOG(INFO) << "Send iovcnt: " << txBusyIov_.size();
    if (unlikely(ret <= 0)) {
      /* Maybe there is no data anymore */
      if (ret < 0 &&
          (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)) {
        if ((++count) > 4) {
          return true;
        }
        continue;
      }
      LOG(ERROR) << "Send buffer failed for fd: " << fd_;
      return false;
    } else {
      if (unlikely(!updateTxOffset(ret))) {
        continue;
      } else {
        DisableWriting();
        return true;
      }
    }
  } while (true);
}

void AgentConn::doConnClose() {
  Connector::CloseConn();
  LOG(INFO) << "txBusyIov_ iovcnt: " << txBusyIov_.size();
  auto iter = txBusyIov_.begin();
  while (iter != txBusyIov_.end()) {
    mpool_->Free(iter->iov_base);
    iter = txBusyIov_.erase(iter);
  }
  LOG(INFO) << "txQequeIov_ iovcnt: " << txQequeIov_.size();
  iter = txQequeIov_.begin();
  while (iter != txQequeIov_.end()) {
    mpool_->Free(iter->iov_base);
    iter = txQequeIov_.erase(iter);
  }
  LOG(INFO) << "rxQequeIov_ iovcnt: " << rxQequeIov_.size();
  auto it = rxQequeIov_.begin();
  while (it != rxQequeIov_.end()) {
    mpool_->Free(it->iov_base);
    it = rxQequeIov_.erase(it);
  }
}

}  // namespace MSF
