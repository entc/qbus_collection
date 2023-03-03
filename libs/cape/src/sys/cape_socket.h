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

                             /* close the handle / socket */
__CAPE_LIBEX   void          cape_sock__close             (void*);

                             /* set the handle / socket to none blocking */
__CAPE_LIBEX   int           cape_sock__noneblocking      (void*, CapeErr);

                             /* resolve IP-Address from a named host */
__CAPE_LIBEX   CapeString    cape_sock__resolve           (const CapeString host, CapeErr err);

//-----------------------------------------------------------------------------

#endif
