/*
 *  Copyright 2018 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "ice_credentials_iterator.h"

#include "p2p_constants.h"
#include "helpers.h"

namespace MSF {

IceCredentialsIterator::IceCredentialsIterator(
    const std::vector<IceParameters>& pooled_credentials)
    : pooled_ice_credentials_(pooled_credentials) {}

IceCredentialsIterator::~IceCredentialsIterator() = default;

IceParameters IceCredentialsIterator::CreateRandomIceCredentials() {
  return IceParameters(CreateRandomString(ICE_UFRAG_LENGTH),
                       CreateRandomString(ICE_PWD_LENGTH), false);
}

IceParameters IceCredentialsIterator::GetIceCredentials() {
  if (pooled_ice_credentials_.empty()) {
    return CreateRandomIceCredentials();
  }
  IceParameters credentials = pooled_ice_credentials_.back();
  pooled_ice_credentials_.pop_back();
  return credentials;
}

}  // namespace cricket
