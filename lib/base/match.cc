// Copyright 2017 The Abseil Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "match.h"

#include <cctype>

namespace MSF {

int memcasecmp(const char* s1, const char* s2, size_t len) {
  const unsigned char* us1 = reinterpret_cast<const unsigned char*>(s1);
  const unsigned char* us2 = reinterpret_cast<const unsigned char*>(s2);

  for (size_t i = 0; i < len; i++) {
    const int diff = int{static_cast<unsigned char>(std::tolower(us1[i]))} -
                     int{static_cast<unsigned char>(std::tolower(us2[i]))};
    if (diff != 0) return diff;
  }
  return 0;
}

bool EqualsIgnoreCase(std::string_view piece1, std::string_view piece2) {
  return (piece1.size() == piece2.size() &&
          0 == memcasecmp(piece1.data(), piece2.data(), piece1.size()));
}

bool StartsWithIgnoreCase(std::string_view text, std::string_view prefix) {
  return (text.size() >= prefix.size()) &&
         EqualsIgnoreCase(text.substr(0, prefix.size()), prefix);
}

bool EndsWithIgnoreCase(std::string_view text, std::string_view suffix) {
  return (text.size() >= suffix.size()) &&
         EqualsIgnoreCase(text.substr(text.size() - suffix.size()), suffix);
}

}  // namespace MSF
