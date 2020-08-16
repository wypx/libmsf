#include <base/Logger.h>
#include "Stun.h"

using namespace MSF::BASE;
using namespace MSF::P2P;



std::string stunClassName(const enum StunClass classi)
{
  switch (classi) {
    case STUN_CLASS_REQUEST:      return "Request";
    case STUN_CLASS_INDICATION:   return "Indication";
    case STUN_CLASS_SUCCESS_RESP: return "Success Response";
    case STUN_CLASS_ERROR_RESP:   return "Error Response";
    default:                      return "Unkown class";
  }
}


const char *stunMethodName(const enum StunMethod method)
{
  switch (method) {
    case STUN_METHOD_BINDING:    return "Binding";
    case STUN_METHOD_ALLOCATE:   return "Allocate";
    case STUN_METHOD_REFRESH:    return "Refresh";
    case STUN_METHOD_SEND:       return "Send";
    case STUN_METHOD_DATA:       return "Data";
    case STUN_METHOD_CREATEPERM: return "CreatePermission";
    case STUN_METHOD_CHANBIND:   return "ChannelBind";
    default:                     return "Unkown method";
  }
}

void Stun::debugNatType(const enum StunNatType type) {
  switch (type) {
    case StunTypeOpen:
      LOG(INFO) << "No NAT detected - P2P should work";
      break;
    case StunTypeConeNat:
      LOG(INFO) << "NAT type: Full Cone Nat detect - P2P will work with STUN";
      break;
    case StunTypeRestrictedNat:
      LOG(INFO) << "NAT type: Address restricted - P2P will work with STUN";
      break;
    case StunTypePortRestrictedNat:
      LOG(INFO) << "NAT type: Port restricted - P2P will work with STUN";
      break;
    case StunTypeSymNat:
      LOG(INFO) << "NAT type: Symetric - P2P will NOT work";
      break;
    case StunTypeBlocked:
      LOG(INFO) << "Could not reach the stun server - check server name is correct";
      break;
    case StunTypeFailure:
      LOG(INFO) << "Local network is not work!";
      break;
    default:
      LOG(INFO) << "Unkown NAT type";
      break;
  }
}
const enum StunNatType Stun::natType() const {
  return StunTypeConeNat;
}

