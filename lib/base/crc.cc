#include "crc.h"

using namespace MSF;

namespace MSF {

CRCFunction g_Crc32;

uint32_t Crc32(uint8_t *chunk, size_t len) {
  if (g_Crc32)
    return g_Crc32(chunk, len);
  else
    return checksum_sw(chunk, len);
}
}