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
#ifndef BASE_BLOOM_H_
#define BASE_BLOOM_H_

#include "slice.h"
#include <string>

using namespace MSF;

size_t bloom_size(int n);
void bloom_create(const Slice* keys, int n, std::string* bitsets);
bool bloom_matches(const Slice& k, const Slice& filter);

#endif