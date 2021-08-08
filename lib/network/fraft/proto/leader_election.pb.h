// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: leader_election.proto

#ifndef PROTOBUF_INCLUDED_leader_5felection_2eproto
#define PROTOBUF_INCLUDED_leader_5felection_2eproto

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
#include <google/protobuf/service.h>
#include <google/protobuf/unknown_field_set.h>
// @@protoc_insertion_point(includes)
#define PROTOBUF_INTERNAL_EXPORT_protobuf_leader_5felection_2eproto 

namespace protobuf_leader_5felection_2eproto {
// Internal implementation detail -- do not use these members.
struct TableStruct {
  static const ::google::protobuf::internal::ParseTableField entries[];
  static const ::google::protobuf::internal::AuxillaryParseTableField aux[];
  static const ::google::protobuf::internal::ParseTable schema[2];
  static const ::google::protobuf::internal::FieldMetadata field_metadata[];
  static const ::google::protobuf::internal::SerializationTable serialization_table[];
  static const ::google::protobuf::uint32 offsets[];
};
void AddDescriptors();
}  // namespace protobuf_leader_5felection_2eproto
namespace fraft {
class Dummy;
class DummyDefaultTypeInternal;
extern DummyDefaultTypeInternal _Dummy_default_instance_;
class Notification;
class NotificationDefaultTypeInternal;
extern NotificationDefaultTypeInternal _Notification_default_instance_;
}  // namespace fraft
namespace google {
namespace protobuf {
template<> ::fraft::Dummy* Arena::CreateMaybeMessage<::fraft::Dummy>(Arena*);
template<> ::fraft::Notification* Arena::CreateMaybeMessage<::fraft::Notification>(Arena*);
}  // namespace protobuf
}  // namespace google
namespace fraft {

// ===================================================================

class Dummy : public ::google::protobuf::Message /* @@protoc_insertion_point(class_definition:fraft.Dummy) */ {
 public:
  Dummy();
  virtual ~Dummy();

  Dummy(const Dummy& from);

  inline Dummy& operator=(const Dummy& from) {
    CopyFrom(from);
    return *this;
  }
  #if LANG_CXX11
  Dummy(Dummy&& from) noexcept
    : Dummy() {
    *this = ::std::move(from);
  }

  inline Dummy& operator=(Dummy&& from) noexcept {
    if (GetArenaNoVirtual() == from.GetArenaNoVirtual()) {
      if (this != &from) InternalSwap(&from);
    } else {
      CopyFrom(from);
    }
    return *this;
  }
  #endif
  static const ::google::protobuf::Descriptor* descriptor();
  static const Dummy& default_instance();

  static void InitAsDefaultInstance();  // FOR INTERNAL USE ONLY
  static inline const Dummy* internal_default_instance() {
    return reinterpret_cast<const Dummy*>(
               &_Dummy_default_instance_);
  }
  static constexpr int kIndexInFileMessages =
    0;

  void Swap(Dummy* other);
  friend void swap(Dummy& a, Dummy& b) {
    a.Swap(&b);
  }

  // implements Message ----------------------------------------------

  inline Dummy* New() const final {
    return CreateMaybeMessage<Dummy>(NULL);
  }

  Dummy* New(::google::protobuf::Arena* arena) const final {
    return CreateMaybeMessage<Dummy>(arena);
  }
  void CopyFrom(const ::google::protobuf::Message& from) final;
  void MergeFrom(const ::google::protobuf::Message& from) final;
  void CopyFrom(const Dummy& from);
  void MergeFrom(const Dummy& from);
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
  void InternalSwap(Dummy* other);
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

  // @@protoc_insertion_point(class_scope:fraft.Dummy)
 private:

  ::google::protobuf::internal::InternalMetadataWithArena _internal_metadata_;
  mutable ::google::protobuf::internal::CachedSize _cached_size_;
  friend struct ::protobuf_leader_5felection_2eproto::TableStruct;
};
// -------------------------------------------------------------------

class Notification : public ::google::protobuf::Message /* @@protoc_insertion_point(class_definition:fraft.Notification) */ {
 public:
  Notification();
  virtual ~Notification();

  Notification(const Notification& from);

  inline Notification& operator=(const Notification& from) {
    CopyFrom(from);
    return *this;
  }
  #if LANG_CXX11
  Notification(Notification&& from) noexcept
    : Notification() {
    *this = ::std::move(from);
  }

  inline Notification& operator=(Notification&& from) noexcept {
    if (GetArenaNoVirtual() == from.GetArenaNoVirtual()) {
      if (this != &from) InternalSwap(&from);
    } else {
      CopyFrom(from);
    }
    return *this;
  }
  #endif
  static const ::google::protobuf::Descriptor* descriptor();
  static const Notification& default_instance();

  static void InitAsDefaultInstance();  // FOR INTERNAL USE ONLY
  static inline const Notification* internal_default_instance() {
    return reinterpret_cast<const Notification*>(
               &_Notification_default_instance_);
  }
  static constexpr int kIndexInFileMessages =
    1;

  void Swap(Notification* other);
  friend void swap(Notification& a, Notification& b) {
    a.Swap(&b);
  }

  // implements Message ----------------------------------------------

  inline Notification* New() const final {
    return CreateMaybeMessage<Notification>(NULL);
  }

  Notification* New(::google::protobuf::Arena* arena) const final {
    return CreateMaybeMessage<Notification>(arena);
  }
  void CopyFrom(const ::google::protobuf::Message& from) final;
  void MergeFrom(const ::google::protobuf::Message& from) final;
  void CopyFrom(const Notification& from);
  void MergeFrom(const Notification& from);
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
  void InternalSwap(Notification* other);
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

  // uint64 leader = 1;
  void clear_leader();
  static const int kLeaderFieldNumber = 1;
  ::google::protobuf::uint64 leader() const;
  void set_leader(::google::protobuf::uint64 value);

  // uint64 zxid = 2;
  void clear_zxid();
  static const int kZxidFieldNumber = 2;
  ::google::protobuf::uint64 zxid() const;
  void set_zxid(::google::protobuf::uint64 value);

  // uint64 epoch = 3;
  void clear_epoch();
  static const int kEpochFieldNumber = 3;
  ::google::protobuf::uint64 epoch() const;
  void set_epoch(::google::protobuf::uint64 value);

  // uint64 server_id = 5;
  void clear_server_id();
  static const int kServerIdFieldNumber = 5;
  ::google::protobuf::uint64 server_id() const;
  void set_server_id(::google::protobuf::uint64 value);

  // uint32 state = 4;
  void clear_state();
  static const int kStateFieldNumber = 4;
  ::google::protobuf::uint32 state() const;
  void set_state(::google::protobuf::uint32 value);

  // @@protoc_insertion_point(class_scope:fraft.Notification)
 private:

  ::google::protobuf::internal::InternalMetadataWithArena _internal_metadata_;
  ::google::protobuf::uint64 leader_;
  ::google::protobuf::uint64 zxid_;
  ::google::protobuf::uint64 epoch_;
  ::google::protobuf::uint64 server_id_;
  ::google::protobuf::uint32 state_;
  mutable ::google::protobuf::internal::CachedSize _cached_size_;
  friend struct ::protobuf_leader_5felection_2eproto::TableStruct;
};
// ===================================================================

class LeaderElection_Stub;

class LeaderElection : public ::google::protobuf::Service {
 protected:
  // This class should be treated as an abstract interface.
  inline LeaderElection() {};
 public:
  virtual ~LeaderElection();

  typedef LeaderElection_Stub Stub;

  static const ::google::protobuf::ServiceDescriptor* descriptor();

  virtual void LeaderProposal(::google::protobuf::RpcController* controller,
                       const ::fraft::Notification* request,
                       ::fraft::Dummy* response,
                       ::google::protobuf::Closure* done);

  // implements Service ----------------------------------------------

  const ::google::protobuf::ServiceDescriptor* GetDescriptor();
  void CallMethod(const ::google::protobuf::MethodDescriptor* method,
                  ::google::protobuf::RpcController* controller,
                  const ::google::protobuf::Message* request,
                  ::google::protobuf::Message* response,
                  ::google::protobuf::Closure* done);
  const ::google::protobuf::Message& GetRequestPrototype(
    const ::google::protobuf::MethodDescriptor* method) const;
  const ::google::protobuf::Message& GetResponsePrototype(
    const ::google::protobuf::MethodDescriptor* method) const;

 private:
  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(LeaderElection);
};

class LeaderElection_Stub : public LeaderElection {
 public:
  LeaderElection_Stub(::google::protobuf::RpcChannel* channel);
  LeaderElection_Stub(::google::protobuf::RpcChannel* channel,
                   ::google::protobuf::Service::ChannelOwnership ownership);
  ~LeaderElection_Stub();

  inline ::google::protobuf::RpcChannel* channel() { return channel_; }

  // implements LeaderElection ------------------------------------------

  void LeaderProposal(::google::protobuf::RpcController* controller,
                       const ::fraft::Notification* request,
                       ::fraft::Dummy* response,
                       ::google::protobuf::Closure* done);
 private:
  ::google::protobuf::RpcChannel* channel_;
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
  leader_ = GOOGLE_ULONGLONG(0);
}
inline ::google::protobuf::uint64 Notification::leader() const {
  // @@protoc_insertion_point(field_get:fraft.Notification.leader)
  return leader_;
}
inline void Notification::set_leader(::google::protobuf::uint64 value) {
  
  leader_ = value;
  // @@protoc_insertion_point(field_set:fraft.Notification.leader)
}

// uint64 zxid = 2;
inline void Notification::clear_zxid() {
  zxid_ = GOOGLE_ULONGLONG(0);
}
inline ::google::protobuf::uint64 Notification::zxid() const {
  // @@protoc_insertion_point(field_get:fraft.Notification.zxid)
  return zxid_;
}
inline void Notification::set_zxid(::google::protobuf::uint64 value) {
  
  zxid_ = value;
  // @@protoc_insertion_point(field_set:fraft.Notification.zxid)
}

// uint64 epoch = 3;
inline void Notification::clear_epoch() {
  epoch_ = GOOGLE_ULONGLONG(0);
}
inline ::google::protobuf::uint64 Notification::epoch() const {
  // @@protoc_insertion_point(field_get:fraft.Notification.epoch)
  return epoch_;
}
inline void Notification::set_epoch(::google::protobuf::uint64 value) {
  
  epoch_ = value;
  // @@protoc_insertion_point(field_set:fraft.Notification.epoch)
}

// uint32 state = 4;
inline void Notification::clear_state() {
  state_ = 0u;
}
inline ::google::protobuf::uint32 Notification::state() const {
  // @@protoc_insertion_point(field_get:fraft.Notification.state)
  return state_;
}
inline void Notification::set_state(::google::protobuf::uint32 value) {
  
  state_ = value;
  // @@protoc_insertion_point(field_set:fraft.Notification.state)
}

// uint64 server_id = 5;
inline void Notification::clear_server_id() {
  server_id_ = GOOGLE_ULONGLONG(0);
}
inline ::google::protobuf::uint64 Notification::server_id() const {
  // @@protoc_insertion_point(field_get:fraft.Notification.server_id)
  return server_id_;
}
inline void Notification::set_server_id(::google::protobuf::uint64 value) {
  
  server_id_ = value;
  // @@protoc_insertion_point(field_set:fraft.Notification.server_id)
}

#ifdef __GNUC__
  #pragma GCC diagnostic pop
#endif  // __GNUC__
// -------------------------------------------------------------------


// @@protoc_insertion_point(namespace_scope)

}  // namespace fraft

// @@protoc_insertion_point(global_scope)

#endif  // PROTOBUF_INCLUDED_leader_5felection_2eproto
