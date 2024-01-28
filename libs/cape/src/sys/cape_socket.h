#ifndef __CAPE_SYS__SOCKET__H
#define __CAPE_SYS__SOCKET__H 1

#include "sys/cape_export.h"
#include "sys/cape_err.h"
#include "stc/cape_str.h"

//=============================================================================

__CAPE_LIBEX   void*         cape_sock__tcp__clt_new      (const char* host, long port, CapeErr err);
 
__CAPE_LIBEX   void*         cape_sock__tcp__srv_new      (const char* host, long port, CapeErr err);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   void*         cape_sock__udp__clt_new      (const char* host, long port, CapeErr err);

__CAPE_LIBEX   void*         cape_sock__udp__srv_new      (const char* host, long port, CapeErr err);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   void*         cape_sock__icmp__new         (CapeErr err);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   void          cape_sock__close             (void*);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   CapeString    cape_net__resolve            (const CapeString host, int ipv6, CapeErr err);

//-----------------------------------------------------------------------------

#endif
