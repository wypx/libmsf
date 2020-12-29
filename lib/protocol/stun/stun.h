#ifndef LIB_STUN_H_
#define LIB_STUN_H_

// This file contains classes for dealing with the STUN protocol, as specified
// in RFC 5389, and its descendants.

#include <stddef.h>
#include <stdint.h>

#include <memory>
#include <string>
#include <vector>

#include "byte_order.h"
#include "byte_buffer.h"
#include "ip_address.h"
#include "socket_address.h"

namespace MSF {

// These are the types of STUN messages defined in RFC 5389.
enum StunMessageType {
  kStunMessageBindingRequest = 0x0001,
  kStunMessageBindingIndication = 0x0011,
  kStunMessageBindingResponse = 0x0101,
  kStunMessageBindingErrorResponse = 0x0111,

  // Method 0x80, GOOG-PING is a variant of STUN BINDING
  // that is sent instead of a STUN BINDING if the binding
  // was identical to the one before.
  kStunGoodPingRequest = 0x200,
  kStunGoodPingResponse = 0x300,
  kStunGoodPingErrorResponse = 0x310,

  // Defined in TURN RFC 5766.
  kStunAllocateRequst = 0x0003,
  kStunAllocateResponse = 0x0103,
  kStunAllocateErrorResponse = 0x0113,
  kTurnRefreshRequest = 0x0004,
  kTurnRefreshResponse = 0x0104,
  kTurnRefreshErrorResponse = 0x0114,
  kTurnSendSendIndication = 0x0016,
  kTurnDataSendIndication = 0x0017,
  kTurnCreatePermissionRequest = 0x0008,
  kTurnCreatePermissionResonse = 0x0108,
  kTurnCreatePermissionErrorResonse = 0x0118,
  kTurnChannelBindRequest = 0x0009,
  kTurnChannelBindResponse = 0x0109,
  kTurnChannelBindErrorResponse = 0x0119,

  /// TURN TCP RFC 6062
  kTurnConnect = 0x000a,
  kTurnConnectionBind = 0x000b,
  kTurnConnectionAttempt = 0x000c
};

// These are all known STUN attributes, defined in RFC 5389 and elsewhere.
// Next to each is the name of the class (T is StunTAttribute) that implements
// that type.
// RETRANSMIT_COUNT is the number of outstanding pings without a response at
// the time the packet is generated.
enum StunAttributeType {
  kStunAttrNotExist = 0,
  kStunAttrMappedAddress = 0x0001,
  kStunAttrResponseAddress = 0x0002,  // Not implemented
  kStunAttrChangeRequest = 0x0003,    // Not implemented
  kStunAttrSourceAddress = 0x0004,    // Not implemented
  kStunAttrChangedAddress = 0x0005,   // Not implemented
  kStunAttrUsername = 0x0006,
  kStunAttrPassword = 0x0007,          // Not implemented
  kStunAttrMessageIntegrity = 0x0008,  // ByteString, 20 bytes
  kStunAttrStunErrorCode = 0x0009,
  kStunAttrBandwidth = 0x0010,           // GTURN: 4 bytes
  kStunAttrDestinationAddress = 0x0011,  // GTURN:Address
  kStunAttrSourceAddress2 = 0x0012,      // GTURN:Address
  kStunAttrRealm = 0x0014,               // ByteString
  kStunAttrNonce = 0x0015,               // ByteString
  kStunAttrUnknownAttributes = 0x000a,   // UInt16List
  kStunAttrReflectedFrom = 0x000b,       // Not implemented
  // kStunAttrTransportPreferences = 0x000c, // Not implemented

  // Turn attributes
  // "GTURN"-specific STUN attributes.
  // TODO(?): Rename these attributes to GTURN_ to avoid conflicts.
  kStunAttrChannelNumber = 0x000c,  // Turn, GTURN: UInt32
  kStunAttrLifetime = 0x000d,  // Turn: UInt32 0x0010: Reserved (was BANDWIDTH)
  kStunAttrAlternateServer = 0x000e,
  kStunAttrMagicCookie = 0x000f,  // GTURN: ByteString, 4 bytes
  kStunAttrXorPeerAddress = 0x0012,
  // TODO(mallinath) - Uncomment after RelayAttributes are renamed.
  kStunAttrData = 0x0013,  // GTURN: ByteString
  kStunAttrXorRelayedAddress = 0x0016,
  kStunAttrEventPort = 0x0018,           // ByteString, 1 byte.
  kStunAttrRequestedTransport = 0x0019,  // UInt32
  kStunAttrDontFragment = 0x001A,        // No content, Length = 0
  kStunAttrXorMappedAddress = 0x0020,
  /// 0x0021: Reserved (was TIMER-VAL)
  kStunAttrReservationToken = 0x0022,  // ByteString, 8 bytes token value
  kStunAttrOptions = 0x8001,           // GTURN: UInt32
  kStunAttrSoftware = 0x8022,
  SkStunAttrAlternateServer = 0x8023,  // Address
  kStunAttrFingerprint = 0x8028,       // TURN UInt32
  kStunAttrOrigin = 0x802F,            // ByteString
  // TODO(mallinath) - Rename STUN_ATTR_TURN_LIFETIME to STUN_ATTR_LIFETIME and
  // STUN_ATTR_TURN_DATA to STUN_ATTR_DATA. Also rename RelayMessage attributes
  // by appending G to attribute name.

  // TURN TCP
  kStunAttrConnectionID = 0x002a,
  kStunAttrRetransmitCount = 0xFF00,  // UInt32

  // RFC 5245 ICE STUN attributes.
  kStunAttrICEControlled = 0x8029,    // UInt64
  kStunAttrICEControlling = 0x802A,   // UInt64
  kStunAttrICEPriority = 0x0024,      // UInt32
  kStunAttrICEUseCandidate = 0x0025,  // No content, Length = 0
  // UInt32. The higher 16 bits are the network ID. The lower 16 bits are the
  // network cost.
  kStunAttrICENomination = 0xC001,  // UInt32
  // The following attributes are in the comprehension-optional range
  // (0xC000-0xFFFF) and are not registered with IANA. These STUN attributes are
  // intended for ICE and should NOT be used in generic use cases of STUN
  // messages.
  //
  // Note that the value 0xC001 has already been assigned by IANA to
  // ENF-FLOW-DESCRIPTION
  // (https://www.iana.org/assignments/stun-parameters/stun-parameters.xml).
  // UInt32. The higher 16 bits are the network ID. The lower 16 bits are the
  // network cost.
  kStunAttrICENetworkInfo = 0xC057,
  // Experimental: Transaction ID of the last connectivity check received.
  kStunAttrLastIceCheckReceived = 0xC058,
  // Uint16List. Miscellaneous attributes for future extension
  kStunAttrGoodMiscInfo = 0xC059,
  // MESSAGE-INTEGRITY truncated to 32-bit.
  kStunAttrGoodMessageInteger32 = 0xC060,
};

// These are the types of the values associated with the attributes above.
// This allows us to perform some basic validation when reading or adding
// attributes. Note that these values are for our own use, and not defined in
// RFC 5389.
enum StunAttributeValueType {
  kStunAttrValueUnknown = 0,
  kStunAttrValueAddress = 1,
  kStunAttrValueXorAddress = 2,
  kStunAttrValueUInt32 = 3,
  kStunAttrValueUInt64 = 4,
  kStunAttrValueByteString = 5,
  kStunAttrValueErrorCode = 6,
  kStunAttrValueUInt16List = 7
};

// These are the types of STUN addresses defined in RFC 5389.
enum StunAddressFamily {
  // NB: UNDEF is not part of the STUN spec.
  kStunAddressUndef = 0,
  kStunAddressIpv4 = 1,
  kStunAddressIpv6 = 2
};

// These are the types of STUN error codes defined in RFC 5389.
enum StunErrorCode {
  kStunTryAlteate = 300,
  kStunBadRequest = 400,
  kStunNotAuthorized = 401,
  kStunForbidden = 403,  // Turn: RFC 5766-defined errors.
  kStunUnknownAttribute = 420,
  kStunAllocationMismatch = 437,  // Turn: RFC 5766-defined errors.
  kStunStaleCredentials = 430,    // GICE only
  kStunStaleNonce = 438,
  kStunAddrFamilyNotSupport = 4,
  kStunWrongCredentials = 442,      // Turn: RFC 5766-defined errors.
  kStunUnsuppTransportProto = 442,  // Turn: RFC 5766-defined errors.
  kStunPeerAddrFamilyMismatch = 7,
  kStunAllocationQuotaReached = 10,
  kStunIntegrityCheckFailure = 431,
  kStunMissingUsername = 432,
  kStunUseTLS = 433,
  kStunRoleConflict = 487,  // ICE: RFC 5245-defined errors
  kStunServerError = 500,
  kStunInsufficientCapacity = 11,
  kStunGlobalFailure = 600,

  /// TURN TCP
  kStunConnectionFailure = 9,
  kStunConnectionAlreadyExists = 446,
  kStunConnectionTimeoutOrFailure = 447
};

// The mask used to determine whether a STUN message is a request/response etc.
const uint32_t kStunTypeMask = 0x0110;

// Following values correspond to RFC5389.
const int kStunAttributeHeaderSize = 4;
const int kStunHeaderSize = 20;
const int kStunTransactionIdOffset = 8;
const int kStunTransactionIdLength = 12;
const uint32_t kStunMagicCookie = 0x2112A442;
const int kStunMagicCookieLength = sizeof(kStunMagicCookie);

// Following value corresponds to an earlier version of STUN from
// RFC3489.
const int kStunLegacyTransactionIdLength = 16;

// STUN Message Integrity HMAC length.
const int kStunMessageIntegritySize = 20;

// Size of STUN_ATTR_MESSAGE_INTEGRITY_32
const size_t kStunMessageIntegrity32Size = 4;

class StunAddressAttribute;
class StunAttribute;
class StunByteStringAttribute;
class StunErrorCodeAttribute;

class StunUInt16ListAttribute;
class StunUInt32Attribute;
class StunUInt64Attribute;
class StunXorAddressAttribute;

// Records a complete STUN/TURN message.  Each message consists of a type and
// any number of attributes.  Each attribute is parsed into an instance of an
// appropriate class (see above).  The Get* methods will return instances of
// that attribute class
class StunMessage {
 public:
  StunMessage();
  virtual ~StunMessage();

  std::string MethodString() const;
  std::string ClassString() const;
  std::string ErrorString(uint16_t code) const;
  std::string ToString() const;

  int type() const { return type_; }
  size_t length() const { return length_; }
  const std::string& transaction_id() const { return transaction_id_; }
  uint32_t reduced_transaction_id() const { return reduced_transaction_id_; }

  // Returns true if the message confirms to RFC3489 rather than
  // RFC5389. The main difference between two version of the STUN
  // protocol is the presence of the magic cookie and different length
  // of transaction ID. For outgoing packets version of the protocol
  // is determined by the lengths of the transaction ID.
  bool IsLegacy() const;

  void SetType(int type) { type_ = static_cast<uint16_t>(type); }
  bool SetTransactionID(const std::string& str);

  // Gets the desired attribute value, or NULL if no such attribute type exists.
  const StunAddressAttribute* GetAddress(int type) const;
  const StunUInt32Attribute* GetUInt32(int type) const;
  const StunUInt64Attribute* GetUInt64(int type) const;
  const StunByteStringAttribute* GetByteString(int type) const;
  const StunUInt16ListAttribute* GetUInt16List(int type) const;

  // Gets these specific attribute values.
  const StunErrorCodeAttribute* GetErrorCode() const;
  // Returns the code inside the error code attribute, if present, and
  // STUN_ERROR_GLOBAL_FAILURE otherwise.
  int GetErrorCodeValue() const;
  const StunUInt16ListAttribute* GetUnknownAttributes() const;

  // Takes ownership of the specified attribute and adds it to the message.
  void AddAttribute(std::unique_ptr<StunAttribute> attr);

  // Remove the last occurrence of an attribute.
  std::unique_ptr<StunAttribute> RemoveAttribute(int type);

  // Remote all attributes and releases them.
  void ClearAttributes();

  // Validates that a raw STUN message has a correct MESSAGE-INTEGRITY value.
  // This can't currently be done on a StunMessage, since it is affected by
  // padding data (which we discard when reading a StunMessage).
  static bool ValidateMessageIntegrity(const char* data, size_t size,
                                       const std::string& password);
  static bool ValidateMessageIntegrity32(const char* data, size_t size,
                                         const std::string& password);

  // Adds a MESSAGE-INTEGRITY attribute that is valid for the current message.
  bool AddMessageIntegrity(const std::string& password);
  bool AddMessageIntegrity(const char* key, size_t keylen);

  // Adds a STUN_ATTR_GOOG_MESSAGE_INTEGRITY_32 attribute that is valid for the
  // current message.
  bool AddMessageIntegrity32(std::string_view password);

  // Verify that a buffer has stun magic cookie and one of the specified
  // methods. Note that it does not check for the existance of FINGERPRINT.
  static bool IsStunMethod(ArrayView<int> methods, const char* data,
                           size_t size);

  // Verifies that a given buffer is STUN by checking for a correct FINGERPRINT.
  static bool ValidateFingerprint(const char* data, size_t size);

  // Adds a FINGERPRINT attribute that is valid for the current message.
  bool AddFingerprint();

  // Parses the STUN packet in the given buffer and records it here. The
  // return value indicates whether this was successful.
  bool Read(ByteBufferReader* buf);

  // Writes this object into a STUN packet. The return value indicates whether
  // this was successful.
  bool Write(ByteBufferWriter* buf) const;

  // Creates an empty message. Overridable by derived classes.
  virtual StunMessage* CreateNew() const;

  // Modify the stun magic cookie used for this STUN message.
  // This is used for testing.
  void SetStunMagicCookie(uint32_t val);

  // Contruct a copy of |this|.
  std::unique_ptr<StunMessage> Clone() const;

  // Check if the attributes of this StunMessage equals those of |other|
  // for all attributes that |attribute_type_mask| return true
  bool EqualAttributes(const StunMessage* other,
                       std::function<bool(int type)> attribute_type_mask) const;

 protected:
  // Verifies that the given attribute is allowed for this message.
  virtual StunAttributeValueType GetAttributeValueType(int type) const;

  std::vector<std::unique_ptr<StunAttribute>> attrs_;

 private:
  StunAttribute* CreateAttribute(int type, size_t length) /* const*/;
  const StunAttribute* GetAttribute(int type) const;
  static bool IsValidTransactionId(const std::string& transaction_id);
  bool AddMessageIntegrityOfType(int mi_attr_type, size_t mi_attr_size,
                                 const char* key, size_t keylen);
  static bool ValidateMessageIntegrityOfType(int mi_attr_type,
                                             size_t mi_attr_size,
                                             const char* data, size_t size,
                                             const std::string& password);

  uint16_t type_;
  uint16_t length_;
  std::string transaction_id_;
  uint32_t reduced_transaction_id_;
  uint32_t stun_magic_cookie_;
};

// Base class for all STUN/TURN attributes.
class StunAttribute {
 public:
  virtual ~StunAttribute() {}

  int type() const { return type_; }
  size_t length() const { return length_; }

  // Return the type of this attribute.
  virtual StunAttributeValueType value_type() const = 0;

  // Only XorAddressAttribute needs this so far.
  virtual void SetOwner(StunMessage* owner) {}

  // Reads the body (not the type or length) for this type of attribute from
  // the given buffer.  Return value is true if successful.
  virtual bool Read(ByteBufferReader* buf) = 0;

  // Writes the body (not the type or length) to the given buffer.  Return
  // value is true if successful.
  virtual bool Write(ByteBufferWriter* buf) const = 0;

  // Creates an attribute object with the given type and smallest length.
  static StunAttribute* Create(StunAttributeValueType value_type, uint16_t type,
                               uint16_t length, StunMessage* owner);
  // TODO(?): Allow these create functions to take parameters, to reduce
  // the amount of work callers need to do to initialize attributes.
  static std::unique_ptr<StunAddressAttribute> CreateAddress(uint16_t type);
  static std::unique_ptr<StunXorAddressAttribute> CreateXorAddress(
      uint16_t type);
  static std::unique_ptr<StunUInt32Attribute> CreateUInt32(uint16_t type);
  static std::unique_ptr<StunUInt64Attribute> CreateUInt64(uint16_t type);
  static std::unique_ptr<StunByteStringAttribute> CreateByteString(
      uint16_t type);
  static std::unique_ptr<StunUInt16ListAttribute> CreateUInt16ListAttribute(
      uint16_t type);
  static std::unique_ptr<StunErrorCodeAttribute> CreateErrorCode();
  static std::unique_ptr<StunUInt16ListAttribute> CreateUnknownAttributes();

 protected:
  StunAttribute(uint16_t type, uint16_t length);
  void SetLength(uint16_t length) { length_ = length; }
  void WritePadding(ByteBufferWriter* buf) const;
  void ConsumePadding(ByteBufferReader* buf) const;

 private:
  uint16_t type_;
  uint16_t length_;
};

// Implements STUN attributes that record an Internet address.
class StunAddressAttribute : public StunAttribute {
 public:
  static const uint16_t SIZE_UNDEF = 0;
  static const uint16_t SIZE_IP4 = 8;
  static const uint16_t SIZE_IP6 = 20;
  StunAddressAttribute(uint16_t type, const SocketAddress& addr);
  StunAddressAttribute(uint16_t type, uint16_t length);

  StunAttributeValueType value_type() const override;

  StunAddressFamily family() const {
    switch (address_.ipaddr().family()) {
      case AF_INET:
        return kStunAddressIpv4;
      case AF_INET6:
        return kStunAddressIpv6;
    }
    return kStunAddressUndef;
  }

  const SocketAddress& GetAddress() const { return address_; }
  const IPAddress& ipaddr() const { return address_.ipaddr(); }
  uint16_t port() const { return address_.port(); }

  void SetAddress(const SocketAddress& addr) {
    address_ = addr;
    EnsureAddressLength();
  }
  void SetIP(const IPAddress& ip) {
    address_.SetIP(ip);
    EnsureAddressLength();
  }
  void SetPort(uint16_t port) { address_.SetPort(port); }

  bool Read(ByteBufferReader* buf) override;
  bool Write(ByteBufferWriter* buf) const override;

 private:
  void EnsureAddressLength() {
    switch (family()) {
      case kStunAddressIpv4: {
        SetLength(SIZE_IP4);
        break;
      }
      case kStunAddressIpv6: {
        SetLength(SIZE_IP6);
        break;
      }
      default: {
        SetLength(SIZE_UNDEF);
        break;
      }
    }
  }
  SocketAddress address_;
};

// Implements STUN attributes that record an Internet address. When encoded
// in a STUN message, the address contained in this attribute is XORed with the
// transaction ID of the message.
class StunXorAddressAttribute : public StunAddressAttribute {
 public:
  StunXorAddressAttribute(uint16_t type, const SocketAddress& addr);
  StunXorAddressAttribute(uint16_t type, uint16_t length, StunMessage* owner);

  StunAttributeValueType value_type() const override;
  void SetOwner(StunMessage* owner) override;
  bool Read(ByteBufferReader* buf) override;
  bool Write(ByteBufferWriter* buf) const override;

 private:
  IPAddress GetXoredIP() const;
  StunMessage* owner_;
};

// Implements STUN attributes that record a 32-bit integer.
class StunUInt32Attribute : public StunAttribute {
 public:
  static const uint16_t SIZE = 4;
  StunUInt32Attribute(uint16_t type, uint32_t value);
  explicit StunUInt32Attribute(uint16_t type);

  StunAttributeValueType value_type() const override;

  uint32_t value() const { return bits_; }
  void SetValue(uint32_t bits) { bits_ = bits; }

  bool GetBit(size_t index) const;
  void SetBit(size_t index, bool value);

  bool Read(ByteBufferReader* buf) override;
  bool Write(ByteBufferWriter* buf) const override;

 private:
  uint32_t bits_;
};

class StunUInt64Attribute : public StunAttribute {
 public:
  static const uint16_t SIZE = 8;
  StunUInt64Attribute(uint16_t type, uint64_t value);
  explicit StunUInt64Attribute(uint16_t type);

  StunAttributeValueType value_type() const override;

  uint64_t value() const { return bits_; }
  void SetValue(uint64_t bits) { bits_ = bits; }

  bool Read(ByteBufferReader* buf) override;
  bool Write(ByteBufferWriter* buf) const override;

 private:
  uint64_t bits_;
};

// Implements STUN attributes that record an arbitrary byte string.
class StunByteStringAttribute : public StunAttribute {
 public:
  explicit StunByteStringAttribute(uint16_t type);
  StunByteStringAttribute(uint16_t type, const std::string& str);
  StunByteStringAttribute(uint16_t type, const void* bytes, size_t length);
  StunByteStringAttribute(uint16_t type, uint16_t length);
  ~StunByteStringAttribute() override;

  StunAttributeValueType value_type() const override;

  const char* bytes() const { return bytes_; }
  std::string GetString() const { return std::string(bytes_, length()); }

  void CopyBytes(const char* bytes);  // uses strlen
  void CopyBytes(const void* bytes, size_t length);

  uint8_t GetByte(size_t index) const;
  void SetByte(size_t index, uint8_t value);

  bool Read(ByteBufferReader* buf) override;
  bool Write(ByteBufferWriter* buf) const override;

 private:
  void SetBytes(char* bytes, size_t length);

  char* bytes_;
};

// Implements STUN attributes that record an error code.
class StunErrorCodeAttribute : public StunAttribute {
 public:
  static const uint16_t MIN_SIZE;
  StunErrorCodeAttribute(uint16_t type, int code, const std::string& reason);
  StunErrorCodeAttribute(uint16_t type, uint16_t length);
  ~StunErrorCodeAttribute() override;

  StunAttributeValueType value_type() const override;

  // The combined error and class, e.g. 0x400.
  int code() const;
  void SetCode(int code);

  // The individual error components.
  int eclass() const { return class_; }
  int number() const { return number_; }
  const std::string& reason() const { return reason_; }
  void SetClass(uint8_t eclass) { class_ = eclass; }
  void SetNumber(uint8_t number) { number_ = number; }
  void SetReason(const std::string& reason);

  bool Read(ByteBufferReader* buf) override;
  bool Write(ByteBufferWriter* buf) const override;

 private:
  uint8_t class_;
  uint8_t number_;
  std::string reason_;
};

// Implements STUN attributes that record a list of attribute names.
class StunUInt16ListAttribute : public StunAttribute {
 public:
  StunUInt16ListAttribute(uint16_t type, uint16_t length);
  ~StunUInt16ListAttribute() override;

  StunAttributeValueType value_type() const override;

  size_t Size() const;
  uint16_t GetType(int index) const;
  void SetType(int index, uint16_t value);
  void AddType(uint16_t value);
  void AddTypeAtIndex(uint16_t index, uint16_t value);

  bool Read(ByteBufferReader* buf) override;
  bool Write(ByteBufferWriter* buf) const override;

 private:
  std::vector<uint16_t>* attr_types_;
};

// Return a string e.g "STUN BINDING request".
std::string StunMethodToString(int msg_type);

// Returns the (successful) response type for the given request type.
// Returns -1 if |request_type| is not a valid request type.
int GetStunSuccessResponseType(int request_type);

// Returns the error response type for the given request type.
// Returns -1 if |request_type| is not a valid request type.
int GetStunErrorResponseType(int request_type);

// Returns whether a given message is a request type.
bool IsStunRequestType(int msg_type);

// Returns whether a given message is an indication type.
bool IsStunIndicationType(int msg_type);

// Returns whether a given response is a success type.
bool IsStunSuccessResponseType(int msg_type);

// Returns whether a given response is an error type.
bool IsStunErrorResponseType(int msg_type);

// Computes the STUN long-term credential hash.
bool ComputeStunCredentialHash(const std::string& username,
                               const std::string& realm,
                               const std::string& password, std::string* hash);

// Make a copy af |attribute| and return a new StunAttribute.
//   This is useful if you don't care about what kind of attribute you
//   are handling.
//
// The implementation copies by calling Write() followed by Read().
//
// If |tmp_buffer| is supplied this buffer will be used, otherwise
// a buffer will created in the method.
std::unique_ptr<StunAttribute> CopyStunAttribute(
    const StunAttribute& attribute, ByteBufferWriter* tmp_buffer_ptr = 0);

// TODO(?): Move the TURN/ICE stuff below out to separate files.
extern const char TURN_MAGIC_COOKIE_VALUE[4];

// "GTURN" STUN methods.
// TODO(?): Rename these methods to GTURN_ to make it clear they aren't
// part of standard STUN/TURN.
enum RelayMessageType {
  // For now, using the same defs from TurnMessageType below.
  // STUN_ALLOCATE_REQUEST              = 0x0003,
  // STUN_ALLOCATE_RESPONSE             = 0x0103,
  // STUN_ALLOCATE_ERROR_RESPONSE       = 0x0113,
  STUN_SEND_REQUEST = 0x0004,
  STUN_SEND_RESPONSE = 0x0104,
  STUN_SEND_ERROR_RESPONSE = 0x0114,
  STUN_DATA_INDICATION = 0x0115,
};

class RelayMessage : public StunMessage {
 protected:
  StunAttributeValueType GetAttributeValueType(int type) const override;
  StunMessage* CreateNew() const override;
};

// Defined in TURN RFC 5766.
enum TurnMessageType {
  STUN_ALLOCATE_REQUEST = 0x0003,
  STUN_ALLOCATE_RESPONSE = 0x0103,
  STUN_ALLOCATE_ERROR_RESPONSE = 0x0113,
  TURN_REFRESH_REQUEST = 0x0004,
  TURN_REFRESH_RESPONSE = 0x0104,
  TURN_REFRESH_ERROR_RESPONSE = 0x0114,
  TURN_SEND_INDICATION = 0x0016,
  TURN_DATA_INDICATION = 0x0017,
  TURN_CREATE_PERMISSION_REQUEST = 0x0008,
  TURN_CREATE_PERMISSION_RESPONSE = 0x0108,
  TURN_CREATE_PERMISSION_ERROR_RESPONSE = 0x0118,
  TURN_CHANNEL_BIND_REQUEST = 0x0009,
  TURN_CHANNEL_BIND_RESPONSE = 0x0109,
  TURN_CHANNEL_BIND_ERROR_RESPONSE = 0x0119,
};

class TurnMessage : public StunMessage {
 protected:
  StunAttributeValueType GetAttributeValueType(int type) const override;
  StunMessage* CreateNew() const override;
};

// When adding new attributes to STUN_ATTR_GOOG_MISC_INFO
// (which is a list of uint16_t), append the indices of these attributes below
// and do NOT change the existing indices. The indices of attributes must be
// consistent with those used in ConnectionRequest::Prepare when forming a STUN
// message for the ICE connectivity check, and they are used when parsing a
// received STUN message.
enum class IceGoogMiscInfoBindingRequestAttributeIndex {
  SUPPORT_GOOG_PING_VERSION = 0,
};

enum class IceGoogMiscInfoBindingResponseAttributeIndex {
  SUPPORT_GOOG_PING_VERSION = 0,
};

// A RFC 5245 ICE STUN message.
class IceMessage : public StunMessage {
 protected:
  StunAttributeValueType GetAttributeValueType(int type) const override;
  StunMessage* CreateNew() const override;
};

enum StunClassType {
  kClassRequest = 0x0000,
  kClassIndication = 0x0010,
  kClassSuccessResponse = 0x0100,
  kClassErrorResponse = 0x0110
};

#if 0
inline bool isChannelData(uint16_t msgType)
{
  // The first two bits of a channel data message are 0b01.
  return ((msgType & 0xC000) == 0x4000);
}

inline bool isRequestType(int msgType) {
     return ((msgType & 0x0110) == 0x000);
}

inline bool isIndicationType(int msgType) {
    return ((msgType & 0x0110) == 0x010);
}

inline bool isSuccessResponseType(int msgType) {
    return ((msgType & 0x0110) == 0x100);
}

inline bool isErrorResponseType(int msgType) {
  return ((msgType & 0x0110) == 0x110);
}

inline int getSuccessResponseType(int reqType) {
    return isRequestType(reqType) ? (reqType | 0x100) : -1;
}

inline int getErrorResponseType(int reqType) {
    return isRequestType(reqType) ? (reqType | 0x110) : -1;
}

#define IS_STUN_REQUEST(msgType) (((msgType) & 0x0110) == 0x0000)
#define IS_STUN_INDICATION(msgType) (((msgType) & 0x0110) == 0x0010)
#define IS_STUN_SUCCESS_RESP(msgType) (((msgType) & 0x0110) == 0x0100)
#define IS_STUN_ERR_RESP(msgType) (((msgType) & 0x0110) == 0x0110)

#define GET_STUN_REQUEST(msgType) (msgType & 0xFEEF)
#define GET_STUN_INDICATION(msgType) ((msgType & 0xFEEF) | 0x0010)
#define GET_STUN_SUCCESS_RESP(msgType) ((msgType & 0xFEEF) | 0x0100)
#define GET_STUN_ERR_RESP(msgType) (msgType | 0x0110)

#define STUN_HEADER_LENGTH (20)
#define STUN_CHANNEL_HEADER_LENGTH (4)

#define STUN_MAX_USERNAME_SIZE (513)
#define STUN_MAX_REALM_SIZE (127)
#define STUN_MAX_NONCE_SIZE (127)
#define STUN_MAX_PWD_SIZE (127)

#define STUN_MAGIC_COOKIE (0x2112A442)

// Lifetimes:
#define STUN_DEFAULT_ALLOCATE_LIFETIME (600)
#define STUN_MIN_ALLOCATE_LIFETIME STUN_DEFAULT_ALLOCATE_LIFETIME
#define STUN_MAX_ALLOCATE_LIFETIME (3600)
#define STUN_CHANNEL_LIFETIME (600)
#define STUN_PERMISSION_LIFETIME (300)
#define STUN_NONCE_EXPIRATION_TIME (600)
#endif
}

#endif
