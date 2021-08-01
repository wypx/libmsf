// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: frpc.proto

#ifndef GOOGLE_PROTOBUF_INCLUDED_frpc_2eproto
#define GOOGLE_PROTOBUF_INCLUDED_frpc_2eproto

#include <limits>
#include <string>

#include <google/protobuf/port_def.inc>
#if PROTOBUF_VERSION < 3017000
#error This file was generated by a newer version of protoc which is
#error incompatible with your Protocol Buffer headers. Please update
#error your headers.
#endif
#if 3017001 < PROTOBUF_MIN_PROTOC_VERSION
#error This file was generated by an older version of protoc which is
#error incompatible with your Protocol Buffer headers. Please
#error regenerate this file with a newer version of protoc.
#endif

#include <google/protobuf/port_undef.inc>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/arena.h>
#include <google/protobuf/arenastring.h>
#include <google/protobuf/generated_message_table_driven.h>
#include <google/protobuf/generated_message_util.h>
#include <google/protobuf/metadata_lite.h>
#include <google/protobuf/generated_message_reflection.h>
#include <google/protobuf/message.h>
#include <google/protobuf/repeated_field.h>  // IWYU pragma: export
#include <google/protobuf/extension_set.h>   // IWYU pragma: export
#include <google/protobuf/generated_enum_reflection.h>
#include <google/protobuf/unknown_field_set.h>
// @@protoc_insertion_point(includes)
#include <google/protobuf/port_def.inc>
#define PROTOBUF_INTERNAL_EXPORT_frpc_2eproto
PROTOBUF_NAMESPACE_OPEN
namespace internal {
class AnyMetadata;
}  // namespace internal
PROTOBUF_NAMESPACE_CLOSE

// Internal implementation detail -- do not use these members.
struct TableStruct_frpc_2eproto {
  static const ::PROTOBUF_NAMESPACE_ID::internal::ParseTableField entries
      [] PROTOBUF_SECTION_VARIABLE(protodesc_cold);
  static const ::PROTOBUF_NAMESPACE_ID::internal::AuxiliaryParseTableField aux
      [] PROTOBUF_SECTION_VARIABLE(protodesc_cold);
  static const ::PROTOBUF_NAMESPACE_ID::internal::ParseTable schema
      [1] PROTOBUF_SECTION_VARIABLE(protodesc_cold);
  static const ::PROTOBUF_NAMESPACE_ID::internal::FieldMetadata field_metadata
      [];
  static const ::PROTOBUF_NAMESPACE_ID::internal::SerializationTable
      serialization_table[];
  static const ::PROTOBUF_NAMESPACE_ID::uint32 offsets[];
};
extern const ::PROTOBUF_NAMESPACE_ID::internal::DescriptorTable
    descriptor_table_frpc_2eproto;
namespace frpc {
class FastMessage;
struct FastMessageDefaultTypeInternal;
extern FastMessageDefaultTypeInternal _FastMessage_default_instance_;
}  // namespace frpc
PROTOBUF_NAMESPACE_OPEN
template <>
::frpc::FastMessage* Arena::CreateMaybeMessage<::frpc::FastMessage>(Arena*);
PROTOBUF_NAMESPACE_CLOSE
namespace frpc {

enum MessageType : int {
  FRPC_MESSAGE_UNKNOWN = 0,
  FRPC_MESSAGE_REQUEST = 1,
  FRPC_MESSAGE_RESPONSE = 2,
  FRPC_MESSAGE_CANCEL = 3,
  MessageType_INT_MIN_SENTINEL_DO_NOT_USE_ =
      std::numeric_limits<::PROTOBUF_NAMESPACE_ID::int32>::min(),
  MessageType_INT_MAX_SENTINEL_DO_NOT_USE_ =
      std::numeric_limits<::PROTOBUF_NAMESPACE_ID::int32>::max()
};
bool MessageType_IsValid(int value);
constexpr MessageType MessageType_MIN = FRPC_MESSAGE_UNKNOWN;
constexpr MessageType MessageType_MAX = FRPC_MESSAGE_CANCEL;
constexpr int MessageType_ARRAYSIZE = MessageType_MAX + 1;

const ::PROTOBUF_NAMESPACE_ID::EnumDescriptor* MessageType_descriptor();
template <typename T>
inline const std::string& MessageType_Name(T enum_t_value) {
  static_assert(
      ::std::is_same<T, MessageType>::value || ::std::is_integral<T>::value,
      "Incorrect type passed to function MessageType_Name.");
  return ::PROTOBUF_NAMESPACE_ID::internal::NameOfEnum(MessageType_descriptor(),
                                                       enum_t_value);
}
inline bool MessageType_Parse(::PROTOBUF_NAMESPACE_ID::ConstStringParam name,
                              MessageType* value) {
  return ::PROTOBUF_NAMESPACE_ID::internal::ParseNamedEnum<MessageType>(
      MessageType_descriptor(), name, value);
}
enum MessageID : int {
  FRPC_LOGIN = 0,
  FRPC_HEARTBEAT = 1,
  MessageID_INT_MIN_SENTINEL_DO_NOT_USE_ =
      std::numeric_limits<::PROTOBUF_NAMESPACE_ID::int32>::min(),
  MessageID_INT_MAX_SENTINEL_DO_NOT_USE_ =
      std::numeric_limits<::PROTOBUF_NAMESPACE_ID::int32>::max()
};
bool MessageID_IsValid(int value);
constexpr MessageID MessageID_MIN = FRPC_LOGIN;
constexpr MessageID MessageID_MAX = FRPC_HEARTBEAT;
constexpr int MessageID_ARRAYSIZE = MessageID_MAX + 1;

const ::PROTOBUF_NAMESPACE_ID::EnumDescriptor* MessageID_descriptor();
template <typename T>
inline const std::string& MessageID_Name(T enum_t_value) {
  static_assert(
      ::std::is_same<T, MessageID>::value || ::std::is_integral<T>::value,
      "Incorrect type passed to function MessageID_Name.");
  return ::PROTOBUF_NAMESPACE_ID::internal::NameOfEnum(MessageID_descriptor(),
                                                       enum_t_value);
}
inline bool MessageID_Parse(::PROTOBUF_NAMESPACE_ID::ConstStringParam name,
                            MessageID* value) {
  return ::PROTOBUF_NAMESPACE_ID::internal::ParseNamedEnum<MessageID>(
      MessageID_descriptor(), name, value);
}
enum ReturnCode : int {
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
  ReturnCode_INT_MIN_SENTINEL_DO_NOT_USE_ =
      std::numeric_limits<::PROTOBUF_NAMESPACE_ID::int32>::min(),
  ReturnCode_INT_MAX_SENTINEL_DO_NOT_USE_ =
      std::numeric_limits<::PROTOBUF_NAMESPACE_ID::int32>::max()
};
bool ReturnCode_IsValid(int value);
constexpr ReturnCode ReturnCode_MIN = FRPC_SUCCESS;
constexpr ReturnCode ReturnCode_MAX = RPC_ERROR_RESON_UNKNOWN;
constexpr int ReturnCode_ARRAYSIZE = ReturnCode_MAX + 1;

const ::PROTOBUF_NAMESPACE_ID::EnumDescriptor* ReturnCode_descriptor();
template <typename T>
inline const std::string& ReturnCode_Name(T enum_t_value) {
  static_assert(
      ::std::is_same<T, ReturnCode>::value || ::std::is_integral<T>::value,
      "Incorrect type passed to function ReturnCode_Name.");
  return ::PROTOBUF_NAMESPACE_ID::internal::NameOfEnum(ReturnCode_descriptor(),
                                                       enum_t_value);
}
inline bool ReturnCode_Parse(::PROTOBUF_NAMESPACE_ID::ConstStringParam name,
                             ReturnCode* value) {
  return ::PROTOBUF_NAMESPACE_ID::internal::ParseNamedEnum<ReturnCode>(
      ReturnCode_descriptor(), name, value);
}
enum CompressType : int {
  CompressTypeNone = 0,
  CompressTypeGzip = 1,
  CompressTypeZlib = 2,
  CompressTypeSnappy = 3,
  CompressTypeLZ4 = 4,
  CompressTypeMax = 5,
  CompressType_INT_MIN_SENTINEL_DO_NOT_USE_ =
      std::numeric_limits<::PROTOBUF_NAMESPACE_ID::int32>::min(),
  CompressType_INT_MAX_SENTINEL_DO_NOT_USE_ =
      std::numeric_limits<::PROTOBUF_NAMESPACE_ID::int32>::max()
};
bool CompressType_IsValid(int value);
constexpr CompressType CompressType_MIN = CompressTypeNone;
constexpr CompressType CompressType_MAX = CompressTypeMax;
constexpr int CompressType_ARRAYSIZE = CompressType_MAX + 1;

const ::PROTOBUF_NAMESPACE_ID::EnumDescriptor* CompressType_descriptor();
template <typename T>
inline const std::string& CompressType_Name(T enum_t_value) {
  static_assert(
      ::std::is_same<T, CompressType>::value || ::std::is_integral<T>::value,
      "Incorrect type passed to function CompressType_Name.");
  return ::PROTOBUF_NAMESPACE_ID::internal::NameOfEnum(
      CompressType_descriptor(), enum_t_value);
}
inline bool CompressType_Parse(::PROTOBUF_NAMESPACE_ID::ConstStringParam name,
                               CompressType* value) {
  return ::PROTOBUF_NAMESPACE_ID::internal::ParseNamedEnum<CompressType>(
      CompressType_descriptor(), name, value);
}
// ===================================================================

class FastMessage final
    : public ::PROTOBUF_NAMESPACE_ID::
          Message /* @@protoc_insertion_point(class_definition:frpc.FastMessage) */ {
 public:
  inline FastMessage() : FastMessage(nullptr) {}
  ~FastMessage() override;
  explicit constexpr FastMessage(
      ::PROTOBUF_NAMESPACE_ID::internal::ConstantInitialized);

  FastMessage(const FastMessage& from);
  FastMessage(FastMessage&& from) noexcept : FastMessage() {
    *this = ::std::move(from);
  }

  inline FastMessage& operator=(const FastMessage& from) {
    CopyFrom(from);
    return *this;
  }
  inline FastMessage& operator=(FastMessage&& from) noexcept {
    if (this == &from) return *this;
    if (GetOwningArena() == from.GetOwningArena()) {
      InternalSwap(&from);
    } else {
      CopyFrom(from);
    }
    return *this;
  }

  static const ::PROTOBUF_NAMESPACE_ID::Descriptor* descriptor() {
    return GetDescriptor();
  }
  static const ::PROTOBUF_NAMESPACE_ID::Descriptor* GetDescriptor() {
    return default_instance().GetMetadata().descriptor;
  }
  static const ::PROTOBUF_NAMESPACE_ID::Reflection* GetReflection() {
    return default_instance().GetMetadata().reflection;
  }
  static const FastMessage& default_instance() {
    return *internal_default_instance();
  }
  static inline const FastMessage* internal_default_instance() {
    return reinterpret_cast<const FastMessage*>(
        &_FastMessage_default_instance_);
  }
  static constexpr int kIndexInFileMessages = 0;

  friend void swap(FastMessage& a, FastMessage& b) { a.Swap(&b); }
  inline void Swap(FastMessage* other) {
    if (other == this) return;
    if (GetOwningArena() == other->GetOwningArena()) {
      InternalSwap(other);
    } else {
      ::PROTOBUF_NAMESPACE_ID::internal::GenericSwap(this, other);
    }
  }
  void UnsafeArenaSwap(FastMessage* other) {
    if (other == this) return;
    GOOGLE_DCHECK(GetOwningArena() == other->GetOwningArena());
    InternalSwap(other);
  }

  // implements Message ----------------------------------------------

  inline FastMessage* New() const final { return new FastMessage(); }

  FastMessage* New(::PROTOBUF_NAMESPACE_ID::Arena* arena) const final {
    return CreateMaybeMessage<FastMessage>(arena);
  }
  using ::PROTOBUF_NAMESPACE_ID::Message::CopyFrom;
  void CopyFrom(const FastMessage& from);
  using ::PROTOBUF_NAMESPACE_ID::Message::MergeFrom;
  void MergeFrom(const FastMessage& from);

 private:
  static void MergeImpl(::PROTOBUF_NAMESPACE_ID::Message* to,
                        const ::PROTOBUF_NAMESPACE_ID::Message& from);

 public:
  PROTOBUF_ATTRIBUTE_REINITIALIZES void Clear() final;
  bool IsInitialized() const final;

  size_t ByteSizeLong() const final;
  const char* _InternalParse(
      const char* ptr,
      ::PROTOBUF_NAMESPACE_ID::internal::ParseContext* ctx) final;
  ::PROTOBUF_NAMESPACE_ID::uint8* _InternalSerialize(
      ::PROTOBUF_NAMESPACE_ID::uint8* target,
      ::PROTOBUF_NAMESPACE_ID::io::EpsCopyOutputStream* stream) const final;
  int GetCachedSize() const final { return _cached_size_.Get(); }

 private:
  void SharedCtor();
  void SharedDtor();
  void SetCachedSize(int size) const final;
  void InternalSwap(FastMessage* other);
  friend class ::PROTOBUF_NAMESPACE_ID::internal::AnyMetadata;
  static ::PROTOBUF_NAMESPACE_ID::StringPiece FullMessageName() {
    return "frpc.FastMessage";
  }

 protected:
  explicit FastMessage(::PROTOBUF_NAMESPACE_ID::Arena* arena,
                       bool is_message_owned = false);

 private:
  static void ArenaDtor(void* object);
  inline void RegisterArenaDtor(::PROTOBUF_NAMESPACE_ID::Arena* arena);

 public:
  static const ClassData _class_data_;
  const ::PROTOBUF_NAMESPACE_ID::Message::ClassData* GetClassData() const final;

  ::PROTOBUF_NAMESPACE_ID::Metadata GetMetadata() const final;

  // nested types ----------------------------------------------------

  // accessors -------------------------------------------------------

  enum : int {
    kCallIdFieldNumber = 5,
    kMethodFieldNumber = 6,
    kServiceFieldNumber = 7,
    kMessageFieldNumber = 11,
    kVersionFieldNumber = 1,
    kMagicFieldNumber = 2,
    kTypeFieldNumber = 3,
    kLengthFieldNumber = 4,
    kRequestCompressTypeFieldNumber = 8,
    kResponseCompressTypeFieldNumber = 9,
    kRetcodeFieldNumber = 10,
  };
  // string call_id = 5;
  void clear_call_id();
  const std::string& call_id() const;
  template <typename ArgT0 = const std::string&, typename... ArgT>
  void set_call_id(ArgT0&& arg0, ArgT... args);
  std::string* mutable_call_id();
  PROTOBUF_MUST_USE_RESULT std::string* release_call_id();
  void set_allocated_call_id(std::string* call_id);

 private:
  const std::string& _internal_call_id() const;
  inline PROTOBUF_ALWAYS_INLINE void _internal_set_call_id(
      const std::string& value);
  std::string* _internal_mutable_call_id();

 public:
  // string method = 6;
  void clear_method();
  const std::string& method() const;
  template <typename ArgT0 = const std::string&, typename... ArgT>
  void set_method(ArgT0&& arg0, ArgT... args);
  std::string* mutable_method();
  PROTOBUF_MUST_USE_RESULT std::string* release_method();
  void set_allocated_method(std::string* method);

 private:
  const std::string& _internal_method() const;
  inline PROTOBUF_ALWAYS_INLINE void _internal_set_method(
      const std::string& value);
  std::string* _internal_mutable_method();

 public:
  // string service = 7;
  void clear_service();
  const std::string& service() const;
  template <typename ArgT0 = const std::string&, typename... ArgT>
  void set_service(ArgT0&& arg0, ArgT... args);
  std::string* mutable_service();
  PROTOBUF_MUST_USE_RESULT std::string* release_service();
  void set_allocated_service(std::string* service);

 private:
  const std::string& _internal_service() const;
  inline PROTOBUF_ALWAYS_INLINE void _internal_set_service(
      const std::string& value);
  std::string* _internal_mutable_service();

 public:
  // string message = 11;
  void clear_message();
  const std::string& message() const;
  template <typename ArgT0 = const std::string&, typename... ArgT>
  void set_message(ArgT0&& arg0, ArgT... args);
  std::string* mutable_message();
  PROTOBUF_MUST_USE_RESULT std::string* release_message();
  void set_allocated_message(std::string* message);

 private:
  const std::string& _internal_message() const;
  inline PROTOBUF_ALWAYS_INLINE void _internal_set_message(
      const std::string& value);
  std::string* _internal_mutable_message();

 public:
  // fixed32 version = 1;
  void clear_version();
  ::PROTOBUF_NAMESPACE_ID::uint32 version() const;
  void set_version(::PROTOBUF_NAMESPACE_ID::uint32 value);

 private:
  ::PROTOBUF_NAMESPACE_ID::uint32 _internal_version() const;
  void _internal_set_version(::PROTOBUF_NAMESPACE_ID::uint32 value);

 public:
  // fixed32 magic = 2;
  void clear_magic();
  ::PROTOBUF_NAMESPACE_ID::uint32 magic() const;
  void set_magic(::PROTOBUF_NAMESPACE_ID::uint32 value);

 private:
  ::PROTOBUF_NAMESPACE_ID::uint32 _internal_magic() const;
  void _internal_set_magic(::PROTOBUF_NAMESPACE_ID::uint32 value);

 public:
  // fixed32 type = 3;
  void clear_type();
  ::PROTOBUF_NAMESPACE_ID::uint32 type() const;
  void set_type(::PROTOBUF_NAMESPACE_ID::uint32 value);

 private:
  ::PROTOBUF_NAMESPACE_ID::uint32 _internal_type() const;
  void _internal_set_type(::PROTOBUF_NAMESPACE_ID::uint32 value);

 public:
  // fixed32 length = 4;
  void clear_length();
  ::PROTOBUF_NAMESPACE_ID::uint32 length() const;
  void set_length(::PROTOBUF_NAMESPACE_ID::uint32 value);

 private:
  ::PROTOBUF_NAMESPACE_ID::uint32 _internal_length() const;
  void _internal_set_length(::PROTOBUF_NAMESPACE_ID::uint32 value);

 public:
  // .frpc.CompressType request_compress_type = 8;
  void clear_request_compress_type();
  ::frpc::CompressType request_compress_type() const;
  void set_request_compress_type(::frpc::CompressType value);

 private:
  ::frpc::CompressType _internal_request_compress_type() const;
  void _internal_set_request_compress_type(::frpc::CompressType value);

 public:
  // .frpc.CompressType response_compress_type = 9;
  void clear_response_compress_type();
  ::frpc::CompressType response_compress_type() const;
  void set_response_compress_type(::frpc::CompressType value);

 private:
  ::frpc::CompressType _internal_response_compress_type() const;
  void _internal_set_response_compress_type(::frpc::CompressType value);

 public:
  // int32 retcode = 10;
  void clear_retcode();
  ::PROTOBUF_NAMESPACE_ID::int32 retcode() const;
  void set_retcode(::PROTOBUF_NAMESPACE_ID::int32 value);

 private:
  ::PROTOBUF_NAMESPACE_ID::int32 _internal_retcode() const;
  void _internal_set_retcode(::PROTOBUF_NAMESPACE_ID::int32 value);

 public:
  // @@protoc_insertion_point(class_scope:frpc.FastMessage)
 private:
  class _Internal;

  template <typename T>
  friend class ::PROTOBUF_NAMESPACE_ID::Arena::InternalHelper;
  typedef void InternalArenaConstructable_;
  typedef void DestructorSkippable_;
  ::PROTOBUF_NAMESPACE_ID::internal::ArenaStringPtr call_id_;
  ::PROTOBUF_NAMESPACE_ID::internal::ArenaStringPtr method_;
  ::PROTOBUF_NAMESPACE_ID::internal::ArenaStringPtr service_;
  ::PROTOBUF_NAMESPACE_ID::internal::ArenaStringPtr message_;
  ::PROTOBUF_NAMESPACE_ID::uint32 version_;
  ::PROTOBUF_NAMESPACE_ID::uint32 magic_;
  ::PROTOBUF_NAMESPACE_ID::uint32 type_;
  ::PROTOBUF_NAMESPACE_ID::uint32 length_;
  int request_compress_type_;
  int response_compress_type_;
  ::PROTOBUF_NAMESPACE_ID::int32 retcode_;
  mutable ::PROTOBUF_NAMESPACE_ID::internal::CachedSize _cached_size_;
  friend struct ::TableStruct_frpc_2eproto;
};
// ===================================================================

// ===================================================================

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif  // __GNUC__
// FastMessage

// fixed32 version = 1;
inline void FastMessage::clear_version() { version_ = 0u; }
inline ::PROTOBUF_NAMESPACE_ID::uint32 FastMessage::_internal_version() const {
  return version_;
}
inline ::PROTOBUF_NAMESPACE_ID::uint32 FastMessage::version() const {
  // @@protoc_insertion_point(field_get:frpc.FastMessage.version)
  return _internal_version();
}
inline void FastMessage::_internal_set_version(
    ::PROTOBUF_NAMESPACE_ID::uint32 value) {

  version_ = value;
}
inline void FastMessage::set_version(::PROTOBUF_NAMESPACE_ID::uint32 value) {
  _internal_set_version(value);
  // @@protoc_insertion_point(field_set:frpc.FastMessage.version)
}

// fixed32 magic = 2;
inline void FastMessage::clear_magic() { magic_ = 0u; }
inline ::PROTOBUF_NAMESPACE_ID::uint32 FastMessage::_internal_magic() const {
  return magic_;
}
inline ::PROTOBUF_NAMESPACE_ID::uint32 FastMessage::magic() const {
  // @@protoc_insertion_point(field_get:frpc.FastMessage.magic)
  return _internal_magic();
}
inline void FastMessage::_internal_set_magic(
    ::PROTOBUF_NAMESPACE_ID::uint32 value) {

  magic_ = value;
}
inline void FastMessage::set_magic(::PROTOBUF_NAMESPACE_ID::uint32 value) {
  _internal_set_magic(value);
  // @@protoc_insertion_point(field_set:frpc.FastMessage.magic)
}

// fixed32 type = 3;
inline void FastMessage::clear_type() { type_ = 0u; }
inline ::PROTOBUF_NAMESPACE_ID::uint32 FastMessage::_internal_type() const {
  return type_;
}
inline ::PROTOBUF_NAMESPACE_ID::uint32 FastMessage::type() const {
  // @@protoc_insertion_point(field_get:frpc.FastMessage.type)
  return _internal_type();
}
inline void FastMessage::_internal_set_type(
    ::PROTOBUF_NAMESPACE_ID::uint32 value) {

  type_ = value;
}
inline void FastMessage::set_type(::PROTOBUF_NAMESPACE_ID::uint32 value) {
  _internal_set_type(value);
  // @@protoc_insertion_point(field_set:frpc.FastMessage.type)
}

// fixed32 length = 4;
inline void FastMessage::clear_length() { length_ = 0u; }
inline ::PROTOBUF_NAMESPACE_ID::uint32 FastMessage::_internal_length() const {
  return length_;
}
inline ::PROTOBUF_NAMESPACE_ID::uint32 FastMessage::length() const {
  // @@protoc_insertion_point(field_get:frpc.FastMessage.length)
  return _internal_length();
}
inline void FastMessage::_internal_set_length(
    ::PROTOBUF_NAMESPACE_ID::uint32 value) {

  length_ = value;
}
inline void FastMessage::set_length(::PROTOBUF_NAMESPACE_ID::uint32 value) {
  _internal_set_length(value);
  // @@protoc_insertion_point(field_set:frpc.FastMessage.length)
}

// string call_id = 5;
inline void FastMessage::clear_call_id() { call_id_.ClearToEmpty(); }
inline const std::string& FastMessage::call_id() const {
  // @@protoc_insertion_point(field_get:frpc.FastMessage.call_id)
  return _internal_call_id();
}
template <typename ArgT0, typename... ArgT>
inline PROTOBUF_ALWAYS_INLINE void FastMessage::set_call_id(ArgT0&& arg0,
                                                            ArgT... args) {

  call_id_.Set(
      ::PROTOBUF_NAMESPACE_ID::internal::ArenaStringPtr::EmptyDefault{},
      static_cast<ArgT0&&>(arg0), args..., GetArenaForAllocation());
  // @@protoc_insertion_point(field_set:frpc.FastMessage.call_id)
}
inline std::string* FastMessage::mutable_call_id() {
  std::string* _s = _internal_mutable_call_id();
  // @@protoc_insertion_point(field_mutable:frpc.FastMessage.call_id)
  return _s;
}
inline const std::string& FastMessage::_internal_call_id() const {
  return call_id_.Get();
}
inline void FastMessage::_internal_set_call_id(const std::string& value) {

  call_id_.Set(
      ::PROTOBUF_NAMESPACE_ID::internal::ArenaStringPtr::EmptyDefault{}, value,
      GetArenaForAllocation());
}
inline std::string* FastMessage::_internal_mutable_call_id() {

  return call_id_.Mutable(
      ::PROTOBUF_NAMESPACE_ID::internal::ArenaStringPtr::EmptyDefault{},
      GetArenaForAllocation());
}
inline std::string* FastMessage::release_call_id() {
  // @@protoc_insertion_point(field_release:frpc.FastMessage.call_id)
  return call_id_.Release(
      &::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited(),
      GetArenaForAllocation());
}
inline void FastMessage::set_allocated_call_id(std::string* call_id) {
  if (call_id != nullptr) {

  } else {
  }
  call_id_.SetAllocated(
      &::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited(),
      call_id, GetArenaForAllocation());
  // @@protoc_insertion_point(field_set_allocated:frpc.FastMessage.call_id)
}

// string method = 6;
inline void FastMessage::clear_method() { method_.ClearToEmpty(); }
inline const std::string& FastMessage::method() const {
  // @@protoc_insertion_point(field_get:frpc.FastMessage.method)
  return _internal_method();
}
template <typename ArgT0, typename... ArgT>
inline PROTOBUF_ALWAYS_INLINE void FastMessage::set_method(ArgT0&& arg0,
                                                           ArgT... args) {

  method_.Set(::PROTOBUF_NAMESPACE_ID::internal::ArenaStringPtr::EmptyDefault{},
              static_cast<ArgT0&&>(arg0), args..., GetArenaForAllocation());
  // @@protoc_insertion_point(field_set:frpc.FastMessage.method)
}
inline std::string* FastMessage::mutable_method() {
  std::string* _s = _internal_mutable_method();
  // @@protoc_insertion_point(field_mutable:frpc.FastMessage.method)
  return _s;
}
inline const std::string& FastMessage::_internal_method() const {
  return method_.Get();
}
inline void FastMessage::_internal_set_method(const std::string& value) {

  method_.Set(::PROTOBUF_NAMESPACE_ID::internal::ArenaStringPtr::EmptyDefault{},
              value, GetArenaForAllocation());
}
inline std::string* FastMessage::_internal_mutable_method() {

  return method_.Mutable(
      ::PROTOBUF_NAMESPACE_ID::internal::ArenaStringPtr::EmptyDefault{},
      GetArenaForAllocation());
}
inline std::string* FastMessage::release_method() {
  // @@protoc_insertion_point(field_release:frpc.FastMessage.method)
  return method_.Release(
      &::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited(),
      GetArenaForAllocation());
}
inline void FastMessage::set_allocated_method(std::string* method) {
  if (method != nullptr) {

  } else {
  }
  method_.SetAllocated(
      &::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited(), method,
      GetArenaForAllocation());
  // @@protoc_insertion_point(field_set_allocated:frpc.FastMessage.method)
}

// string service = 7;
inline void FastMessage::clear_service() { service_.ClearToEmpty(); }
inline const std::string& FastMessage::service() const {
  // @@protoc_insertion_point(field_get:frpc.FastMessage.service)
  return _internal_service();
}
template <typename ArgT0, typename... ArgT>
inline PROTOBUF_ALWAYS_INLINE void FastMessage::set_service(ArgT0&& arg0,
                                                            ArgT... args) {

  service_.Set(
      ::PROTOBUF_NAMESPACE_ID::internal::ArenaStringPtr::EmptyDefault{},
      static_cast<ArgT0&&>(arg0), args..., GetArenaForAllocation());
  // @@protoc_insertion_point(field_set:frpc.FastMessage.service)
}
inline std::string* FastMessage::mutable_service() {
  std::string* _s = _internal_mutable_service();
  // @@protoc_insertion_point(field_mutable:frpc.FastMessage.service)
  return _s;
}
inline const std::string& FastMessage::_internal_service() const {
  return service_.Get();
}
inline void FastMessage::_internal_set_service(const std::string& value) {

  service_.Set(
      ::PROTOBUF_NAMESPACE_ID::internal::ArenaStringPtr::EmptyDefault{}, value,
      GetArenaForAllocation());
}
inline std::string* FastMessage::_internal_mutable_service() {

  return service_.Mutable(
      ::PROTOBUF_NAMESPACE_ID::internal::ArenaStringPtr::EmptyDefault{},
      GetArenaForAllocation());
}
inline std::string* FastMessage::release_service() {
  // @@protoc_insertion_point(field_release:frpc.FastMessage.service)
  return service_.Release(
      &::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited(),
      GetArenaForAllocation());
}
inline void FastMessage::set_allocated_service(std::string* service) {
  if (service != nullptr) {

  } else {
  }
  service_.SetAllocated(
      &::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited(),
      service, GetArenaForAllocation());
  // @@protoc_insertion_point(field_set_allocated:frpc.FastMessage.service)
}

// .frpc.CompressType request_compress_type = 8;
inline void FastMessage::clear_request_compress_type() {
  request_compress_type_ = 0;
}
inline ::frpc::CompressType FastMessage::_internal_request_compress_type()
    const {
  return static_cast<::frpc::CompressType>(request_compress_type_);
}
inline ::frpc::CompressType FastMessage::request_compress_type() const {
  // @@protoc_insertion_point(field_get:frpc.FastMessage.request_compress_type)
  return _internal_request_compress_type();
}
inline void FastMessage::_internal_set_request_compress_type(
    ::frpc::CompressType value) {

  request_compress_type_ = value;
}
inline void FastMessage::set_request_compress_type(::frpc::CompressType value) {
  _internal_set_request_compress_type(value);
  // @@protoc_insertion_point(field_set:frpc.FastMessage.request_compress_type)
}

// .frpc.CompressType response_compress_type = 9;
inline void FastMessage::clear_response_compress_type() {
  response_compress_type_ = 0;
}
inline ::frpc::CompressType FastMessage::_internal_response_compress_type()
    const {
  return static_cast<::frpc::CompressType>(response_compress_type_);
}
inline ::frpc::CompressType FastMessage::response_compress_type() const {
  // @@protoc_insertion_point(field_get:frpc.FastMessage.response_compress_type)
  return _internal_response_compress_type();
}
inline void FastMessage::_internal_set_response_compress_type(
    ::frpc::CompressType value) {

  response_compress_type_ = value;
}
inline void FastMessage::set_response_compress_type(
    ::frpc::CompressType value) {
  _internal_set_response_compress_type(value);
  // @@protoc_insertion_point(field_set:frpc.FastMessage.response_compress_type)
}

// int32 retcode = 10;
inline void FastMessage::clear_retcode() { retcode_ = 0; }
inline ::PROTOBUF_NAMESPACE_ID::int32 FastMessage::_internal_retcode() const {
  return retcode_;
}
inline ::PROTOBUF_NAMESPACE_ID::int32 FastMessage::retcode() const {
  // @@protoc_insertion_point(field_get:frpc.FastMessage.retcode)
  return _internal_retcode();
}
inline void FastMessage::_internal_set_retcode(
    ::PROTOBUF_NAMESPACE_ID::int32 value) {

  retcode_ = value;
}
inline void FastMessage::set_retcode(::PROTOBUF_NAMESPACE_ID::int32 value) {
  _internal_set_retcode(value);
  // @@protoc_insertion_point(field_set:frpc.FastMessage.retcode)
}

// string message = 11;
inline void FastMessage::clear_message() { message_.ClearToEmpty(); }
inline const std::string& FastMessage::message() const {
  // @@protoc_insertion_point(field_get:frpc.FastMessage.message)
  return _internal_message();
}
template <typename ArgT0, typename... ArgT>
inline PROTOBUF_ALWAYS_INLINE void FastMessage::set_message(ArgT0&& arg0,
                                                            ArgT... args) {

  message_.Set(
      ::PROTOBUF_NAMESPACE_ID::internal::ArenaStringPtr::EmptyDefault{},
      static_cast<ArgT0&&>(arg0), args..., GetArenaForAllocation());
  // @@protoc_insertion_point(field_set:frpc.FastMessage.message)
}
inline std::string* FastMessage::mutable_message() {
  std::string* _s = _internal_mutable_message();
  // @@protoc_insertion_point(field_mutable:frpc.FastMessage.message)
  return _s;
}
inline const std::string& FastMessage::_internal_message() const {
  return message_.Get();
}
inline void FastMessage::_internal_set_message(const std::string& value) {

  message_.Set(
      ::PROTOBUF_NAMESPACE_ID::internal::ArenaStringPtr::EmptyDefault{}, value,
      GetArenaForAllocation());
}
inline std::string* FastMessage::_internal_mutable_message() {

  return message_.Mutable(
      ::PROTOBUF_NAMESPACE_ID::internal::ArenaStringPtr::EmptyDefault{},
      GetArenaForAllocation());
}
inline std::string* FastMessage::release_message() {
  // @@protoc_insertion_point(field_release:frpc.FastMessage.message)
  return message_.Release(
      &::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited(),
      GetArenaForAllocation());
}
inline void FastMessage::set_allocated_message(std::string* message) {
  if (message != nullptr) {

  } else {
  }
  message_.SetAllocated(
      &::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited(),
      message, GetArenaForAllocation());
  // @@protoc_insertion_point(field_set_allocated:frpc.FastMessage.message)
}

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif  // __GNUC__

// @@protoc_insertion_point(namespace_scope)

}  // namespace frpc

PROTOBUF_NAMESPACE_OPEN

template <>
struct is_proto_enum<::frpc::MessageType> : ::std::true_type {};
template <>
inline const EnumDescriptor* GetEnumDescriptor<::frpc::MessageType>() {
  return ::frpc::MessageType_descriptor();
}
template <>
struct is_proto_enum<::frpc::MessageID> : ::std::true_type {};
template <>
inline const EnumDescriptor* GetEnumDescriptor<::frpc::MessageID>() {
  return ::frpc::MessageID_descriptor();
}
template <>
struct is_proto_enum<::frpc::ReturnCode> : ::std::true_type {};
template <>
inline const EnumDescriptor* GetEnumDescriptor<::frpc::ReturnCode>() {
  return ::frpc::ReturnCode_descriptor();
}
template <>
struct is_proto_enum<::frpc::CompressType> : ::std::true_type {};
template <>
inline const EnumDescriptor* GetEnumDescriptor<::frpc::CompressType>() {
  return ::frpc::CompressType_descriptor();
}

PROTOBUF_NAMESPACE_CLOSE

// @@protoc_insertion_point(global_scope)

#include <google/protobuf/port_undef.inc>
#endif  // GOOGLE_PROTOBUF_INCLUDED_GOOGLE_PROTOBUF_INCLUDED_frpc_2eproto
