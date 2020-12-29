#include "stun.h"

#include <string.h>

#include <algorithm>
#include <memory>
#include <utility>
#include <unordered_map>

#include "crc.h"
#include "message_digest.h"

#include <butil/logging.h>

namespace {

uint32_t ReduceTransactionId(const std::string& transaction_id) {
  assert(transaction_id.length() == kStunTransactionIdLength ||
         transaction_id.length() == kStunLegacyTransactionIdLength);
  ByteBufferReader reader(transaction_id.c_str(), transaction_id.length());
  uint32_t result = 0;
  uint32_t next;
  while (reader.ReadUInt32(&next)) {
    result ^= next;
  }
  return result;
}

}  // namespace

namespace MSF {

const char TURN_MAGIC_COOKIE_VALUE[] = {'\x72', '\xC6', '\x4B', '\xC6'};
const char EMPTY_TRANSACTION_ID[] = "0000000000000000";
const uint32_t STUN_FINGERPRINT_XOR_VALUE = 0x5354554E;
const int SERVER_NOT_REACHABLE_ERROR = 701;

static std::unordered_map<uint16_t, std::string> kStunErrorCodeMap = {
    {kStunTryAlteate, "Try Alternate"},
    {kStunBadRequest, "Bad Request"},
    {kStunNotAuthorized, "Unauthorized"},
    {kStunForbidden, "Forbidden"},
    {kStunUnknownAttribute, "Unknown Attribute"},
    {kStunAllocationMismatch, "Allocation Mismatch"},
    {kStunStaleNonce, "Stale Nonce"},
    // {KStunIntegrityCheckFailure, "Integritt Check Failure"},
    {kStunMissingUsername, "Missing Username"},
    {kStunUseTLS, "Use TLS"},
    {kStunRoleConflict, "Role Conflict"},
    {kStunAddrFamilyNotSupport, "Address Family Not Supported"},
    {kStunWrongCredentials, "Wrong Credentials"},
    {kStunUnsuppTransportProto, "Unsupported Transport Protocol"},
    {kStunPeerAddrFamilyMismatch, "Peer Address Family Mismatch"},
    {kStunConnectionAlreadyExists, "Connection Already Exists"},
    {kStunConnectionFailure, "Connection Failure"},
    {kStunAllocationQuotaReached, "Allocation Quota Reached"},
    {kStunRoleConflict, "Role Conflict"},
    {kStunServerError, "Server Error"},
    {kStunGlobalFailure, "Global Failure"},
    {kStunInsufficientCapacity, "Insufficient Capacity"},
    {kStunConnectionTimeoutOrFailure, "Connection Timeout or Failure"}};

std::string StunErrorToString(uint16_t code) {
  auto it = kStunErrorCodeMap.find(code);
  if (it != kStunErrorCodeMap.end()) {
    return it->second;
  }
  return "UnknownError";
}

static std::unordered_map<uint16_t, std::string> kStunAttrTypeMap = {
    {kStunAttrNotExist, "Not Exist"},
    {kStunAttrMappedAddress, "MAPPED-ADDRESS"},
    {kStunAttrResponseAddress, "RESPONSE-ADDRESS"},
    {kStunAttrChangeRequest, "CHANGE-REQUEST"},
    {kStunAttrSourceAddress, "SOURCE-ADDRESS"},
    {kStunAttrChangedAddress, "CHANGED-ADDRESS"},
    {kStunAttrUsername, "USERNAME"},
    {kStunAttrPassword, "PASSWORD"},
    {kStunAttrMessageIntegrity, "MESSAGE-INTEGRITY"},
    {kStunAttrStunErrorCode, "ERROR-CODE"},
    {kStunAttrBandwidth, "BANDWIDTH"},
    {kStunAttrDestinationAddress, "DESTINATION-ADDRESS"},
    {kStunAttrRealm, "REALM"},
    {kStunAttrNonce, "NONCE"},
    {kStunAttrUnknownAttributes, "UNKNOWN-ATTRIBUTES"},
    {kStunAttrReflectedFrom, "REFLECTED-FORM"},
    // {kStunAttrTransportPreferences, "TRANSPORT-PREFERENCES"},
    {kStunAttrChannelNumber, "CHANNEL-NUMBER"},
    {kStunAttrLifetime, "LIFETIME"},
    {kStunAttrAlternateServer, "ALTERNATE-SERVER"},
    {kStunAttrMagicCookie, "MAGIC-COOKIE"},
    {kStunAttrXorPeerAddress, "XOR-PEER-ADDRESS"},
    {kStunAttrData, "DATA"},
    {kStunAttrXorRelayedAddress, "XOR-RELAYED-ADDRESS"},
    {kStunAttrEventPort, "EVEN-PORT"},
    {kStunAttrRequestedTransport, "REQUESTED-TRANSPORT"},
    {kStunAttrDontFragment, "DONT-FRAGMENT"},
    {kStunAttrXorMappedAddress, "XOR-MAPPED-ADDRESS"},
    /// 0x0021: Reserved (was TIMER-VAL)
    {kStunAttrReservationToken, "RESERVED-TOKEN"},
    {kStunAttrOptions, "OPTIONS"},
    {kStunAttrSoftware, "SOFTWARE"},
    {SkStunAttrAlternateServer, ""},
    {kStunAttrFingerprint, "FINGERPRINT"},
    {kStunAttrOrigin, "ORIGIN"},
    {kStunAttrConnectionID, "CONNECTION-ID"},
    {kStunAttrRetransmitCount, "RETRANSMIT-COUNT"},
    {kStunAttrICEControlled, "ICE-CONTROLLED"},
    {kStunAttrICEControlling, "ICE-CONTROLLING"},
    {kStunAttrICEPriority, "PRIORITY"},
    {kStunAttrICEUseCandidate, "USE-CANDIDATE"}, };

std::string StunAttrToString(uint16_t type) {
  auto it = kStunAttrTypeMap.find(type);
  if (it != kStunAttrTypeMap.end()) {
    return it->second;
  }
  return "StunAttrUnknown";
}

// StunMessage

StunMessage::StunMessage()
    : type_(0),
      length_(0),
      transaction_id_(EMPTY_TRANSACTION_ID),
      stun_magic_cookie_(kStunMagicCookie) {
  assert(IsValidTransactionId(transaction_id_));
}

StunMessage::~StunMessage() = default;

bool StunMessage::IsLegacy() const {
  if (transaction_id_.size() == kStunLegacyTransactionIdLength) return true;
  assert(transaction_id_.size() == kStunTransactionIdLength);
  return false;
}

bool StunMessage::SetTransactionID(const std::string& str) {
  if (!IsValidTransactionId(str)) {
    return false;
  }
  transaction_id_ = str;
  reduced_transaction_id_ = ReduceTransactionId(transaction_id_);
  return true;
}

static bool DesignatedExpertRange(int attr_type) {
  return (attr_type >= 0x4000 && attr_type <= 0x7FFF) ||
         (attr_type >= 0xC000 && attr_type <= 0xFFFF);
}

void StunMessage::AddAttribute(std::unique_ptr<StunAttribute> attr) {
  // Fail any attributes that aren't valid for this type of message,
  // but allow any type for the range that in the RFC is reserved for
  // the "designated experts".
  if (!DesignatedExpertRange(attr->type())) {
    assert(attr->value_type() == GetAttributeValueType(attr->type()));
  }

  attr->SetOwner(this);
  size_t attr_length = attr->length();
  if (attr_length % 4 != 0) {
    attr_length += (4 - (attr_length % 4));
  }
  length_ += static_cast<uint16_t>(attr_length + 4);

  attrs_.push_back(std::move(attr));
}

std::unique_ptr<StunAttribute> StunMessage::RemoveAttribute(int type) {
  std::unique_ptr<StunAttribute> attribute;
  for (auto it = attrs_.rbegin(); it != attrs_.rend(); ++it) {
    if ((*it)->type() == type) {
      attribute = std::move(*it);
      attrs_.erase(std::next(it).base());
      break;
    }
  }
  if (attribute) {
    attribute->SetOwner(nullptr);
    size_t attr_length = attribute->length();
    if (attr_length % 4 != 0) {
      attr_length += (4 - (attr_length % 4));
    }
    length_ -= static_cast<uint16_t>(attr_length + 4);
  }
  return attribute;
}

void StunMessage::ClearAttributes() {
  for (auto it = attrs_.rbegin(); it != attrs_.rend(); ++it) {
    (*it)->SetOwner(nullptr);
  }
  attrs_.clear();
  length_ = 0;
}

const StunAddressAttribute* StunMessage::GetAddress(int type) const {
  switch (type) {
    case kStunAttrMappedAddress: {
      // Return XOR-MAPPED-ADDRESS when MAPPED-ADDRESS attribute is
      // missing.
      const StunAttribute* mapped_address =
          GetAttribute(kStunAttrMappedAddress);
      if (!mapped_address)
        mapped_address = GetAttribute(kStunAttrXorMappedAddress);
      return reinterpret_cast<const StunAddressAttribute*>(mapped_address);
    }

    default:
      return static_cast<const StunAddressAttribute*>(GetAttribute(type));
  }
}

const StunUInt32Attribute* StunMessage::GetUInt32(int type) const {
  return static_cast<const StunUInt32Attribute*>(GetAttribute(type));
}

const StunUInt64Attribute* StunMessage::GetUInt64(int type) const {
  return static_cast<const StunUInt64Attribute*>(GetAttribute(type));
}

const StunByteStringAttribute* StunMessage::GetByteString(int type) const {
  return static_cast<const StunByteStringAttribute*>(GetAttribute(type));
}

const StunUInt16ListAttribute* StunMessage::GetUInt16List(int type) const {
  return static_cast<const StunUInt16ListAttribute*>(GetAttribute(type));
}

const StunErrorCodeAttribute* StunMessage::GetErrorCode() const {
  return static_cast<const StunErrorCodeAttribute*>(
      GetAttribute(kStunAttrStunErrorCode));
}

int StunMessage::GetErrorCodeValue() const {
  const StunErrorCodeAttribute* error_attribute = GetErrorCode();
  return error_attribute ? error_attribute->code() : kStunGlobalFailure;
}

const StunUInt16ListAttribute* StunMessage::GetUnknownAttributes() const {
  return static_cast<const StunUInt16ListAttribute*>(
      GetAttribute(kStunAttrUnknownAttributes));
}

bool StunMessage::ValidateMessageIntegrity(const char* data, size_t size,
                                           const std::string& password) {
  return ValidateMessageIntegrityOfType(kStunAttrMessageIntegrity,
                                        kStunMessageIntegritySize, data, size,
                                        password);
}

bool StunMessage::ValidateMessageIntegrity32(const char* data, size_t size,
                                             const std::string& password) {
  return ValidateMessageIntegrityOfType(kStunAttrGoodMessageInteger32,
                                        kStunMessageIntegrity32Size, data, size,
                                        password);
}

// Verifies a STUN message has a valid MESSAGE-INTEGRITY attribute, using the
// procedure outlined in RFC 5389, section 15.4.
bool StunMessage::ValidateMessageIntegrityOfType(int mi_attr_type,
                                                 size_t mi_attr_size,
                                                 const char* data, size_t size,
                                                 const std::string& password) {
  assert(mi_attr_size <= kStunMessageIntegritySize);

  // Verifying the size of the message.
  if ((size % 4) != 0 || size < kStunHeaderSize) {
    return false;
  }

  // Getting the message length from the STUN header.
  size_t msg_length = GetBE16(&data[2]);
  if (size != (msg_length + kStunHeaderSize)) {
    return false;
  }

  // Finding Message Integrity attribute in stun message.
  size_t current_pos = kStunHeaderSize;
  bool has_message_integrity_attr = false;
  while (current_pos + 4 <= size) {
    uint16_t attr_type, attr_length;
    // Getting attribute type and length.
    attr_type = GetBE16(&data[current_pos]);
    attr_length = GetBE16(&data[current_pos + sizeof(attr_type)]);

    // If M-I, sanity check it, and break out.
    if (attr_type == mi_attr_type) {
      if (attr_length != mi_attr_size ||
          current_pos + sizeof(attr_type) + sizeof(attr_length) + attr_length >
              size) {
        return false;
      }
      has_message_integrity_attr = true;
      break;
    }

    // Otherwise, skip to the next attribute.
    current_pos += sizeof(attr_type) + sizeof(attr_length) + attr_length;
    if ((attr_length % 4) != 0) {
      current_pos += (4 - (attr_length % 4));
    }
  }

  if (!has_message_integrity_attr) {
    return false;
  }

  // Getting length of the message to calculate Message Integrity.
  size_t mi_pos = current_pos;
  std::unique_ptr<char[]> temp_data(new char[current_pos]);
  memcpy(temp_data.get(), data, current_pos);
  if (size > mi_pos + kStunAttributeHeaderSize + mi_attr_size) {
    // Stun message has other attributes after message integrity.
    // Adjust the length parameter in stun message to calculate HMAC.
    size_t extra_offset =
        size - (mi_pos + kStunAttributeHeaderSize + mi_attr_size);
    size_t new_adjusted_len = size - extra_offset - kStunHeaderSize;

    // Writing new length of the STUN message @ Message Length in temp buffer.
    //      0                   1                   2                   3
    //      0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
    //     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    //     |0 0|     STUN Message Type     |         Message Length        |
    //     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    SetBE16(temp_data.get() + 2, static_cast<uint16_t>(new_adjusted_len));
  }

  char hmac[kStunMessageIntegritySize];
  size_t ret = ComputeHmac(DIGEST_SHA_1, password.c_str(), password.size(),
                           temp_data.get(), mi_pos, hmac, sizeof(hmac));
  assert(ret == sizeof(hmac));
  if (ret != sizeof(hmac)) {
    return false;
  }

  // Comparing the calculated HMAC with the one present in the message.
  return memcmp(data + current_pos + kStunAttributeHeaderSize, hmac,
                mi_attr_size) == 0;
}

bool StunMessage::AddMessageIntegrity(const std::string& password) {
  return AddMessageIntegrityOfType(kStunAttrMessageIntegrity,
                                   kStunMessageIntegritySize, password.c_str(),
                                   password.size());
}

bool StunMessage::AddMessageIntegrity(const char* key, size_t keylen) {
  return AddMessageIntegrityOfType(kStunAttrMessageIntegrity,
                                   kStunMessageIntegritySize, key, keylen);
}

bool StunMessage::AddMessageIntegrity32(std::string_view password) {
  return AddMessageIntegrityOfType(kStunAttrGoodMessageInteger32,
                                   kStunMessageIntegrity32Size, password.data(),
                                   password.length());
}

bool StunMessage::AddMessageIntegrityOfType(int attr_type, size_t attr_size,
                                            const char* key, size_t keylen) {
  // Add the attribute with a dummy value. Since this is a known attribute, it
  // can't fail.
  assert(attr_size <= kStunMessageIntegritySize);
  auto msg_integrity_attr_ptr = std::make_unique<StunByteStringAttribute>(
      attr_type, std::string(attr_size, '0'));
  auto* msg_integrity_attr = msg_integrity_attr_ptr.get();
  AddAttribute(std::move(msg_integrity_attr_ptr));

  // Calculate the HMAC for the message.
  ByteBufferWriter buf;
  if (!Write(&buf)) return false;

  int msg_len_for_hmac = static_cast<int>(
      buf.Length() - kStunAttributeHeaderSize - msg_integrity_attr->length());
  char hmac[kStunMessageIntegritySize];
  size_t ret = ComputeHmac(DIGEST_SHA_1, key, keylen, buf.Data(),
                           msg_len_for_hmac, hmac, sizeof(hmac));
  assert(ret == sizeof(hmac));
  if (ret != sizeof(hmac)) {
    LOG(ERROR) << "HMAC computation failed. Message-Integrity "
                  "has dummy value.";
    return false;
  }

  // Insert correct HMAC into the attribute.
  msg_integrity_attr->CopyBytes(hmac, attr_size);
  return true;
}

// Verifies a message is in fact a STUN message, by performing the checks
// outlined in RFC 5389, section 7.3, including the FINGERPRINT check detailed
// in section 15.5.
bool StunMessage::ValidateFingerprint(const char* data, size_t size) {
  // Check the message length.
  size_t fingerprint_attr_size =
      kStunAttributeHeaderSize + StunUInt32Attribute::SIZE;
  if (size % 4 != 0 || size < kStunHeaderSize + fingerprint_attr_size)
    return false;

  // Skip the rest if the magic cookie isn't present.
  const char* magic_cookie =
      data + kStunTransactionIdOffset - kStunMagicCookieLength;
  if (GetBE32(magic_cookie) != kStunMagicCookie) return false;

  // Check the fingerprint type and length.
  const char* fingerprint_attr_data = data + size - fingerprint_attr_size;
  if (GetBE16(fingerprint_attr_data) != kStunAttrFingerprint ||
      GetBE16(fingerprint_attr_data + sizeof(uint16_t)) !=
          StunUInt32Attribute::SIZE)
    return false;

  // Check the fingerprint value.
  uint32_t fingerprint =
      GetBE32(fingerprint_attr_data + kStunAttributeHeaderSize);
  return ((fingerprint ^ STUN_FINGERPRINT_XOR_VALUE) ==
          Crc32((uint8_t*)(data), size - fingerprint_attr_size));
}

bool StunMessage::IsStunMethod(ArrayView<int> methods, const char* data,
                               size_t size) {
  // Check the message length.
  if (size % 4 != 0 || size < kStunHeaderSize) return false;

  // Skip the rest if the magic cookie isn't present.
  const char* magic_cookie =
      data + kStunTransactionIdOffset - kStunMagicCookieLength;
  if (GetBE32(magic_cookie) != kStunMagicCookie) return false;

  int method = GetBE16(data);
  for (int m : methods) {
    if (m == method) {
      return true;
    }
  }
  return false;
}

bool StunMessage::AddFingerprint() {
  // Add the attribute with a dummy value. Since this is a known attribute,
  // it can't fail.
  auto fingerprint_attr_ptr =
      std::make_unique<StunUInt32Attribute>(kStunAttrFingerprint, 0);
  auto* fingerprint_attr = fingerprint_attr_ptr.get();
  AddAttribute(std::move(fingerprint_attr_ptr));

  // Calculate the CRC-32 for the message and insert it.
  ByteBufferWriter buf;
  if (!Write(&buf)) return false;

  int msg_len_for_crc32 = static_cast<int>(
      buf.Length() - kStunAttributeHeaderSize - fingerprint_attr->length());
  uint32_t c = Crc32((uint8_t*)(buf.Data()), msg_len_for_crc32);

  // Insert the correct CRC-32, XORed with a constant, into the attribute.
  fingerprint_attr->SetValue(c ^ STUN_FINGERPRINT_XOR_VALUE);
  return true;
}

bool StunMessage::Read(ByteBufferReader* buf) {
  if (!buf->ReadUInt16(&type_)) {
    return false;
  }

  if (type_ & 0x8000) {
    // RTP and RTCP set the MSB of first byte, since first two bits are version,
    // and version is always 2 (10). If set, this is not a STUN packet.
    return false;
  }

  if (!buf->ReadUInt16(&length_)) {
    return false;
  }

  std::string magic_cookie;
  if (!buf->ReadString(&magic_cookie, kStunMagicCookieLength)) {
    return false;
  }

  std::string transaction_id;
  if (!buf->ReadString(&transaction_id, kStunTransactionIdLength)) {
    return false;
  }

  uint32_t magic_cookie_int;
  static_assert(sizeof(magic_cookie_int) == kStunMagicCookieLength,
                "Integer size mismatch: magic_cookie_int and kStunMagicCookie");
  std::memcpy(&magic_cookie_int, magic_cookie.data(), sizeof(magic_cookie_int));
  if (NetworkToHost32(magic_cookie_int) != kStunMagicCookie) {
    // If magic cookie is invalid it means that the peer implements
    // RFC3489 instead of RFC5389.
    transaction_id.insert(0, magic_cookie);
  }
  assert(IsValidTransactionId(transaction_id));
  transaction_id_ = transaction_id;
  reduced_transaction_id_ = ReduceTransactionId(transaction_id_);

  if (length_ != buf->Length()) {
    return false;
  }

  attrs_.resize(0);

  size_t rest = buf->Length() - length_;
  while (buf->Length() > rest) {
    uint16_t attr_type, attr_length;
    if (!buf->ReadUInt16(&attr_type)) return false;
    if (!buf->ReadUInt16(&attr_length)) return false;

    std::unique_ptr<StunAttribute> attr(
        CreateAttribute(attr_type, attr_length));
    if (!attr) {
      // Skip any unknown or malformed attributes.
      if ((attr_length % 4) != 0) {
        attr_length += (4 - (attr_length % 4));
      }
      if (!buf->Consume(attr_length)) {
        return false;
      }
    } else {
      if (!attr->Read(buf)) {
        return false;
      }
      attrs_.push_back(std::move(attr));
    }
  }

  assert(buf->Length() == rest);
  return true;
}

bool StunMessage::Write(ByteBufferWriter* buf) const {
  buf->WriteUInt16(type_);
  buf->WriteUInt16(length_);
  if (!IsLegacy()) buf->WriteUInt32(stun_magic_cookie_);
  buf->WriteString(transaction_id_);

  for (const auto& attr : attrs_) {
    buf->WriteUInt16(attr->type());
    buf->WriteUInt16(static_cast<uint16_t>(attr->length()));
    if (!attr->Write(buf)) {
      return false;
    }
  }

  return true;
}

StunMessage* StunMessage::CreateNew() const { return new StunMessage(); }

void StunMessage::SetStunMagicCookie(uint32_t val) { stun_magic_cookie_ = val; }

StunAttributeValueType StunMessage::GetAttributeValueType(int type) const {
  switch (type) {
    case kStunAttrMappedAddress:
      return kStunAttrValueAddress;
    case kStunAttrUsername:
      return kStunAttrValueByteString;
    case kStunAttrMessageIntegrity:
      return kStunAttrValueByteString;
    case kStunAttrStunErrorCode:
      return kStunAttrValueErrorCode;
    case kStunAttrUnknownAttributes:
      return kStunAttrValueUInt16List;
    case kStunAttrRealm:
      return kStunAttrValueByteString;
    case kStunAttrNonce:
      return kStunAttrValueByteString;
    case kStunAttrXorMappedAddress:
      return kStunAttrValueXorAddress;
    case kStunAttrSoftware:
      return kStunAttrValueByteString;
    case kStunAttrAlternateServer:
      return kStunAttrValueAddress;
    case kStunAttrFingerprint:
      return kStunAttrValueUInt32;
    case kStunAttrOrigin:
      return kStunAttrValueByteString;
    case kStunAttrRetransmitCount:
      return kStunAttrValueUInt32;
    case kStunAttrLastIceCheckReceived:
      return kStunAttrValueByteString;
    case kStunAttrGoodMiscInfo:
      return kStunAttrValueUInt16List;
    default:
      return kStunAttrValueUnknown;
  }
}

StunAttribute* StunMessage::CreateAttribute(int type, size_t length) /*const*/ {
  StunAttributeValueType value_type = GetAttributeValueType(type);
  if (value_type != kStunAttrValueUnknown) {
    return StunAttribute::Create(value_type, type,
                                 static_cast<uint16_t>(length), this);
  } else if (DesignatedExpertRange(type)) {
    // Read unknown attributes as kStunAttrValueByteString
    return StunAttribute::Create(kStunAttrValueByteString, type,
                                 static_cast<uint16_t>(length), this);
  } else {
    return NULL;
  }
}

const StunAttribute* StunMessage::GetAttribute(int type) const {
  for (const auto& attr : attrs_) {
    if (attr->type() == type) {
      return attr.get();
    }
  }
  return NULL;
}

bool StunMessage::IsValidTransactionId(const std::string& transaction_id) {
  return transaction_id.size() == kStunTransactionIdLength ||
         transaction_id.size() == kStunLegacyTransactionIdLength;
}

bool StunMessage::EqualAttributes(
    const StunMessage* other,
    std::function<bool(int type)> attribute_type_mask) const {
  assert(other != nullptr);
  ByteBufferWriter tmp_buffer_ptr1;
  ByteBufferWriter tmp_buffer_ptr2;
  for (const auto& attr : attrs_) {
    if (attribute_type_mask(attr->type())) {
      const StunAttribute* other_attr = other->GetAttribute(attr->type());
      if (other_attr == nullptr) {
        return false;
      }
      tmp_buffer_ptr1.Clear();
      tmp_buffer_ptr2.Clear();
      attr->Write(&tmp_buffer_ptr1);
      other_attr->Write(&tmp_buffer_ptr2);
      if (tmp_buffer_ptr1.Length() != tmp_buffer_ptr2.Length()) {
        return false;
      }
      if (memcmp(tmp_buffer_ptr1.Data(), tmp_buffer_ptr2.Data(),
                 tmp_buffer_ptr1.Length()) != 0) {
        return false;
      }
    }
  }

  for (const auto& attr : other->attrs_) {
    if (attribute_type_mask(attr->type())) {
      const StunAttribute* own_attr = GetAttribute(attr->type());
      if (own_attr == nullptr) {
        return false;
      }
      // we have already compared all values...
    }
  }
  return true;
}

// StunAttribute

StunAttribute::StunAttribute(uint16_t type, uint16_t length)
    : type_(type), length_(length) {}

void StunAttribute::ConsumePadding(ByteBufferReader* buf) const {
  int remainder = length_ % 4;
  if (remainder > 0) {
    buf->Consume(4 - remainder);
  }
}

void StunAttribute::WritePadding(ByteBufferWriter* buf) const {
  int remainder = length_ % 4;
  if (remainder > 0) {
    char zeroes[4] = {0};
    buf->WriteBytes(zeroes, 4 - remainder);
  }
}

StunAttribute* StunAttribute::Create(StunAttributeValueType value_type,
                                     uint16_t type, uint16_t length,
                                     StunMessage* owner) {
  switch (value_type) {
    case kStunAttrValueAddress:
      return new StunAddressAttribute(type, length);
    case kStunAttrValueXorAddress:
      return new StunXorAddressAttribute(type, length, owner);
    case kStunAttrValueUInt32:
      return new StunUInt32Attribute(type);
    case kStunAttrValueUInt64:
      return new StunUInt64Attribute(type);
    case kStunAttrValueByteString:
      return new StunByteStringAttribute(type, length);
    case kStunAttrValueErrorCode:
      return new StunErrorCodeAttribute(type, length);
    case kStunAttrValueUInt16List:
      return new StunUInt16ListAttribute(type, length);
    default:
      return nullptr;
  }
}

std::unique_ptr<StunAddressAttribute> StunAttribute::CreateAddress(
    uint16_t type) {
  return std::make_unique<StunAddressAttribute>(type, 0);
}

std::unique_ptr<StunXorAddressAttribute> StunAttribute::CreateXorAddress(
    uint16_t type) {
  return std::make_unique<StunXorAddressAttribute>(type, 0, nullptr);
}

std::unique_ptr<StunUInt64Attribute> StunAttribute::CreateUInt64(
    uint16_t type) {
  return std::make_unique<StunUInt64Attribute>(type);
}

std::unique_ptr<StunUInt32Attribute> StunAttribute::CreateUInt32(
    uint16_t type) {
  return std::make_unique<StunUInt32Attribute>(type);
}

std::unique_ptr<StunByteStringAttribute> StunAttribute::CreateByteString(
    uint16_t type) {
  return std::make_unique<StunByteStringAttribute>(type, 0);
}

std::unique_ptr<StunErrorCodeAttribute> StunAttribute::CreateErrorCode() {
  return std::make_unique<StunErrorCodeAttribute>(
      kStunAttrStunErrorCode, StunErrorCodeAttribute::MIN_SIZE);
}

std::unique_ptr<StunUInt16ListAttribute>
StunAttribute::CreateUInt16ListAttribute(uint16_t type) {
  return std::make_unique<StunUInt16ListAttribute>(type, 0);
}

std::unique_ptr<StunUInt16ListAttribute>
StunAttribute::CreateUnknownAttributes() {
  return std::make_unique<StunUInt16ListAttribute>(kStunAttrUnknownAttributes,
                                                   0);
}

StunAddressAttribute::StunAddressAttribute(uint16_t type,
                                           const SocketAddress& addr)
    : StunAttribute(type, 0) {
  SetAddress(addr);
}

StunAddressAttribute::StunAddressAttribute(uint16_t type, uint16_t length)
    : StunAttribute(type, length) {}

StunAttributeValueType StunAddressAttribute::value_type() const {
  return kStunAttrValueAddress;
}

bool StunAddressAttribute::Read(ByteBufferReader* buf) {
  uint8_t dummy;
  if (!buf->ReadUInt8(&dummy)) return false;

  uint8_t stun_family;
  if (!buf->ReadUInt8(&stun_family)) {
    return false;
  }
  uint16_t port;
  if (!buf->ReadUInt16(&port)) return false;
  if (stun_family == kStunAddressIpv4) {
    in_addr v4addr;
    if (length() != SIZE_IP4) {
      return false;
    }
    if (!buf->ReadBytes(reinterpret_cast<char*>(&v4addr), sizeof(v4addr))) {
      return false;
    }
    IPAddress ipaddr(v4addr);
    SetAddress(SocketAddress(ipaddr, port));
  } else if (stun_family == kStunAddressIpv6) {
    in6_addr v6addr;
    if (length() != SIZE_IP6) {
      return false;
    }
    if (!buf->ReadBytes(reinterpret_cast<char*>(&v6addr), sizeof(v6addr))) {
      return false;
    }
    IPAddress ipaddr(v6addr);
    SetAddress(SocketAddress(ipaddr, port));
  } else {
    return false;
  }
  return true;
}

bool StunAddressAttribute::Write(ByteBufferWriter* buf) const {
  StunAddressFamily address_family = family();
  if (address_family == kStunAddressUndef) {
    LOG(ERROR) << "Error writing address attribute: unknown family.";
    return false;
  }
  buf->WriteUInt8(0);
  buf->WriteUInt8(address_family);
  buf->WriteUInt16(address_.port());
  switch (address_.family()) {
    case AF_INET: {
      in_addr v4addr = address_.ipaddr().ipv4_address();
      buf->WriteBytes(reinterpret_cast<char*>(&v4addr), sizeof(v4addr));
      break;
    }
    case AF_INET6: {
      in6_addr v6addr = address_.ipaddr().ipv6_address();
      buf->WriteBytes(reinterpret_cast<char*>(&v6addr), sizeof(v6addr));
      break;
    }
  }
  return true;
}

StunXorAddressAttribute::StunXorAddressAttribute(uint16_t type,
                                                 const SocketAddress& addr)
    : StunAddressAttribute(type, addr), owner_(NULL) {}

StunXorAddressAttribute::StunXorAddressAttribute(uint16_t type, uint16_t length,
                                                 StunMessage* owner)
    : StunAddressAttribute(type, length), owner_(owner) {}

StunAttributeValueType StunXorAddressAttribute::value_type() const {
  return kStunAttrValueXorAddress;
}

void StunXorAddressAttribute::SetOwner(StunMessage* owner) { owner_ = owner; }

IPAddress StunXorAddressAttribute::GetXoredIP() const {
  if (owner_) {
    IPAddress ip = ipaddr();
    switch (ip.family()) {
      case AF_INET: {
        in_addr v4addr = ip.ipv4_address();
        v4addr.s_addr = (v4addr.s_addr ^ HostToNetwork32(kStunMagicCookie));
        return IPAddress(v4addr);
      }
      case AF_INET6: {
        in6_addr v6addr = ip.ipv6_address();
        const std::string& transaction_id = owner_->transaction_id();
        if (transaction_id.length() == kStunTransactionIdLength) {
          uint32_t transactionid_as_ints[3];
          memcpy(&transactionid_as_ints[0], transaction_id.c_str(),
                 transaction_id.length());
          uint32_t* ip_as_ints = reinterpret_cast<uint32_t*>(&v6addr.s6_addr);
          // Transaction ID is in network byte order, but magic cookie
          // is stored in host byte order.
          ip_as_ints[0] = (ip_as_ints[0] ^ HostToNetwork32(kStunMagicCookie));
          ip_as_ints[1] = (ip_as_ints[1] ^ transactionid_as_ints[0]);
          ip_as_ints[2] = (ip_as_ints[2] ^ transactionid_as_ints[1]);
          ip_as_ints[3] = (ip_as_ints[3] ^ transactionid_as_ints[2]);
          return IPAddress(v6addr);
        }
        break;
      }
    }
  }
  // Invalid ip family or transaction ID, or missing owner.
  // Return an AF_UNSPEC address.
  return IPAddress();
}

bool StunXorAddressAttribute::Read(ByteBufferReader* buf) {
  if (!StunAddressAttribute::Read(buf)) return false;
  uint16_t xoredport = port() ^ (kStunMagicCookie >> 16);
  IPAddress xored_ip = GetXoredIP();
  SetAddress(SocketAddress(xored_ip, xoredport));
  return true;
}

bool StunXorAddressAttribute::Write(ByteBufferWriter* buf) const {
  StunAddressFamily address_family = family();
  if (address_family == kStunAddressUndef) {
    LOG(ERROR) << "Error writing xor-address attribute: unknown family.";
    return false;
  }
  IPAddress xored_ip = GetXoredIP();
  if (xored_ip.family() == AF_UNSPEC) {
    return false;
  }
  buf->WriteUInt8(0);
  buf->WriteUInt8(family());
  buf->WriteUInt16(port() ^ (kStunMagicCookie >> 16));
  switch (xored_ip.family()) {
    case AF_INET: {
      in_addr v4addr = xored_ip.ipv4_address();
      buf->WriteBytes(reinterpret_cast<const char*>(&v4addr), sizeof(v4addr));
      break;
    }
    case AF_INET6: {
      in6_addr v6addr = xored_ip.ipv6_address();
      buf->WriteBytes(reinterpret_cast<const char*>(&v6addr), sizeof(v6addr));
      break;
    }
  }
  return true;
}

StunUInt32Attribute::StunUInt32Attribute(uint16_t type, uint32_t value)
    : StunAttribute(type, SIZE), bits_(value) {}

StunUInt32Attribute::StunUInt32Attribute(uint16_t type)
    : StunAttribute(type, SIZE), bits_(0) {}

StunAttributeValueType StunUInt32Attribute::value_type() const {
  return kStunAttrValueUInt32;
}

bool StunUInt32Attribute::GetBit(size_t index) const {
  assert(index < 32);
  return static_cast<bool>((bits_ >> index) & 0x1);
}

void StunUInt32Attribute::SetBit(size_t index, bool value) {
  assert(index < 32);
  bits_ &= ~(1 << index);
  bits_ |= value ? (1 << index) : 0;
}

bool StunUInt32Attribute::Read(ByteBufferReader* buf) {
  if (length() != SIZE || !buf->ReadUInt32(&bits_)) return false;
  return true;
}

bool StunUInt32Attribute::Write(ByteBufferWriter* buf) const {
  buf->WriteUInt32(bits_);
  return true;
}

StunUInt64Attribute::StunUInt64Attribute(uint16_t type, uint64_t value)
    : StunAttribute(type, SIZE), bits_(value) {}

StunUInt64Attribute::StunUInt64Attribute(uint16_t type)
    : StunAttribute(type, SIZE), bits_(0) {}

StunAttributeValueType StunUInt64Attribute::value_type() const {
  return kStunAttrValueUInt64;
}

bool StunUInt64Attribute::Read(ByteBufferReader* buf) {
  if (length() != SIZE || !buf->ReadUInt64(&bits_)) return false;
  return true;
}

bool StunUInt64Attribute::Write(ByteBufferWriter* buf) const {
  buf->WriteUInt64(bits_);
  return true;
}

StunByteStringAttribute::StunByteStringAttribute(uint16_t type)
    : StunAttribute(type, 0), bytes_(NULL) {}

StunByteStringAttribute::StunByteStringAttribute(uint16_t type,
                                                 const std::string& str)
    : StunAttribute(type, 0), bytes_(NULL) {
  CopyBytes(str.c_str(), str.size());
}

StunByteStringAttribute::StunByteStringAttribute(uint16_t type,
                                                 const void* bytes,
                                                 size_t length)
    : StunAttribute(type, 0), bytes_(NULL) {
  CopyBytes(bytes, length);
}

StunByteStringAttribute::StunByteStringAttribute(uint16_t type, uint16_t length)
    : StunAttribute(type, length), bytes_(NULL) {}

StunByteStringAttribute::~StunByteStringAttribute() { delete[] bytes_; }

StunAttributeValueType StunByteStringAttribute::value_type() const {
  return kStunAttrValueByteString;
}

void StunByteStringAttribute::CopyBytes(const char* bytes) {
  CopyBytes(bytes, strlen(bytes));
}

void StunByteStringAttribute::CopyBytes(const void* bytes, size_t length) {
  char* new_bytes = new char[length];
  memcpy(new_bytes, bytes, length);
  SetBytes(new_bytes, length);
}

uint8_t StunByteStringAttribute::GetByte(size_t index) const {
  assert(bytes_ != NULL);
  assert(index < length());
  return static_cast<uint8_t>(bytes_[index]);
}

void StunByteStringAttribute::SetByte(size_t index, uint8_t value) {
  assert(bytes_ != NULL);
  assert(index < length());
  bytes_[index] = value;
}

bool StunByteStringAttribute::Read(ByteBufferReader* buf) {
  bytes_ = new char[length()];
  if (!buf->ReadBytes(bytes_, length())) {
    return false;
  }

  ConsumePadding(buf);
  return true;
}

bool StunByteStringAttribute::Write(ByteBufferWriter* buf) const {
  buf->WriteBytes(bytes_, length());
  WritePadding(buf);
  return true;
}

void StunByteStringAttribute::SetBytes(char* bytes, size_t length) {
  delete[] bytes_;
  bytes_ = bytes;
  SetLength(static_cast<uint16_t>(length));
}

const uint16_t StunErrorCodeAttribute::MIN_SIZE = 4;

StunErrorCodeAttribute::StunErrorCodeAttribute(uint16_t type, int code,
                                               const std::string& reason)
    : StunAttribute(type, 0) {
  SetCode(code);
  SetReason(reason);
}

StunErrorCodeAttribute::StunErrorCodeAttribute(uint16_t type, uint16_t length)
    : StunAttribute(type, length), class_(0), number_(0) {}

StunErrorCodeAttribute::~StunErrorCodeAttribute() {}

StunAttributeValueType StunErrorCodeAttribute::value_type() const {
  return kStunAttrValueErrorCode;
}

int StunErrorCodeAttribute::code() const { return class_ * 100 + number_; }

void StunErrorCodeAttribute::SetCode(int code) {
  class_ = static_cast<uint8_t>(code / 100);
  number_ = static_cast<uint8_t>(code % 100);
}

void StunErrorCodeAttribute::SetReason(const std::string& reason) {
  SetLength(MIN_SIZE + static_cast<uint16_t>(reason.size()));
  reason_ = reason;
}

bool StunErrorCodeAttribute::Read(ByteBufferReader* buf) {
  uint32_t val;
  if (length() < MIN_SIZE || !buf->ReadUInt32(&val)) return false;

  if ((val >> 11) != 0) LOG(ERROR) << "error-code bits not zero";

  class_ = ((val >> 8) & 0x7);
  number_ = (val & 0xff);

  if (!buf->ReadString(&reason_, length() - 4)) return false;

  ConsumePadding(buf);
  return true;
}

bool StunErrorCodeAttribute::Write(ByteBufferWriter* buf) const {
  buf->WriteUInt32(class_ << 8 | number_);
  buf->WriteString(reason_);
  WritePadding(buf);
  return true;
}

StunUInt16ListAttribute::StunUInt16ListAttribute(uint16_t type, uint16_t length)
    : StunAttribute(type, length) {
  attr_types_ = new std::vector<uint16_t>();
}

StunUInt16ListAttribute::~StunUInt16ListAttribute() { delete attr_types_; }

StunAttributeValueType StunUInt16ListAttribute::value_type() const {
  return kStunAttrValueUInt16List;
}

size_t StunUInt16ListAttribute::Size() const { return attr_types_->size(); }

uint16_t StunUInt16ListAttribute::GetType(int index) const {
  return (*attr_types_)[index];
}

void StunUInt16ListAttribute::SetType(int index, uint16_t value) {
  (*attr_types_)[index] = value;
}

void StunUInt16ListAttribute::AddType(uint16_t value) {
  attr_types_->push_back(value);
  SetLength(static_cast<uint16_t>(attr_types_->size() * 2));
}

void StunUInt16ListAttribute::AddTypeAtIndex(uint16_t index, uint16_t value) {
  if (attr_types_->size() < static_cast<size_t>(index + 1)) {
    attr_types_->resize(index + 1);
  }
  (*attr_types_)[index] = value;
  SetLength(static_cast<uint16_t>(attr_types_->size() * 2));
}

bool StunUInt16ListAttribute::Read(ByteBufferReader* buf) {
  if (length() % 2) {
    return false;
  }

  for (size_t i = 0; i < length() / 2; i++) {
    uint16_t attr;
    if (!buf->ReadUInt16(&attr)) return false;
    attr_types_->push_back(attr);
  }
  // Padding of these attributes is done in RFC 5389 style. This is
  // slightly different from RFC3489, but it shouldn't be important.
  // RFC3489 pads out to a 32 bit boundary by duplicating one of the
  // entries in the list (not necessarily the last one - it's unspecified).
  // RFC5389 pads on the end, and the bytes are always ignored.
  ConsumePadding(buf);
  return true;
}

bool StunUInt16ListAttribute::Write(ByteBufferWriter* buf) const {
  for (size_t i = 0; i < attr_types_->size(); ++i) {
    buf->WriteUInt16((*attr_types_)[i]);
  }
  WritePadding(buf);
  return true;
}

static std::unordered_map<uint16_t, std::string> kStunMethodMap = {
    {kStunMessageBindingRequest, "STUN BINDING request"},
    {kStunMessageBindingIndication, "STUN BINDING indication"},
    {kStunMessageBindingResponse, "STUN BINDING response"},
    {kStunMessageBindingErrorResponse, "STUN BINDING error response"},
    {kStunGoodPingRequest, "GOOG PING request"},
    {kStunGoodPingResponse, "GOOG PING response"},
    {kStunGoodPingErrorResponse, "GOOG PING error response"},
    {kStunAllocateRequst, "TURN ALLOCATE request"},
    {kStunAllocateResponse, "TURN ALLOCATE response"},
    {kStunAllocateErrorResponse, "TURN ALLOCATE error respons"},
    {kTurnRefreshRequest, "TURN REFRESH request"},
    {kTurnRefreshResponse, "TURN REFRESH response"},
    {kTurnRefreshErrorResponse, "TURN REFRESH error response"},
    {kTurnSendSendIndication, "TURN SEND INDICATION"},
    {kTurnDataSendIndication, "TURN DATA INDICATION"},
    {kTurnCreatePermissionRequest, "TURN CREATE PERMISSION request"},
    {kTurnCreatePermissionResonse, "TURN CREATE PERMISSION response"},
    {kTurnCreatePermissionErrorResonse,
     "TURN CREATE PERMISSION error response"},
    {kTurnChannelBindRequest, "TURN CHANNEL BIND request"},
    {kTurnChannelBindResponse, "TURN CHANNEL BIND response"},
    {kTurnChannelBindErrorResponse, "TURN CHANNEL BIND error response"},
    {kTurnConnect, "TURN-CHANNEL-BIND"},
    {kTurnConnectionBind, "TURN-CONNECT-BIND"},
    {kTurnConnectionAttempt, "TURN-CONNECTION-ATTEMP"}, };

std::string StunMethodToString(int msg_type) {
  auto it = kStunMethodMap.find(msg_type);
  if (it != kStunMethodMap.end()) {
    return it->second;
  }
  return "UNKNOWN<" + std::to_string(msg_type) + ">";
}

int GetStunSuccessResponseType(int req_type) {
  return IsStunRequestType(req_type) ? (req_type | 0x100) : -1;
}

int GetStunErrorResponseType(int req_type) {
  return IsStunRequestType(req_type) ? (req_type | 0x110) : -1;
}

bool IsStunRequestType(int msg_type) {
  return ((msg_type & kStunTypeMask) == 0x000);
}

bool IsStunIndicationType(int msg_type) {
  return ((msg_type & kStunTypeMask) == 0x010);
}

bool IsStunSuccessResponseType(int msg_type) {
  return ((msg_type & kStunTypeMask) == 0x100);
}

bool IsStunErrorResponseType(int msg_type) {
  return ((msg_type & kStunTypeMask) == 0x110);
}

bool ComputeStunCredentialHash(const std::string& username,
                               const std::string& realm,
                               const std::string& password, std::string* hash) {
  // http://tools.ietf.org/html/rfc5389#section-15.4
  // long-term credentials will be calculated using the key and key is
  // key = MD5(username ":" realm ":" SASLprep(password))
  std::string input = username;
  input += ':';
  input += realm;
  input += ':';
  input += password;

  char digest[MessageDigest::kMaxSize];
  size_t size = ComputeDigest(DIGEST_MD5, input.c_str(), input.size(), digest,
                              sizeof(digest));
  if (size == 0) {
    return false;
  }

  *hash = std::string(digest, size);
  return true;
}

std::unique_ptr<StunAttribute> CopyStunAttribute(
    const StunAttribute& attribute, ByteBufferWriter* tmp_buffer_ptr) {
  ByteBufferWriter tmpBuffer;
  if (tmp_buffer_ptr == nullptr) {
    tmp_buffer_ptr = &tmpBuffer;
  }

  std::unique_ptr<StunAttribute> copy(StunAttribute::Create(
      attribute.value_type(), attribute.type(),
      static_cast<uint16_t>(attribute.length()), nullptr));

  if (!copy) {
    return nullptr;
  }
  tmp_buffer_ptr->Clear();
  if (!attribute.Write(tmp_buffer_ptr)) {
    return nullptr;
  }
  ByteBufferReader reader(*tmp_buffer_ptr);
  if (!copy->Read(&reader)) {
    return nullptr;
  }

  return copy;
}

StunAttributeValueType RelayMessage::GetAttributeValueType(int type) const {
  switch (type) {
    case kStunAttrLifetime:
      return kStunAttrValueUInt32;
    case kStunAttrMagicCookie:
      return kStunAttrValueByteString;
    case kStunAttrBandwidth:
      return kStunAttrValueUInt32;
    case kStunAttrDestinationAddress:
      return kStunAttrValueAddress;
    case kStunAttrSourceAddress2:
      return kStunAttrValueAddress;
    case kStunAttrData:
      return kStunAttrValueByteString;
    case kStunAttrOptions:
      return kStunAttrValueUInt32;
    default:
      return StunMessage::GetAttributeValueType(type);
  }
}

StunMessage* RelayMessage::CreateNew() const { return new RelayMessage(); }

StunAttributeValueType TurnMessage::GetAttributeValueType(int type) const {
  switch (type) {
    case kStunAttrChannelNumber:
      return kStunAttrValueUInt32;
    case kStunAttrLifetime:
      return kStunAttrValueUInt32;
    case kStunAttrXorPeerAddress:
      return kStunAttrValueXorAddress;
    case kStunAttrData:
      return kStunAttrValueByteString;
    case kStunAttrXorRelayedAddress:
      return kStunAttrValueXorAddress;
    case kStunAttrEventPort:
      return kStunAttrValueByteString;
    case kStunAttrRequestedTransport:
      return kStunAttrValueUInt32;
    case kStunAttrDontFragment:
      return kStunAttrValueByteString;
    case kStunAttrReservationToken:
      return kStunAttrValueByteString;
    default:
      return StunMessage::GetAttributeValueType(type);
  }
}

StunMessage* TurnMessage::CreateNew() const { return new TurnMessage(); }

StunAttributeValueType IceMessage::GetAttributeValueType(int type) const {
  switch (type) {
    case kStunAttrICEPriority:
    case kStunAttrICENetworkInfo:
    case kStunAttrICENomination:
      return kStunAttrValueUInt32;
    case kStunAttrICEUseCandidate:
      return kStunAttrValueByteString;
    case kStunAttrICEControlled:
      return kStunAttrValueUInt64;
    case kStunAttrICEControlling:
      return kStunAttrValueUInt64;
    default:
      return StunMessage::GetAttributeValueType(type);
  }
}

StunMessage* IceMessage::CreateNew() const { return new IceMessage(); }

std::unique_ptr<StunMessage> StunMessage::Clone() const {
  std::unique_ptr<StunMessage> copy(CreateNew());
  if (!copy) {
    return nullptr;
  }
  ByteBufferWriter buf;
  if (!Write(&buf)) {
    return nullptr;
  }
  ByteBufferReader reader(buf);
  if (!copy->Read(&reader)) {
    return nullptr;
  }
  return copy;
}
}