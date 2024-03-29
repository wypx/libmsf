// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: frpc.proto

#ifndef PROTOBUF_INCLUDED_frpc_2eproto
#define PROTOBUF_INCLUDED_frpc_2eproto

#include <string>

#include <google/protobuf/stubs/common.h>

#if GOOGLE_PROTOBUF_VERSION < 3006001
#error This file was generated by a newer version of protoc which is
#error incompatible with your Protocol Buffer headers.  Please update
#error your headers.
#endif
#if 3006001 < GOOGLE_PROTOBUF_MIN_PROTOC_VERSION
#error This file was generated by an older version of protoc which is
#error incompatible with your Protocol Buffer headers.  Please
#error regenerate this file with a newer version of protoc.
#endif

#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/arena.h>
#include <google/protobuf/arenastring.h>
#include <google/protobuf/generated_message_table_driven.h>
#include <google/protobuf/generated_message_util.h>
#include <google/protobuf/inlined_string_field.h>
#include <google/protobuf/metadata.h>
#include <google/protobuf/message.h>
#include <google/protobuf/repeated_field.h>  // IWYU pragma: export
#include <google/protobuf/extension_set.h>  // IWYU pragma: export
#include <google/protobuf/generated_enum_reflection.h>
#include <google/protobuf/unknown_field_set.h>
// @@protoc_insertion_point(includes)
#define PROTOBUF_INTERNAL_EXPORT_protobuf_frpc_2eproto 

namespace protobuf_frpc_2eproto {
// Internal implementation detail -- do not use these members.
struct TableStruct {
  static const ::google::protobuf::internal::ParseTableField entries[];
  static const ::google::protobuf::internal::AuxillaryParseTableField aux[];
  static const ::google::protobuf::internal::ParseTable schema[1];
  static const ::google::protobuf::internal::FieldMetadata field_metadata[];
  static const ::google::protobuf::internal::SerializationTable serialization_table[];
  static const ::google::protobuf::uint32 offsets[];
};
void AddDescriptors();
}  // namespace protobuf_frpc_2eproto
namespace frpc {
class FastMessage;
class FastMessageDefaultTypeInternal;
extern FastMessageDefaultTypeInternal _FastMessage_default_instance_;
}  // namespace frpc
namespace google {
namespace protobuf {
template<> ::frpc::FastMessage* Arena::CreateMaybeMessage<::frpc::FastMessage>(Arena*);
}  // namespace protobuf
}  // namespace google
namespace frpc {

enum MessageType {
  FRPC_MESSAGE_UNKNOWN = 0,
  FRPC_MESSAGE_REQUEST = 1,
  FRPC_MESSAGE_RESPONSE = 2,
  FRPC_MESSAGE_CANCEL = 3,
  MessageType_INT_MIN_SENTINEL_DO_NOT_USE_ = ::google::protobuf::kint32min,
  MessageType_INT_MAX_SENTINEL_DO_NOT_USE_ = ::google::protobuf::kint32max
};
bool MessageType_IsValid(int value);
const MessageType MessageType_MIN = FRPC_MESSAGE_UNKNOWN;
const MessageType MessageType_MAX = FRPC_MESSAGE_CANCEL;
const int MessageType_ARRAYSIZE = MessageType_MAX + 1;

const ::google::protobuf::EnumDescriptor* MessageType_descriptor();
inline const ::std::string& MessageType_Name(MessageType value) {
  return ::google::protobuf::internal::NameOfEnum(
    MessageType_descriptor(), value);
}
inline bool MessageType_Parse(
    const ::std::string& name, MessageType* value) {
  return ::google::protobuf::internal::ParseNamedEnum<MessageType>(
    MessageType_descriptor(), name, value);
}
enum MessageID {
  FRPC_LOGIN = 0,
  FRPC_HEARTBEAT = 1,
  MessageID_INT_MIN_SENTINEL_DO_NOT_USE_ = ::google::protobuf::kint32min,
  MessageID_INT_MAX_SENTINEL_DO_NOT_USE_ = ::google::protobuf::kint32max
};
bool MessageID_IsValid(int value);
const MessageID MessageID_MIN = FRPC_LOGIN;
const MessageID MessageID_MAX = FRPC_HEARTBEAT;
const int MessageID_ARRAYSIZE = MessageID_MAX + 1;

const ::google::protobuf::EnumDescriptor* MessageID_descriptor();
inline const ::std::string& MessageID_Name(MessageID value) {
  return ::google::protobuf::internal::NameOfEnum(
    MessageID_descriptor(), value);
}
inline bool MessageID_Parse(
    const ::std::string& name, MessageID* value) {
  return ::google::protobuf::internal::ParseNamedEnum<MessageID>(
    MessageID_descriptor(), name, value);
}
enum ReturnCode {
  FRPC_SUCCESS = 0,
  FRPC_NO_SERVICE = 1,
  FRPC_NO_METHOD = 2,
  FRPC_INVALID_REQUEST = 3,
  FRPC_INVALID_REPONSE = 4,
  RPC_ERROR_PARSE_REQUEST = 5,
  RPC_ERROR_PARSE_REPONS = 6,
  RPC_ERROR_COMPRESS_TYPE = 7,
  RPC_ERROR_NO_METHOD_NAME = 8,
  RPC_ERROR_PARSE_METHOD_NAME = 9,
  RPC_ERROR_FOUND_SERVICE = 10,
  RPC_ERROR_FOUND_METHOD = 11,
  RPC_ERROR_CHANNEL_BROKEN = 12,
  RPC_ERROR_CONNECTION_CLOSED = 13,
  RPC_ERROR_REQUEST_TIMEOUT = 14,
  RPC_ERROR_REQUEST_CANCELED = 15,
  RPC_ERROR_SERVER_UNAVAILABLE = 16,
  RPC_ERROR_SERVER_UNREACHABLE = 17,
  RPC_ERROR_SERVER_SHUTDOWN = 18,
  RPC_ERROR_SEND_BUFFER_FULL = 19,
  RPC_ERROR_SERIALIZE_REQUEST = 20,
  RPC_ERROR_SERIALIZE_RESPONSE = 21,
  RPC_ERROR_RESOLVE_ADDRESS = 22,
  RPC_ERROR_CREATE_STREAM = 23,
  RPC_ERROR_NOT_IN_RUNNING = 24,
  RPC_ERROR_SERVER_BUSY = 25,
  RPC_ERROR_TOO_MANY_OPEN_FILES = 26,
  RPC_ERROR_RESON_UNKNOWN = 27,
  ReturnCode_INT_MIN_SENTINEL_DO_NOT_USE_ = ::google::protobuf::kint32min,
  ReturnCode_INT_MAX_SENTINEL_DO_NOT_USE_ = ::google::protobuf::kint32max
};
bool ReturnCode_IsValid(int value);
const ReturnCode ReturnCode_MIN = FRPC_SUCCESS;
const ReturnCode ReturnCode_MAX = RPC_ERROR_RESON_UNKNOWN;
const int ReturnCode_ARRAYSIZE = ReturnCode_MAX + 1;

const ::google::protobuf::EnumDescriptor* ReturnCode_descriptor();
inline const ::std::string& ReturnCode_Name(ReturnCode value) {
  return ::google::protobuf::internal::NameOfEnum(
    ReturnCode_descriptor(), value);
}
inline bool ReturnCode_Parse(
    const ::std::string& name, ReturnCode* value) {
  return ::google::protobuf::internal::ParseNamedEnum<ReturnCode>(
    ReturnCode_descriptor(), name, value);
}
enum CompressType {
  CompressTypeNone = 0,
  CompressTypeGzip = 1,
  CompressTypeZlib = 2,
  CompressTypeSnappy = 3,
  CompressTypeLZ4 = 4,
  CompressTypeMax = 5,
  CompressType_INT_MIN_SENTINEL_DO_NOT_USE_ = ::google::protobuf::kint32min,
  CompressType_INT_MAX_SENTINEL_DO_NOT_USE_ = ::google::protobuf::kint32max
};
bool CompressType_IsValid(int value);
const CompressType CompressType_MIN = CompressTypeNone;
const CompressType CompressType_MAX = CompressTypeMax;
const int CompressType_ARRAYSIZE = CompressType_MAX + 1;

const ::google::protobuf::EnumDescriptor* CompressType_descriptor();
inline const ::std::string& CompressType_Name(CompressType value) {
  return ::google::protobuf::internal::NameOfEnum(
    CompressType_descriptor(), value);
}
inline bool CompressType_Parse(
    const ::std::string& name, CompressType* value) {
  return ::google::protobuf::internal::ParseNamedEnum<CompressType>(
    CompressType_descriptor(), name, value);
}
// ===================================================================

class FastMessage : public ::google::protobuf::Message /* @@protoc_insertion_point(class_definition:frpc.FastMessage) */ {
 public:
  FastMessage();
  virtual ~FastMessage();

  FastMessage(const FastMessage& from);

  inline FastMessage& operator=(const FastMessage& from) {
    CopyFrom(from);
    return *this;
  }
  #if LANG_CXX11
  FastMessage(FastMessage&& from) noexcept
    : FastMessage() {
    *this = ::std::move(from);
  }

  inline FastMessage& operator=(FastMessage&& from) noexcept {
    if (GetArenaNoVirtual() == from.GetArenaNoVirtual()) {
      if (this != &from) InternalSwap(&from);
    } else {
      CopyFrom(from);
    }
    return *this;
  }
  #endif
  static const ::google::protobuf::Descriptor* descriptor();
  static const FastMessage& default_instance();

  static void InitAsDefaultInstance();  // FOR INTERNAL USE ONLY
  static inline const FastMessage* internal_default_instance() {
    return reinterpret_cast<const FastMessage*>(
               &_FastMessage_default_instance_);
  }
  static constexpr int kIndexInFileMessages =
    0;

  void Swap(FastMessage* other);
  friend void swap(FastMessage& a, FastMessage& b) {
    a.Swap(&b);
  }

  // implements Message ----------------------------------------------

  inline FastMessage* New() const final {
    return CreateMaybeMessage<FastMessage>(NULL);
  }

  FastMessage* New(::google::protobuf::Arena* arena) const final {
    return CreateMaybeMessage<FastMessage>(arena);
  }
  void CopyFrom(const ::google::protobuf::Message& from) final;
  void MergeFrom(const ::google::protobuf::Message& from) final;
  void CopyFrom(const FastMessage& from);
  void MergeFrom(const FastMessage& from);
  void Clear() final;
  bool IsInitialized() const final;

  size_t ByteSizeLong() const final;
  bool MergePartialFromCodedStream(
      ::google::protobuf::io::CodedInputStream* input) final;
  void SerializeWithCachedSizes(
      ::google::protobuf::io::CodedOutputStream* output) const final;
  ::google::protobuf::uint8* InternalSerializeWithCachedSizesToArray(
      bool deterministic, ::google::protobuf::uint8* target) const final;
  int GetCachedSize() const final { return _cached_size_.Get(); }

  private:
  void SharedCtor();
  void SharedDtor();
  void SetCachedSize(int size) const final;
  void InternalSwap(FastMessage* other);
  private:
  inline ::google::protobuf::Arena* GetArenaNoVirtual() const {
    return NULL;
  }
  inline void* MaybeArenaPtr() const {
    return NULL;
  }
  public:

  ::google::protobuf::Metadata GetMetadata() const final;

  // nested types ----------------------------------------------------

  // accessors -------------------------------------------------------

  // fixed32 version = 1;
  void clear_version();
  static const int kVersionFieldNumber = 1;
  ::google::protobuf::uint32 version() const;
  void set_version(::google::protobuf::uint32 value);

  // fixed32 magic = 2;
  void clear_magic();
  static const int kMagicFieldNumber = 2;
  ::google::protobuf::uint32 magic() const;
  void set_magic(::google::protobuf::uint32 value);

  // fixed32 type = 3;
  void clear_type();
  static const int kTypeFieldNumber = 3;
  ::google::protobuf::uint32 type() const;
  void set_type(::google::protobuf::uint32 value);

  // fixed32 length = 4;
  void clear_length();
  static const int kLengthFieldNumber = 4;
  ::google::protobuf::uint32 length() const;
  void set_length(::google::protobuf::uint32 value);

  // fixed32 call_id = 5;
  void clear_call_id();
  static const int kCallIdFieldNumber = 5;
  ::google::protobuf::uint32 call_id() const;
  void set_call_id(::google::protobuf::uint32 value);

  // fixed32 opcode = 8;
  void clear_opcode();
  static const int kOpcodeFieldNumber = 8;
  ::google::protobuf::uint32 opcode() const;
  void set_opcode(::google::protobuf::uint32 value);

  // fixed32 compress = 9;
  void clear_compress();
  static const int kCompressFieldNumber = 9;
  ::google::protobuf::uint32 compress() const;
  void set_compress(::google::protobuf::uint32 value);

  // fixed32 retcode = 10;
  void clear_retcode();
  static const int kRetcodeFieldNumber = 10;
  ::google::protobuf::uint32 retcode() const;
  void set_retcode(::google::protobuf::uint32 value);

  // @@protoc_insertion_point(class_scope:frpc.FastMessage)
 private:

  ::google::protobuf::internal::InternalMetadataWithArena _internal_metadata_;
  ::google::protobuf::uint32 version_;
  ::google::protobuf::uint32 magic_;
  ::google::protobuf::uint32 type_;
  ::google::protobuf::uint32 length_;
  ::google::protobuf::uint32 call_id_;
  ::google::protobuf::uint32 opcode_;
  ::google::protobuf::uint32 compress_;
  ::google::protobuf::uint32 retcode_;
  mutable ::google::protobuf::internal::CachedSize _cached_size_;
  friend struct ::protobuf_frpc_2eproto::TableStruct;
};
// ===================================================================


// ===================================================================

#ifdef __GNUC__
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif  // __GNUC__
// FastMessage

// fixed32 version = 1;
inline void FastMessage::clear_version() {
  version_ = 0u;
}
inline ::google::protobuf::uint32 FastMessage::version() const {
  // @@protoc_insertion_point(field_get:frpc.FastMessage.version)
  return version_;
}
inline void FastMessage::set_version(::google::protobuf::uint32 value) {
  
  version_ = value;
  // @@protoc_insertion_point(field_set:frpc.FastMessage.version)
}

// fixed32 magic = 2;
inline void FastMessage::clear_magic() {
  magic_ = 0u;
}
inline ::google::protobuf::uint32 FastMessage::magic() const {
  // @@protoc_insertion_point(field_get:frpc.FastMessage.magic)
  return magic_;
}
inline void FastMessage::set_magic(::google::protobuf::uint32 value) {
  
  magic_ = value;
  // @@protoc_insertion_point(field_set:frpc.FastMessage.magic)
}

// fixed32 type = 3;
inline void FastMessage::clear_type() {
  type_ = 0u;
}
inline ::google::protobuf::uint32 FastMessage::type() const {
  // @@protoc_insertion_point(field_get:frpc.FastMessage.type)
  return type_;
}
inline void FastMessage::set_type(::google::protobuf::uint32 value) {
  
  type_ = value;
  // @@protoc_insertion_point(field_set:frpc.FastMessage.type)
}

// fixed32 length = 4;
inline void FastMessage::clear_length() {
  length_ = 0u;
}
inline ::google::protobuf::uint32 FastMessage::length() const {
  // @@protoc_insertion_point(field_get:frpc.FastMessage.length)
  return length_;
}
inline void FastMessage::set_length(::google::protobuf::uint32 value) {
  
  length_ = value;
  // @@protoc_insertion_point(field_set:frpc.FastMessage.length)
}

// fixed32 call_id = 5;
inline void FastMessage::clear_call_id() {
  call_id_ = 0u;
}
inline ::google::protobuf::uint32 FastMessage::call_id() const {
  // @@protoc_insertion_point(field_get:frpc.FastMessage.call_id)
  return call_id_;
}
inline void FastMessage::set_call_id(::google::protobuf::uint32 value) {
  
  call_id_ = value;
  // @@protoc_insertion_point(field_set:frpc.FastMessage.call_id)
}

// fixed32 opcode = 8;
inline void FastMessage::clear_opcode() {
  opcode_ = 0u;
}
inline ::google::protobuf::uint32 FastMessage::opcode() const {
  // @@protoc_insertion_point(field_get:frpc.FastMessage.opcode)
  return opcode_;
}
inline void FastMessage::set_opcode(::google::protobuf::uint32 value) {
  
  opcode_ = value;
  // @@protoc_insertion_point(field_set:frpc.FastMessage.opcode)
}

// fixed32 compress = 9;
inline void FastMessage::clear_compress() {
  compress_ = 0u;
}
inline ::google::protobuf::uint32 FastMessage::compress() const {
  // @@protoc_insertion_point(field_get:frpc.FastMessage.compress)
  return compress_;
}
inline void FastMessage::set_compress(::google::protobuf::uint32 value) {
  
  compress_ = value;
  // @@protoc_insertion_point(field_set:frpc.FastMessage.compress)
}

// fixed32 retcode = 10;
inline void FastMessage::clear_retcode() {
  retcode_ = 0u;
}
inline ::google::protobuf::uint32 FastMessage::retcode() const {
  // @@protoc_insertion_point(field_get:frpc.FastMessage.retcode)
  return retcode_;
}
inline void FastMessage::set_retcode(::google::protobuf::uint32 value) {
  
  retcode_ = value;
  // @@protoc_insertion_point(field_set:frpc.FastMessage.retcode)
}

#ifdef __GNUC__
  #pragma GCC diagnostic pop
#endif  // __GNUC__

// @@protoc_insertion_point(namespace_scope)

}  // namespace frpc

namespace google {
namespace protobuf {

template <> struct is_proto_enum< ::frpc::MessageType> : ::std::true_type {};
template <>
inline const EnumDescriptor* GetEnumDescriptor< ::frpc::MessageType>() {
  return ::frpc::MessageType_descriptor();
}
template <> struct is_proto_enum< ::frpc::MessageID> : ::std::true_type {};
template <>
inline const EnumDescriptor* GetEnumDescriptor< ::frpc::MessageID>() {
  return ::frpc::MessageID_descriptor();
}
template <> struct is_proto_enum< ::frpc::ReturnCode> : ::std::true_type {};
template <>
inline const EnumDescriptor* GetEnumDescriptor< ::frpc::ReturnCode>() {
  return ::frpc::ReturnCode_descriptor();
}
template <> struct is_proto_enum< ::frpc::CompressType> : ::std::true_type {};
template <>
inline const EnumDescriptor* GetEnumDescriptor< ::frpc::CompressType>() {
  return ::frpc::CompressType_descriptor();
}

}  // namespace protobuf
}  // namespace google

// @@protoc_insertion_point(global_scope)

#endif  // PROTOBUF_INCLUDED_frpc_2eproto
