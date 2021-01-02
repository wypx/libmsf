#ifndef LIB_DSCP_H_
#define LIB_DSCP_H_

namespace MSF {

// Differentiated Services Code Point.
// See http://tools.ietf.org/html/rfc2474 for details.
enum DiffServCodePoint {
  DSCP_NO_CHANGE = -1,
  DSCP_DEFAULT = 0,  // Same as DSCP_CS0
  DSCP_CS0 = 0,      // The default
  DSCP_CS1 = 8,      // Bulk/background traffic
  DSCP_AF11 = 10,
  DSCP_AF12 = 12,
  DSCP_AF13 = 14,
  DSCP_CS2 = 16,
  DSCP_AF21 = 18,
  DSCP_AF22 = 20,
  DSCP_AF23 = 22,
  DSCP_CS3 = 24,
  DSCP_AF31 = 26,
  DSCP_AF32 = 28,
  DSCP_AF33 = 30,
  DSCP_CS4 = 32,
  DSCP_AF41 = 34,  // Video
  DSCP_AF42 = 36,  // Video
  DSCP_AF43 = 38,  // Video
  DSCP_CS5 = 40,   // Video
  DSCP_EF = 46,    // Voice
  DSCP_CS6 = 48,   // Voice
  DSCP_CS7 = 56,   // Control messages
};

}  // namespace MSF

#endif  //__RTCBASE_DSCP_H_
