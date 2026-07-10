#ifndef __CAPE_SYS__SOCKET__H
#define __CAPE_SYS__SOCKET__H 1

#include "sys/cape_export.h"
#include "sys/cape_err.h"
#include "sys/cape_net.h"
#include "stc/cape_str.h"
#include "stc/cape_stream.h"

//=============================================================================

__CAPE_LIBEX   void*         cape_sock__tcp__clt_new      (const char* host, long port, CapeErr err);
 
__CAPE_LIBEX   void*         cape_sock__tcp__srv_new      (const char* host, long port, CapeErr err);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   void*         cape_sock__udp__clt_new      (CapeErr err);

__CAPE_LIBEX   void*         cape_sock__udp__srv_new      (const char* host, long port, CapeErr err);

__CAPE_LIBEX   int           cape_sock__udp__send_to      (void* handle, CapeStream buf, const char* host, long port, CapeErr err);

__CAPE_LIBEX   int           cape_sock__udp__send_to_nr   (void* handle, CapeStream buf, struct addrinfo*, CapeErr err);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   void*         cape_sock__icmp__new         (CapeErr err);

//-----------------------------------------------------------------------------

                             /* accepts connections and returns new handle */
__CAPE_LIBEX   void*         cape_sock__accept            (void*, CapeString* p_remote_addr, CapeErr err);

                             /* reads data from the socket, buflen defines the maximum size of data */
__CAPE_LIBEX   int           cape_sock__recv              (void*, CapeStream bufdat, number_t buflen, CapeErr err);

                             /* sends data to the socket, bufdat contains all data */
__CAPE_LIBEX   int           cape_sock__send              (void*, CapeStream bufdat, CapeErr err);

                             /* sends no data to the socket */
__CAPE_LIBEX   int           cape_sock__touch             (void*, CapeErr err);

__CAPE_LIBEX   void          cape_sock__close             (void*);

                             /* checks if socket is coneccted, returns cape_err result */
__CAPE_LIBEX   int           cape_sock__status            (void*, CapeErr err);

__CAPE_LIBEX   void          cape_sock__shutdown          (void*);

__CAPE_LIBEX   void          cape_sock__shutdown__rd      (void*);

__CAPE_LIBEX   void          cape_sock__shutdown__wr      (void*);

__CAPE_LIBEX   int           cape_sock__noneblocking      (void*, CapeErr err);

//-----------------------------------------------------------------------------

#endif
