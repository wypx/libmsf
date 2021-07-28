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
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sstream>
#include <iostream>
#include <fraft/string_utility.h>

namespace {
static const uint32_t kBufferLength = 20;
};
namespace MSF {

template <class Type>
static std::string ConvertValueToString(Type value) {
  std::stringstream stream;
  stream << value;
  return stream.str();
}

template <class Type>
static Type ConvertStringToValue(const std::string &input) {
  std::stringstream stream(input);
  Type result;
  stream >> result;
  return result;
}

template <class Type>
static std::string SerializeValueToString(const Type &value) {
  char buffer[kBufferLength] = {'\0'};
  memcpy(buffer, &value, sizeof(Type));
  return std::string(buffer, sizeof(Type));
}

template <class Type>
Type DeserializeStringToValue(const std::string &input) {
  Type result;
  memcpy((char *)(&result), input.c_str(), sizeof(Type));
  return result;
}

std::string StringUtility::ConvertBoolToString(bool value) {
  return ConvertValueToString(value);
}

std::string StringUtility::ConvertUint64ToString(uint64_t value) {
  return ConvertValueToString(value);
}

std::string StringUtility::ConvertInt64ToString(int64_t value) {
  return ConvertValueToString(value);
}

std::string StringUtility::ConvertUint32ToString(uint32_t value) {
  return ConvertValueToString(value);
}

std::string StringUtility::ConvertInt32ToString(int32_t value) {
  return ConvertValueToString(value);
}

bool StringUtility::ConvertStringToBool(const std::string &input) {
  return ConvertStringToValue<bool>(input);
}

uint64_t StringUtility::ConvertStringToUint64(const std::string &input) {
  return ConvertStringToValue<uint64_t>(input);
}

int64_t StringUtility::ConvertStringToInt64(const std::string &input) {
  return ConvertStringToValue<int64_t>(input);
}

uint32_t StringUtility::ConvertStringToUint32(const std::string &input) {
  return ConvertStringToValue<uint32_t>(input);
}

int32_t StringUtility::ConvertStringToInt32(const std::string &input) {
  return ConvertStringToValue<int32_t>(input);
}

std::string StringUtility::SerializeBoolToString(bool value) {
  return SerializeValueToString(value);
}

std::string StringUtility::SerializeUint64ToString(uint64_t value) {
  return SerializeValueToString(value);
}

std::string StringUtility::SerializeInt64ToString(int64_t value) {
  return SerializeValueToString(value);
}

std::string StringUtility::SerializeUint32ToString(uint32_t value) {
  return SerializeValueToString(value);
}

std::string StringUtility::SerializeInt32ToString(int32_t value) {
  return SerializeValueToString(value);
}

bool StringUtility::DeserializeStringToBool(const std::string &input) {
  return DeserializeStringToValue<bool>(input);
}

uint64_t StringUtility::DeserializeStringToUint64(const std::string &input) {
  return DeserializeStringToValue<uint64_t>(input);
}

int64_t StringUtility::DeserializeStringToInt64(const std::string &input) {
  return DeserializeStringToValue<int64_t>(input);
}

uint32_t StringUtility::DeserializeStringToUint32(const std::string &input) {
  return DeserializeStringToValue<uint32_t>(input);
}

int32_t StringUtility::DeserializeStringToInt32(const std::string &input) {
  return DeserializeStringToValue<int32_t>(input);
}
};
