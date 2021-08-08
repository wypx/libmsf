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
#ifndef FRAFT_STRING_UTILITY_H_
#define FRAFT_STRING_UTILITY_H_

#include <string>

namespace MSF {

class StringUtility {
 public:
  static std::string ConvertBoolToString(bool value);
  static std::string ConvertUint64ToString(uint64_t value);
  static std::string ConvertInt64ToString(int64_t value);
  static std::string ConvertUint32ToString(uint32_t value);
  static std::string ConvertInt32ToString(int32_t value);

  static bool ConvertStringToBool(const std::string &input);
  static uint64_t ConvertStringToUint64(const std::string &input);
  static int64_t ConvertStringToInt64(const std::string &input);
  static uint32_t ConvertStringToUint32(const std::string &input);
  static int32_t ConvertStringToInt32(const std::string &input);

  static std::string SerializeBoolToString(bool value);
  static std::string SerializeUint64ToString(uint64_t value);
  static std::string SerializeInt64ToString(int64_t value);
  static std::string SerializeUint32ToString(uint32_t value);
  static std::string SerializeInt32ToString(int32_t value);

  static bool DeserializeStringToBool(const std::string &input);
  static uint64_t DeserializeStringToUint64(const std::string &input);
  static int64_t DeserializeStringToInt64(const std::string &input);
  static uint32_t DeserializeStringToUint32(const std::string &input);
  static int32_t DeserializeStringToInt32(const std::string &input);
};
};
#endif  // FRAFT_STRING_UTILITY_H_
