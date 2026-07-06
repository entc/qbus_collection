#ifndef __CAPE_SYS__NET__H
#define __CAPE_SYS__NET__H 1

#include "sys/cape_export.h"
#include "sys/cape_err.h"
#include "stc/cape_str.h"

//-----------------------------------------------------------------------------

                                  /* constructor */
__CAPE_LIBEX   struct addrinfo*   cape_net__new                (int flags, int family, int socktype, int protocol, void* addr_buf, number_t addr_len, const CapeString canonname);

                                  /* constructor with host and port conversion */
__CAPE_LIBEX   struct addrinfo*   cape_net__new_simple         (int flags, int family, int socktype, int protocol, const CapeString host, int port, const CapeString canonname);

                                  /* constructor with hostname resolution */
__CAPE_LIBEX   struct addrinfo*   cape_net__resolve_os         (const CapeString host, int port, int ipv6, CapeErr err);

                                  /* destructor */
__CAPE_LIBEX   void               cape_net__resolve_del        (struct addrinfo**);

                                  /* this function logs the content of the addrinfo, for debug purposes */
__CAPE_LIBEX   void               cape_net__print              (struct addrinfo*);

//-----------------------------------------------------------------------------

                                  /* resolves a host into an IP address */
__CAPE_LIBEX   CapeString         cape_net__resolve            (const CapeString host, int ipv6, CapeErr err);

                                  /* this function is only implemented for windows to initialize WSA */
__CAPE_LIBEX   int                cape_net__init               (void);

//-----------------------------------------------------------------------------

#endif
