#ifndef __CAPE_SYS__NET__H
#define __CAPE_SYS__NET__H 1

#include "sys/cape_export.h"
#include "sys/cape_err.h"
#include "stc/cape_str.h"

//-----------------------------------------------------------------------------

typedef struct sockaddr_in* CapeSockaddr;

//-----------------------------------------------------------------------------

__CAPE_LIBEX   CapeSockaddr   cape_net__resolve_os         (const CapeString host, u_short port, int ipv6, CapeErr err);

__CAPE_LIBEX   void           cape_net__resolve_del        (CapeSockaddr*);

__CAPE_LIBEX   CapeString     cape_net__resolve            (const CapeString host, int ipv6, CapeErr err);

//-----------------------------------------------------------------------------

#endif
