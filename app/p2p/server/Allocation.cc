#include "Server.h"


#define MAX_CLIENT_BANDWIDTH    128  /* In Kbps */
#define DEFA_CLIENT_BANDWIDTH	  64

#define MIN_LIFETIME    30
#define MAX_LIFETIME    600
#define DEF_LIFETIME    300

/**
 * Maximum length of text representation of an IPv6 address.
 */
#define INET6_ADDRSTRLEN	46

/* Parsed Allocation request. */
struct AllocRequest {
    uint32_t  tpType_;  	    /* Requested transport  */
    char      addr_[INET6_ADDRSTRLEN];  /* Requested IP	    */
    uint32_t bandWidth_;      /* Requested bandwidth  */
    uint32_t lifeTime_;       /* Lifetime.	    */
    uint32_t rppBits_;        /* A bits		    */
    uint32_t rppPort_;        /* Requested port	    */
};