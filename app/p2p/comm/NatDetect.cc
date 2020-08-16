#include "NatDetect.h"

using namespace MSF::P2P;

static const std::string g_NatTypeName[] =
{
    "Unknown",
    "ErrUnknown",
    "Open",
    "Blocked",
    "Full Cone",
    "Symmetric UDP",
    "Symmetric",
    "Restricted",
    "Port Restricted"
};

std::string GetNatTypeName(enum StunNatType natType) {
  return g_NatTypeName[natType];
}