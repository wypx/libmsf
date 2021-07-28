// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: leader_election.proto

#ifndef GOOGLE_PROTOBUF_INCLUDED_leader_5felection_2eproto
#define GOOGLE_PROTOBUF_INCLUDED_leader_5felection_2eproto

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
#include <google/protobuf/extension_set.h>  // IWYU pragma: export
#include <google/protobuf/service.h>
#include <google/protobuf/unknown_field_set.h>
// @@protoc_insertion_point(includes)
#include <google/protobuf/port_def.inc>
#define PROTOBUF_INTERNAL_EXPORT_leader_5felection_2eproto
PROTOBUF_NAMESPACE_OPEN
namespace internal {
class AnyMetadata;
}  // namespace internal
PROTOBUF_NAMESPACE_CLOSE

// Internal implementation detail -- do not use these members.
struct TableStruct_leader_5felection_2eproto {
  static const ::PROTOBUF_NAMESPACE_ID::internal::ParseTableField entries[]
    PROTOBUF_SECTION_VARIABLE(protodesc_cold);
  static const ::PROTOBUF_NAMESPACE_ID::internal::AuxiliaryParseTableField aux[]
    PROTOBUF_SECTION_VARIABLE(protodesc_cold);
  static const ::PROTOBUF_NAMESPACE_ID::internal::ParseTable schema[2]
    PROTOBUF_SECTION_VARIABLE(protodesc_cold);
  static const ::PROTOBUF_NAMESPACE_ID::internal::FieldMetadata field_metadata[];
  static const ::PROTOBUF_NAMESPACE_ID::internal::SerializationTable serialization_table[];
  static const ::PROTOBUF_NAMESPACE_ID::uint32 offsets[];
};
extern const ::PROTOBUF_NAMESPACE_ID::internal::DescriptorTable descriptor_table_leader_5felection_2eproto;
namespace fraft {
class Dummy;
struct DummyDefaultTypeInternal;
extern DummyDefaultTypeInternal _Dummy_default_instance_;
class Notification;
struct NotificationDefaultTypeInternal;
extern NotificationDefaultTypeInternal _Notification_default_instance_;
}  // namespace fraft
PROTOBUF_NAMESPACE_OPEN
template<> ::fraft::Dummy* Arena::CreateMaybeMessage<::fraft::Dummy>(Arena*);
template<> ::fraft::Notification* Arena::CreateMaybeMessage<::fraft::Notification>(Arena*);
PROTOBUF_NAMESPACE_CLOSE
namespace fraft {

// ===================================================================

class Dummy final :
    public ::PROTOBUF_NAMESPACE_ID::Message /* @@protoc_insertion_point(class_definition:fraft.Dummy) */ {
 public:
  inline Dummy() : Dummy(nullptr) {}
  ~Dummy() override;
  explicit constexpr Dummy(::PROTOBUF_NAMESPACE_ID::internal::ConstantInitialized);

  Dummy(const Dummy& from);
  Dummy(Dummy&& from) noexcept
    : Dummy() {
    *this = ::std::move(from);
  }

  inline Dummy& operator=(const Dummy& from) {
    CopyFrom(from);
    return *this;
  }
  inline Dummy& operator=(Dummy&& from) noexcept {
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
  static const Dummy& default_instance() {
    return *internal_default_instance();
  }
  static inline const Dummy* internal_default_instance() {
    return reinterpret_cast<const Dummy*>(
               &_Dummy_default_instance_);
  }
  static constexpr int kIndexInFileMessages =
    0;

  friend void swap(Dummy& a, Dummy& b) {
    a.Swap(&b);
  }
  inline void Swap(Dummy* other) {
    if (other == this) return;
    if (GetOwningArena() == other->GetOwningArena()) {
      InternalSwap(other);
    } else {
      ::PROTOBUF_NAMESPACE_ID::internal::GenericSwap(this, other);
    }
  }
  void UnsafeArenaSwap(Dummy* other) {
    if (other == this) return;
    GOOGLE_DCHECK(GetOwningArena() == other->GetOwningArena());
    InternalSwap(other);
  }

  // implements Message ----------------------------------------------

  inline Dummy* New() const final {
    return new Dummy();
  }

  Dummy* New(::PROTOBUF_NAMESPACE_ID::Arena* arena) const final {
    return CreateMaybeMessage<Dummy>(arena);
  }
  using ::PROTOBUF_NAMESPACE_ID::Message::CopyFrom;
  void CopyFrom(const Dummy& from);
  using ::PROTOBUF_NAMESPACE_ID::Message::MergeFrom;
  void MergeFrom(const Dummy& from);
  private:
  static void MergeImpl(::PROTOBUF_NAMESPACE_ID::Message*to, const ::PROTOBUF_NAMESPACE_ID::Message&from);
  public:
  PROTOBUF_ATTRIBUTE_REINITIALIZES void Clear() final;
  bool IsInitialized() const final;

  size_t ByteSizeLong() const final;
  const char* _InternalParse(const char* ptr, ::PROTOBUF_NAMESPACE_ID::internal::ParseContext* ctx) final;
  ::PROTOBUF_NAMESPACE_ID::uint8* _InternalSerialize(
      ::PROTOBUF_NAMESPACE_ID::uint8* target, ::PROTOBUF_NAMESPACE_ID::io::EpsCopyOutputStream* stream) const final;
  int GetCachedSize() const final { return _cached_size_.Get(); }

  private:
  void SharedCtor();
  void SharedDtor();
  void SetCachedSize(int size) const final;
  void InternalSwap(Dummy* other);
  friend class ::PROTOBUF_NAMESPACE_ID::internal::AnyMetadata;
  static ::PROTOBUF_NAMESPACE_ID::StringPiece FullMessageName() {
    return "fraft.Dummy";
  }
  protected:
  explicit Dummy(::PROTOBUF_NAMESPACE_ID::Arena* arena,
                       bool is_message_owned = false);
  private:
  static void ArenaDtor(void* object);
  inline void RegisterArenaDtor(::PROTOBUF_NAMESPACE_ID::Arena* arena);
  public:

  static const ClassData _class_data_;
  const ::PROTOBUF_NAMESPACE_ID::Message::ClassData*GetClassData() const final;

  ::PROTOBUF_NAMESPACE_ID::Metadata GetMetadata() const final;

  // nested types ----------------------------------------------------

  // accessors -------------------------------------------------------

  // @@protoc_insertion_point(class_scope:fraft.Dummy)
 private:
  class _Internal;

  template <typename T> friend class ::PROTOBUF_NAMESPACE_ID::Arena::InternalHelper;
  typedef void InternalArenaConstructable_;
  typedef void DestructorSkippable_;
  mutable ::PROTOBUF_NAMESPACE_ID::internal::CachedSize _cached_size_;
  friend struct ::TableStruct_leader_5felection_2eproto;
};
// -------------------------------------------------------------------

class Notification final :
    public ::PROTOBUF_NAMESPACE_ID::Message /* @@protoc_insertion_point(class_definition:fraft.Notification) */ {
 public:
  inline Notification() : Notification(nullptr) {}
  ~Notification() override;
  explicit constexpr Notification(::PROTOBUF_NAMESPACE_ID::internal::ConstantInitialized);

  Notification(const Notification& from);
  Notification(Notification&& from) noexcept
    : Notification() {
    *this = ::std::move(from);
  }

  inline Notification& operator=(const Notification& from) {
    CopyFrom(from);
    return *this;
  }
  inline Notification& operator=(Notification&& from) noexcept {
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
  static const Notification& default_instance() {
    return *internal_default_instance();
  }
  static inline const Notification* internal_default_instance() {
    return reinterpret_cast<const Notification*>(
               &_Notification_default_instance_);
  }
  static constexpr int kIndexInFileMessages =
    1;

  friend void swap(Notification& a, Notification& b) {
    a.Swap(&b);
  }
  inline void Swap(Notification* other) {
    if (other == this) return;
    if (GetOwningArena() == other->GetOwningArena()) {
      InternalSwap(other);
    } else {
      ::PROTOBUF_NAMESPACE_ID::internal::GenericSwap(this, other);
    }
  }
  void UnsafeArenaSwap(Notification* other) {
    if (other == this) return;
    GOOGLE_DCHECK(GetOwningArena() == other->GetOwningArena());
    InternalSwap(other);
  }

  // implements Message ----------------------------------------------

  inline Notification* New() const final {
    return new Notification();
  }

  Notification* New(::PROTOBUF_NAMESPACE_ID::Arena* arena) const final {
    return CreateMaybeMessage<Notification>(arena);
  }
  using ::PROTOBUF_NAMESPACE_ID::Message::CopyFrom;
  void CopyFrom(const Notification& from);
  using ::PROTOBUF_NAMESPACE_ID::Message::MergeFrom;
  void MergeFrom(const Notification& from);
  private:
  static void MergeImpl(::PROTOBUF_NAMESPACE_ID::Message*to, const ::PROTOBUF_NAMESPACE_ID::Message&from);
  public:
  PROTOBUF_ATTRIBUTE_REINITIALIZES void Clear() final;
  bool IsInitialized() const final;

  size_t ByteSizeLong() const final;
  const char* _InternalParse(const char* ptr, ::PROTOBUF_NAMESPACE_ID::internal::ParseContext* ctx) final;
  ::PROTOBUF_NAMESPACE_ID::uint8* _InternalSerialize(
      ::PROTOBUF_NAMESPACE_ID::uint8* target, ::PROTOBUF_NAMESPACE_ID::io::EpsCopyOutputStream* stream) const final;
  int GetCachedSize() const final { return _cached_size_.Get(); }

  private:
  void SharedCtor();
  void SharedDtor();
  void SetCachedSize(int size) const final;
  void InternalSwap(Notification* other);
  friend class ::PROTOBUF_NAMESPACE_ID::internal::AnyMetadata;
  static ::PROTOBUF_NAMESPACE_ID::StringPiece FullMessageName() {
    return "fraft.Notification";
  }
  protected:
  explicit Notification(::PROTOBUF_NAMESPACE_ID::Arena* arena,
                       bool is_message_owned = false);
  private:
  static void ArenaDtor(void* object);
  inline void RegisterArenaDtor(::PROTOBUF_NAMESPACE_ID::Arena* arena);
  public:

  static const ClassData _class_data_;
  const ::PROTOBUF_NAMESPACE_ID::Message::ClassData*GetClassData() const final;

  ::PROTOBUF_NAMESPACE_ID::Metadata GetMetadata() const final;

  // nested types ----------------------------------------------------

  // accessors -------------------------------------------------------

  enum : int {
    kLeaderFieldNumber = 1,
    kZxidFieldNumber = 2,
    kEpochFieldNumber = 3,
    kServerIdFieldNumber = 5,
    kStateFieldNumber = 4,
  };
  // uint64 leader = 1;
  void clear_leader();
  ::PROTOBUF_NAMESPACE_ID::uint64 leader() const;
  void set_leader(::PROTOBUF_NAMESPACE_ID::uint64 value);
  private:
  ::PROTOBUF_NAMESPACE_ID::uint64 _internal_leader() const;
  void _internal_set_leader(::PROTOBUF_NAMESPACE_ID::uint64 value);
  public:

  // uint64 zxid = 2;
  void clear_zxid();
  ::PROTOBUF_NAMESPACE_ID::uint64 zxid() const;
  void set_zxid(::PROTOBUF_NAMESPACE_ID::uint64 value);
  private:
  ::PROTOBUF_NAMESPACE_ID::uint64 _internal_zxid() const;
  void _internal_set_zxid(::PROTOBUF_NAMESPACE_ID::uint64 value);
  public:

  // uint64 epoch = 3;
  void clear_epoch();
  ::PROTOBUF_NAMESPACE_ID::uint64 epoch() const;
  void set_epoch(::PROTOBUF_NAMESPACE_ID::uint64 value);
  private:
  ::PROTOBUF_NAMESPACE_ID::uint64 _internal_epoch() const;
  void _internal_set_epoch(::PROTOBUF_NAMESPACE_ID::uint64 value);
  public:

  // uint64 server_id = 5;
  void clear_server_id();
  ::PROTOBUF_NAMESPACE_ID::uint64 server_id() const;
  void set_server_id(::PROTOBUF_NAMESPACE_ID::uint64 value);
  private:
  ::PROTOBUF_NAMESPACE_ID::uint64 _internal_server_id() const;
  void _internal_set_server_id(::PROTOBUF_NAMESPACE_ID::uint64 value);
  public:

  // uint32 state = 4;
  void clear_state();
  ::PROTOBUF_NAMESPACE_ID::uint32 state() const;
  void set_state(::PROTOBUF_NAMESPACE_ID::uint32 value);
  private:
  ::PROTOBUF_NAMESPACE_ID::uint32 _internal_state() const;
  void _internal_set_state(::PROTOBUF_NAMESPACE_ID::uint32 value);
  public:

  // @@protoc_insertion_point(class_scope:fraft.Notification)
 private:
  class _Internal;

  template <typename T> friend class ::PROTOBUF_NAMESPACE_ID::Arena::InternalHelper;
  typedef void InternalArenaConstructable_;
  typedef void DestructorSkippable_;
  ::PROTOBUF_NAMESPACE_ID::uint64 leader_;
  ::PROTOBUF_NAMESPACE_ID::uint64 zxid_;
  ::PROTOBUF_NAMESPACE_ID::uint64 epoch_;
  ::PROTOBUF_NAMESPACE_ID::uint64 server_id_;
  ::PROTOBUF_NAMESPACE_ID::uint32 state_;
  mutable ::PROTOBUF_NAMESPACE_ID::internal::CachedSize _cached_size_;
  friend struct ::TableStruct_leader_5felection_2eproto;
};
// ===================================================================

class LeaderElection_Stub;

class LeaderElection : public ::PROTOBUF_NAMESPACE_ID::Service {
 protected:
  // This class should be treated as an abstract interface.
  inline LeaderElection() {};
 public:
  virtual ~LeaderElection();

  typedef LeaderElection_Stub Stub;

  static const ::PROTOBUF_NAMESPACE_ID::ServiceDescriptor* descriptor();

  virtual void LeaderProposal(::PROTOBUF_NAMESPACE_ID::RpcController* controller,
                       const ::fraft::Notification* request,
                       ::fraft::Dummy* response,
                       ::google::protobuf::Closure* done);

  // implements Service ----------------------------------------------

  const ::PROTOBUF_NAMESPACE_ID::ServiceDescriptor* GetDescriptor();
  void CallMethod(const ::PROTOBUF_NAMESPACE_ID::MethodDescriptor* method,
                  ::PROTOBUF_NAMESPACE_ID::RpcController* controller,
                  const ::PROTOBUF_NAMESPACE_ID::Message* request,
                  ::PROTOBUF_NAMESPACE_ID::Message* response,
                  ::google::protobuf::Closure* done);
  const ::PROTOBUF_NAMESPACE_ID::Message& GetRequestPrototype(
    const ::PROTOBUF_NAMESPACE_ID::MethodDescriptor* method) const;
  const ::PROTOBUF_NAMESPACE_ID::Message& GetResponsePrototype(
    const ::PROTOBUF_NAMESPACE_ID::MethodDescriptor* method) const;

 private:
  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(LeaderElection);
};

class LeaderElection_Stub : public LeaderElection {
 public:
  LeaderElection_Stub(::PROTOBUF_NAMESPACE_ID::RpcChannel* channel);
  LeaderElection_Stub(::PROTOBUF_NAMESPACE_ID::RpcChannel* channel,
                   ::PROTOBUF_NAMESPACE_ID::Service::ChannelOwnership ownership);
  ~LeaderElection_Stub();

  inline ::PROTOBUF_NAMESPACE_ID::RpcChannel* channel() { return channel_; }

  // implements LeaderElection ------------------------------------------

  void LeaderProposal(::PROTOBUF_NAMESPACE_ID::RpcController* controller,
                       const ::fraft::Notification* request,
                       ::fraft::Dummy* response,
                       ::google::protobuf::Closure* done);
 private:
  ::PROTOBUF_NAMESPACE_ID::RpcChannel* channel_;
  bool owns_channel_;
  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(LeaderElection_Stub);
};


// ===================================================================


// ===================================================================

#ifdef __GNUC__
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif  // __GNUC__
// Dummy

// -------------------------------------------------------------------

// Notification

// uint64 leader = 1;
inline void Notification::clear_leader() {
  leader_ = uint64_t{0u};
}
inline ::PROTOBUF_NAMESPACE_ID::uint64 Notification::_internal_leader() const {
  return leader_;
}
inline ::PROTOBUF_NAMESPACE_ID::uint64 Notification::leader() const {
  // @@protoc_insertion_point(field_get:fraft.Notification.leader)
  return _internal_leader();
}
inline void Notification::_internal_set_leader(::PROTOBUF_NAMESPACE_ID::uint64 value) {
  
  leader_ = value;
}
inline void Notification::set_leader(::PROTOBUF_NAMESPACE_ID::uint64 value) {
  _internal_set_leader(value);
  // @@protoc_insertion_point(field_set:fraft.Notification.leader)
}

// uint64 zxid = 2;
inline void Notification::clear_zxid() {
  zxid_ = uint64_t{0u};
}
inline ::PROTOBUF_NAMESPACE_ID::uint64 Notification::_internal_zxid() const {
  return zxid_;
}
inline ::PROTOBUF_NAMESPACE_ID::uint64 Notification::zxid() const {
  // @@protoc_insertion_point(field_get:fraft.Notification.zxid)
  return _internal_zxid();
}
inline void Notification::_internal_set_zxid(::PROTOBUF_NAMESPACE_ID::uint64 value) {
  
  zxid_ = value;
}
inline void Notification::set_zxid(::PROTOBUF_NAMESPACE_ID::uint64 value) {
  _internal_set_zxid(value);
  // @@protoc_insertion_point(field_set:fraft.Notification.zxid)
}

// uint64 epoch = 3;
inline void Notification::clear_epoch() {
  epoch_ = uint64_t{0u};
}
inline ::PROTOBUF_NAMESPACE_ID::uint64 Notification::_internal_epoch() const {
  return epoch_;
}
inline ::PROTOBUF_NAMESPACE_ID::uint64 Notification::epoch() const {
  // @@protoc_insertion_point(field_get:fraft.Notification.epoch)
  return _internal_epoch();
}
inline void Notification::_internal_set_epoch(::PROTOBUF_NAMESPACE_ID::uint64 value) {
  
  epoch_ = value;
}
inline void Notification::set_epoch(::PROTOBUF_NAMESPACE_ID::uint64 value) {
  _internal_set_epoch(value);
  // @@protoc_insertion_point(field_set:fraft.Notification.epoch)
}

// uint32 state = 4;
inline void Notification::clear_state() {
  state_ = 0u;
}
inline ::PROTOBUF_NAMESPACE_ID::uint32 Notification::_internal_state() const {
  return state_;
}
inline ::PROTOBUF_NAMESPACE_ID::uint32 Notification::state() const {
  // @@protoc_insertion_point(field_get:fraft.Notification.state)
  return _internal_state();
}
inline void Notification::_internal_set_state(::PROTOBUF_NAMESPACE_ID::uint32 value) {
  
  state_ = value;
}
inline void Notification::set_state(::PROTOBUF_NAMESPACE_ID::uint32 value) {
  _internal_set_state(value);
  // @@protoc_insertion_point(field_set:fraft.Notification.state)
}

// uint64 server_id = 5;
inline void Notification::clear_server_id() {
  server_id_ = uint64_t{0u};
}
inline ::PROTOBUF_NAMESPACE_ID::uint64 Notification::_internal_server_id() const {
  return server_id_;
}
inline ::PROTOBUF_NAMESPACE_ID::uint64 Notification::server_id() const {
  // @@protoc_insertion_point(field_get:fraft.Notification.server_id)
  return _internal_server_id();
}
inline void Notification::_internal_set_server_id(::PROTOBUF_NAMESPACE_ID::uint64 value) {
  
  server_id_ = value;
}
inline void Notification::set_server_id(::PROTOBUF_NAMESPACE_ID::uint64 value) {
  _internal_set_server_id(value);
  // @@protoc_insertion_point(field_set:fraft.Notification.server_id)
}

#ifdef __GNUC__
  #pragma GCC diagnostic pop
#endif  // __GNUC__
// -------------------------------------------------------------------


// @@protoc_insertion_point(namespace_scope)

}  // namespace fraft

// @@protoc_insertion_point(global_scope)

#include <google/protobuf/port_undef.inc>
#endif  // GOOGLE_PROTOBUF_INCLUDED_GOOGLE_PROTOBUF_INCLUDED_leader_5felection_2eproto
