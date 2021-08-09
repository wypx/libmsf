// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: frpc.proto

#include "frpc.pb.h"

#include <algorithm>

#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/extension_set.h>
#include <google/protobuf/wire_format_lite.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/generated_message_reflection.h>
#include <google/protobuf/reflection_ops.h>
#include <google/protobuf/wire_format.h>
// @@protoc_insertion_point(includes)
#include <google/protobuf/port_def.inc>

PROTOBUF_PRAGMA_INIT_SEG
namespace frpc {
constexpr FastMessage::FastMessage(
  ::PROTOBUF_NAMESPACE_ID::internal::ConstantInitialized)
  : version_(0u)
  , magic_(0u)
  , type_(0u)
  , length_(0u)
  , call_id_(0u)
  , opcode_(0u)
  , compress_(0u)
  , retcode_(0u){}
struct FastMessageDefaultTypeInternal {
  constexpr FastMessageDefaultTypeInternal()
    : _instance(::PROTOBUF_NAMESPACE_ID::internal::ConstantInitialized{}) {}
  ~FastMessageDefaultTypeInternal() {}
  union {
    FastMessage _instance;
  };
};
PROTOBUF_ATTRIBUTE_NO_DESTROY PROTOBUF_CONSTINIT FastMessageDefaultTypeInternal _FastMessage_default_instance_;
}  // namespace frpc
static ::PROTOBUF_NAMESPACE_ID::Metadata file_level_metadata_frpc_2eproto[1];
static const ::PROTOBUF_NAMESPACE_ID::EnumDescriptor* file_level_enum_descriptors_frpc_2eproto[4];
static constexpr ::PROTOBUF_NAMESPACE_ID::ServiceDescriptor const** file_level_service_descriptors_frpc_2eproto = nullptr;

const ::PROTOBUF_NAMESPACE_ID::uint32 TableStruct_frpc_2eproto::offsets[] PROTOBUF_SECTION_VARIABLE(protodesc_cold) = {
  ~0u,  // no _has_bits_
  PROTOBUF_FIELD_OFFSET(::frpc::FastMessage, _internal_metadata_),
  ~0u,  // no _extensions_
  ~0u,  // no _oneof_case_
  ~0u,  // no _weak_field_map_
  PROTOBUF_FIELD_OFFSET(::frpc::FastMessage, version_),
  PROTOBUF_FIELD_OFFSET(::frpc::FastMessage, magic_),
  PROTOBUF_FIELD_OFFSET(::frpc::FastMessage, type_),
  PROTOBUF_FIELD_OFFSET(::frpc::FastMessage, length_),
  PROTOBUF_FIELD_OFFSET(::frpc::FastMessage, call_id_),
  PROTOBUF_FIELD_OFFSET(::frpc::FastMessage, opcode_),
  PROTOBUF_FIELD_OFFSET(::frpc::FastMessage, compress_),
  PROTOBUF_FIELD_OFFSET(::frpc::FastMessage, retcode_),
};
static const ::PROTOBUF_NAMESPACE_ID::internal::MigrationSchema schemas[] PROTOBUF_SECTION_VARIABLE(protodesc_cold) = {
  { 0, -1, sizeof(::frpc::FastMessage)},
};

static ::PROTOBUF_NAMESPACE_ID::Message const * const file_default_instances[] = {
  reinterpret_cast<const ::PROTOBUF_NAMESPACE_ID::Message*>(&::frpc::_FastMessage_default_instance_),
};

const char descriptor_table_protodef_frpc_2eproto[] PROTOBUF_SECTION_VARIABLE(protodesc_cold) =
  "\n\nfrpc.proto\022\004frpc\"\217\001\n\013FastMessage\022\017\n\007ve"
  "rsion\030\001 \001(\007\022\r\n\005magic\030\002 \001(\007\022\014\n\004type\030\003 \001(\007"
  "\022\016\n\006length\030\004 \001(\007\022\017\n\007call_id\030\005 \001(\007\022\016\n\006opc"
  "ode\030\010 \001(\007\022\020\n\010compress\030\t \001(\007\022\017\n\007retcode\030\n"
  " \001(\007*u\n\013MessageType\022\030\n\024FRPC_MESSAGE_UNKN"
  "OWN\020\000\022\030\n\024FRPC_MESSAGE_REQUEST\020\001\022\031\n\025FRPC_"
  "MESSAGE_RESPONSE\020\002\022\027\n\023FRPC_MESSAGE_CANCE"
  "L\020\003*/\n\tMessageID\022\016\n\nFRPC_LOGIN\020\000\022\022\n\016FRPC"
  "_HEARTBEAT\020\001*\302\006\n\nReturnCode\022\020\n\014FRPC_SUCC"
  "ESS\020\000\022\023\n\017FRPC_NO_SERVICE\020\001\022\022\n\016FRPC_NO_ME"
  "THOD\020\002\022\030\n\024FRPC_INVALID_REQUEST\020\003\022\030\n\024FRPC"
  "_INVALID_REPONSE\020\004\022\033\n\027RPC_ERROR_PARSE_RE"
  "QUEST\020\005\022\032\n\026RPC_ERROR_PARSE_REPONS\020\006\022\033\n\027R"
  "PC_ERROR_COMPRESS_TYPE\020\007\022\034\n\030RPC_ERROR_NO"
  "_METHOD_NAME\020\010\022\037\n\033RPC_ERROR_PARSE_METHOD"
  "_NAME\020\t\022\033\n\027RPC_ERROR_FOUND_SERVICE\020\n\022\032\n\026"
  "RPC_ERROR_FOUND_METHOD\020\013\022\034\n\030RPC_ERROR_CH"
  "ANNEL_BROKEN\020\014\022\037\n\033RPC_ERROR_CONNECTION_C"
  "LOSED\020\r\022\035\n\031RPC_ERROR_REQUEST_TIMEOUT\020\016\022\036"
  "\n\032RPC_ERROR_REQUEST_CANCELED\020\017\022 \n\034RPC_ER"
  "ROR_SERVER_UNAVAILABLE\020\020\022 \n\034RPC_ERROR_SE"
  "RVER_UNREACHABLE\020\021\022\035\n\031RPC_ERROR_SERVER_S"
  "HUTDOWN\020\022\022\036\n\032RPC_ERROR_SEND_BUFFER_FULL\020"
  "\023\022\037\n\033RPC_ERROR_SERIALIZE_REQUEST\020\024\022 \n\034RP"
  "C_ERROR_SERIALIZE_RESPONSE\020\025\022\035\n\031RPC_ERRO"
  "R_RESOLVE_ADDRESS\020\026\022\033\n\027RPC_ERROR_CREATE_"
  "STREAM\020\027\022\034\n\030RPC_ERROR_NOT_IN_RUNNING\020\030\022\031"
  "\n\025RPC_ERROR_SERVER_BUSY\020\031\022!\n\035RPC_ERROR_T"
  "OO_MANY_OPEN_FILES\020\032\022\033\n\027RPC_ERROR_RESON_"
  "UNKNOWN\020\033*\222\001\n\014CompressType\022\024\n\020CompressTy"
  "peNone\020\000\022\024\n\020CompressTypeGzip\020\001\022\024\n\020Compre"
  "ssTypeZlib\020\002\022\026\n\022CompressTypeSnappy\020\003\022\023\n\017"
  "CompressTypeLZ4\020\004\022\023\n\017CompressTypeMax\020\005B\003"
  "\200\001\001b\006proto3"
  ;
static ::PROTOBUF_NAMESPACE_ID::internal::once_flag descriptor_table_frpc_2eproto_once;
const ::PROTOBUF_NAMESPACE_ID::internal::DescriptorTable descriptor_table_frpc_2eproto = {
  false, false, 1331, descriptor_table_protodef_frpc_2eproto, "frpc.proto", 
  &descriptor_table_frpc_2eproto_once, nullptr, 0, 1,
  schemas, file_default_instances, TableStruct_frpc_2eproto::offsets,
  file_level_metadata_frpc_2eproto, file_level_enum_descriptors_frpc_2eproto, file_level_service_descriptors_frpc_2eproto,
};
PROTOBUF_ATTRIBUTE_WEAK const ::PROTOBUF_NAMESPACE_ID::internal::DescriptorTable* descriptor_table_frpc_2eproto_getter() {
  return &descriptor_table_frpc_2eproto;
}

// Force running AddDescriptors() at dynamic initialization time.
PROTOBUF_ATTRIBUTE_INIT_PRIORITY static ::PROTOBUF_NAMESPACE_ID::internal::AddDescriptorsRunner dynamic_init_dummy_frpc_2eproto(&descriptor_table_frpc_2eproto);
namespace frpc {
const ::PROTOBUF_NAMESPACE_ID::EnumDescriptor* MessageType_descriptor() {
  ::PROTOBUF_NAMESPACE_ID::internal::AssignDescriptors(&descriptor_table_frpc_2eproto);
  return file_level_enum_descriptors_frpc_2eproto[0];
}
bool MessageType_IsValid(int value) {
  switch (value) {
    case 0:
    case 1:
    case 2:
    case 3:
      return true;
    default:
      return false;
  }
}

const ::PROTOBUF_NAMESPACE_ID::EnumDescriptor* MessageID_descriptor() {
  ::PROTOBUF_NAMESPACE_ID::internal::AssignDescriptors(&descriptor_table_frpc_2eproto);
  return file_level_enum_descriptors_frpc_2eproto[1];
}
bool MessageID_IsValid(int value) {
  switch (value) {
    case 0:
    case 1:
      return true;
    default:
      return false;
  }
}

const ::PROTOBUF_NAMESPACE_ID::EnumDescriptor* ReturnCode_descriptor() {
  ::PROTOBUF_NAMESPACE_ID::internal::AssignDescriptors(&descriptor_table_frpc_2eproto);
  return file_level_enum_descriptors_frpc_2eproto[2];
}
bool ReturnCode_IsValid(int value) {
  switch (value) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
    case 10:
    case 11:
    case 12:
    case 13:
    case 14:
    case 15:
    case 16:
    case 17:
    case 18:
    case 19:
    case 20:
    case 21:
    case 22:
    case 23:
    case 24:
    case 25:
    case 26:
    case 27:
      return true;
    default:
      return false;
  }
}

const ::PROTOBUF_NAMESPACE_ID::EnumDescriptor* CompressType_descriptor() {
  ::PROTOBUF_NAMESPACE_ID::internal::AssignDescriptors(&descriptor_table_frpc_2eproto);
  return file_level_enum_descriptors_frpc_2eproto[3];
}
bool CompressType_IsValid(int value) {
  switch (value) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
      return true;
    default:
      return false;
  }
}


// ===================================================================

class FastMessage::_Internal {
 public:
};

FastMessage::FastMessage(::PROTOBUF_NAMESPACE_ID::Arena* arena,
                         bool is_message_owned)
  : ::PROTOBUF_NAMESPACE_ID::Message(arena, is_message_owned) {
  SharedCtor();
  if (!is_message_owned) {
    RegisterArenaDtor(arena);
  }
  // @@protoc_insertion_point(arena_constructor:frpc.FastMessage)
}
FastMessage::FastMessage(const FastMessage& from)
  : ::PROTOBUF_NAMESPACE_ID::Message() {
  _internal_metadata_.MergeFrom<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(from._internal_metadata_);
  ::memcpy(&version_, &from.version_,
    static_cast<size_t>(reinterpret_cast<char*>(&retcode_) -
    reinterpret_cast<char*>(&version_)) + sizeof(retcode_));
  // @@protoc_insertion_point(copy_constructor:frpc.FastMessage)
}

void FastMessage::SharedCtor() {
::memset(reinterpret_cast<char*>(this) + static_cast<size_t>(
    reinterpret_cast<char*>(&version_) - reinterpret_cast<char*>(this)),
    0, static_cast<size_t>(reinterpret_cast<char*>(&retcode_) -
    reinterpret_cast<char*>(&version_)) + sizeof(retcode_));
}

FastMessage::~FastMessage() {
  // @@protoc_insertion_point(destructor:frpc.FastMessage)
  if (GetArenaForAllocation() != nullptr) return;
  SharedDtor();
  _internal_metadata_.Delete<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>();
}

inline void FastMessage::SharedDtor() {
  GOOGLE_DCHECK(GetArenaForAllocation() == nullptr);
}

void FastMessage::ArenaDtor(void* object) {
  FastMessage* _this = reinterpret_cast< FastMessage* >(object);
  (void)_this;
}
void FastMessage::RegisterArenaDtor(::PROTOBUF_NAMESPACE_ID::Arena*) {
}
void FastMessage::SetCachedSize(int size) const {
  _cached_size_.Set(size);
}

void FastMessage::Clear() {
// @@protoc_insertion_point(message_clear_start:frpc.FastMessage)
  ::PROTOBUF_NAMESPACE_ID::uint32 cached_has_bits = 0;
  // Prevent compiler warnings about cached_has_bits being unused
  (void) cached_has_bits;

  ::memset(&version_, 0, static_cast<size_t>(
      reinterpret_cast<char*>(&retcode_) -
      reinterpret_cast<char*>(&version_)) + sizeof(retcode_));
  _internal_metadata_.Clear<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>();
}

const char* FastMessage::_InternalParse(const char* ptr, ::PROTOBUF_NAMESPACE_ID::internal::ParseContext* ctx) {
#define CHK_(x) if (PROTOBUF_PREDICT_FALSE(!(x))) goto failure
  while (!ctx->Done(&ptr)) {
    ::PROTOBUF_NAMESPACE_ID::uint32 tag;
    ptr = ::PROTOBUF_NAMESPACE_ID::internal::ReadTag(ptr, &tag);
    switch (tag >> 3) {
      // fixed32 version = 1;
      case 1:
        if (PROTOBUF_PREDICT_TRUE(static_cast<::PROTOBUF_NAMESPACE_ID::uint8>(tag) == 13)) {
          version_ = ::PROTOBUF_NAMESPACE_ID::internal::UnalignedLoad<::PROTOBUF_NAMESPACE_ID::uint32>(ptr);
          ptr += sizeof(::PROTOBUF_NAMESPACE_ID::uint32);
        } else goto handle_unusual;
        continue;
      // fixed32 magic = 2;
      case 2:
        if (PROTOBUF_PREDICT_TRUE(static_cast<::PROTOBUF_NAMESPACE_ID::uint8>(tag) == 21)) {
          magic_ = ::PROTOBUF_NAMESPACE_ID::internal::UnalignedLoad<::PROTOBUF_NAMESPACE_ID::uint32>(ptr);
          ptr += sizeof(::PROTOBUF_NAMESPACE_ID::uint32);
        } else goto handle_unusual;
        continue;
      // fixed32 type = 3;
      case 3:
        if (PROTOBUF_PREDICT_TRUE(static_cast<::PROTOBUF_NAMESPACE_ID::uint8>(tag) == 29)) {
          type_ = ::PROTOBUF_NAMESPACE_ID::internal::UnalignedLoad<::PROTOBUF_NAMESPACE_ID::uint32>(ptr);
          ptr += sizeof(::PROTOBUF_NAMESPACE_ID::uint32);
        } else goto handle_unusual;
        continue;
      // fixed32 length = 4;
      case 4:
        if (PROTOBUF_PREDICT_TRUE(static_cast<::PROTOBUF_NAMESPACE_ID::uint8>(tag) == 37)) {
          length_ = ::PROTOBUF_NAMESPACE_ID::internal::UnalignedLoad<::PROTOBUF_NAMESPACE_ID::uint32>(ptr);
          ptr += sizeof(::PROTOBUF_NAMESPACE_ID::uint32);
        } else goto handle_unusual;
        continue;
      // fixed32 call_id = 5;
      case 5:
        if (PROTOBUF_PREDICT_TRUE(static_cast<::PROTOBUF_NAMESPACE_ID::uint8>(tag) == 45)) {
          call_id_ = ::PROTOBUF_NAMESPACE_ID::internal::UnalignedLoad<::PROTOBUF_NAMESPACE_ID::uint32>(ptr);
          ptr += sizeof(::PROTOBUF_NAMESPACE_ID::uint32);
        } else goto handle_unusual;
        continue;
      // fixed32 opcode = 8;
      case 8:
        if (PROTOBUF_PREDICT_TRUE(static_cast<::PROTOBUF_NAMESPACE_ID::uint8>(tag) == 69)) {
          opcode_ = ::PROTOBUF_NAMESPACE_ID::internal::UnalignedLoad<::PROTOBUF_NAMESPACE_ID::uint32>(ptr);
          ptr += sizeof(::PROTOBUF_NAMESPACE_ID::uint32);
        } else goto handle_unusual;
        continue;
      // fixed32 compress = 9;
      case 9:
        if (PROTOBUF_PREDICT_TRUE(static_cast<::PROTOBUF_NAMESPACE_ID::uint8>(tag) == 77)) {
          compress_ = ::PROTOBUF_NAMESPACE_ID::internal::UnalignedLoad<::PROTOBUF_NAMESPACE_ID::uint32>(ptr);
          ptr += sizeof(::PROTOBUF_NAMESPACE_ID::uint32);
        } else goto handle_unusual;
        continue;
      // fixed32 retcode = 10;
      case 10:
        if (PROTOBUF_PREDICT_TRUE(static_cast<::PROTOBUF_NAMESPACE_ID::uint8>(tag) == 85)) {
          retcode_ = ::PROTOBUF_NAMESPACE_ID::internal::UnalignedLoad<::PROTOBUF_NAMESPACE_ID::uint32>(ptr);
          ptr += sizeof(::PROTOBUF_NAMESPACE_ID::uint32);
        } else goto handle_unusual;
        continue;
      default: {
      handle_unusual:
        if ((tag == 0) || ((tag & 7) == 4)) {
          CHK_(ptr);
          ctx->SetLastTag(tag);
          goto success;
        }
        ptr = UnknownFieldParse(tag,
            _internal_metadata_.mutable_unknown_fields<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(),
            ptr, ctx);
        CHK_(ptr != nullptr);
        continue;
      }
    }  // switch
  }  // while
success:
  return ptr;
failure:
  ptr = nullptr;
  goto success;
#undef CHK_
}

::PROTOBUF_NAMESPACE_ID::uint8* FastMessage::_InternalSerialize(
    ::PROTOBUF_NAMESPACE_ID::uint8* target, ::PROTOBUF_NAMESPACE_ID::io::EpsCopyOutputStream* stream) const {
  // @@protoc_insertion_point(serialize_to_array_start:frpc.FastMessage)
  ::PROTOBUF_NAMESPACE_ID::uint32 cached_has_bits = 0;
  (void) cached_has_bits;

  // fixed32 version = 1;
  if (this->_internal_version() != 0) {
    target = stream->EnsureSpace(target);
    target = ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::WriteFixed32ToArray(1, this->_internal_version(), target);
  }

  // fixed32 magic = 2;
  if (this->_internal_magic() != 0) {
    target = stream->EnsureSpace(target);
    target = ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::WriteFixed32ToArray(2, this->_internal_magic(), target);
  }

  // fixed32 type = 3;
  if (this->_internal_type() != 0) {
    target = stream->EnsureSpace(target);
    target = ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::WriteFixed32ToArray(3, this->_internal_type(), target);
  }

  // fixed32 length = 4;
  if (this->_internal_length() != 0) {
    target = stream->EnsureSpace(target);
    target = ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::WriteFixed32ToArray(4, this->_internal_length(), target);
  }

  // fixed32 call_id = 5;
  if (this->_internal_call_id() != 0) {
    target = stream->EnsureSpace(target);
    target = ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::WriteFixed32ToArray(5, this->_internal_call_id(), target);
  }

  // fixed32 opcode = 8;
  if (this->_internal_opcode() != 0) {
    target = stream->EnsureSpace(target);
    target = ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::WriteFixed32ToArray(8, this->_internal_opcode(), target);
  }

  // fixed32 compress = 9;
  if (this->_internal_compress() != 0) {
    target = stream->EnsureSpace(target);
    target = ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::WriteFixed32ToArray(9, this->_internal_compress(), target);
  }

  // fixed32 retcode = 10;
  if (this->_internal_retcode() != 0) {
    target = stream->EnsureSpace(target);
    target = ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::WriteFixed32ToArray(10, this->_internal_retcode(), target);
  }

  if (PROTOBUF_PREDICT_FALSE(_internal_metadata_.have_unknown_fields())) {
    target = ::PROTOBUF_NAMESPACE_ID::internal::WireFormat::InternalSerializeUnknownFieldsToArray(
        _internal_metadata_.unknown_fields<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(::PROTOBUF_NAMESPACE_ID::UnknownFieldSet::default_instance), target, stream);
  }
  // @@protoc_insertion_point(serialize_to_array_end:frpc.FastMessage)
  return target;
}

size_t FastMessage::ByteSizeLong() const {
// @@protoc_insertion_point(message_byte_size_start:frpc.FastMessage)
  size_t total_size = 0;

  ::PROTOBUF_NAMESPACE_ID::uint32 cached_has_bits = 0;
  // Prevent compiler warnings about cached_has_bits being unused
  (void) cached_has_bits;

  // fixed32 version = 1;
  if (this->_internal_version() != 0) {
    total_size += 1 + 4;
  }

  // fixed32 magic = 2;
  if (this->_internal_magic() != 0) {
    total_size += 1 + 4;
  }

  // fixed32 type = 3;
  if (this->_internal_type() != 0) {
    total_size += 1 + 4;
  }

  // fixed32 length = 4;
  if (this->_internal_length() != 0) {
    total_size += 1 + 4;
  }

  // fixed32 call_id = 5;
  if (this->_internal_call_id() != 0) {
    total_size += 1 + 4;
  }

  // fixed32 opcode = 8;
  if (this->_internal_opcode() != 0) {
    total_size += 1 + 4;
  }

  // fixed32 compress = 9;
  if (this->_internal_compress() != 0) {
    total_size += 1 + 4;
  }

  // fixed32 retcode = 10;
  if (this->_internal_retcode() != 0) {
    total_size += 1 + 4;
  }

  if (PROTOBUF_PREDICT_FALSE(_internal_metadata_.have_unknown_fields())) {
    return ::PROTOBUF_NAMESPACE_ID::internal::ComputeUnknownFieldsSize(
        _internal_metadata_, total_size, &_cached_size_);
  }
  int cached_size = ::PROTOBUF_NAMESPACE_ID::internal::ToCachedSize(total_size);
  SetCachedSize(cached_size);
  return total_size;
}

const ::PROTOBUF_NAMESPACE_ID::Message::ClassData FastMessage::_class_data_ = {
    ::PROTOBUF_NAMESPACE_ID::Message::CopyWithSizeCheck,
    FastMessage::MergeImpl
};
const ::PROTOBUF_NAMESPACE_ID::Message::ClassData*FastMessage::GetClassData() const { return &_class_data_; }

void FastMessage::MergeImpl(::PROTOBUF_NAMESPACE_ID::Message*to,
                      const ::PROTOBUF_NAMESPACE_ID::Message&from) {
  static_cast<FastMessage *>(to)->MergeFrom(
      static_cast<const FastMessage &>(from));
}


void FastMessage::MergeFrom(const FastMessage& from) {
// @@protoc_insertion_point(class_specific_merge_from_start:frpc.FastMessage)
  GOOGLE_DCHECK_NE(&from, this);
  ::PROTOBUF_NAMESPACE_ID::uint32 cached_has_bits = 0;
  (void) cached_has_bits;

  if (from._internal_version() != 0) {
    _internal_set_version(from._internal_version());
  }
  if (from._internal_magic() != 0) {
    _internal_set_magic(from._internal_magic());
  }
  if (from._internal_type() != 0) {
    _internal_set_type(from._internal_type());
  }
  if (from._internal_length() != 0) {
    _internal_set_length(from._internal_length());
  }
  if (from._internal_call_id() != 0) {
    _internal_set_call_id(from._internal_call_id());
  }
  if (from._internal_opcode() != 0) {
    _internal_set_opcode(from._internal_opcode());
  }
  if (from._internal_compress() != 0) {
    _internal_set_compress(from._internal_compress());
  }
  if (from._internal_retcode() != 0) {
    _internal_set_retcode(from._internal_retcode());
  }
  _internal_metadata_.MergeFrom<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(from._internal_metadata_);
}

void FastMessage::CopyFrom(const FastMessage& from) {
// @@protoc_insertion_point(class_specific_copy_from_start:frpc.FastMessage)
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

bool FastMessage::IsInitialized() const {
  return true;
}

void FastMessage::InternalSwap(FastMessage* other) {
  using std::swap;
  _internal_metadata_.InternalSwap(&other->_internal_metadata_);
  ::PROTOBUF_NAMESPACE_ID::internal::memswap<
      PROTOBUF_FIELD_OFFSET(FastMessage, retcode_)
      + sizeof(FastMessage::retcode_)
      - PROTOBUF_FIELD_OFFSET(FastMessage, version_)>(
          reinterpret_cast<char*>(&version_),
          reinterpret_cast<char*>(&other->version_));
}

::PROTOBUF_NAMESPACE_ID::Metadata FastMessage::GetMetadata() const {
  return ::PROTOBUF_NAMESPACE_ID::internal::AssignDescriptors(
      &descriptor_table_frpc_2eproto_getter, &descriptor_table_frpc_2eproto_once,
      file_level_metadata_frpc_2eproto[0]);
}

// @@protoc_insertion_point(namespace_scope)
}  // namespace frpc
PROTOBUF_NAMESPACE_OPEN
template<> PROTOBUF_NOINLINE ::frpc::FastMessage* Arena::CreateMaybeMessage< ::frpc::FastMessage >(Arena* arena) {
  return Arena::CreateMessageInternal< ::frpc::FastMessage >(arena);
}
PROTOBUF_NAMESPACE_CLOSE

// @@protoc_insertion_point(global_scope)
#include <google/protobuf/port_undef.inc>
