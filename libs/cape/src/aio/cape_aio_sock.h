#ifndef __CAPE_AIO__SOCK__H
#define __CAPE_AIO__SOCK__H 1

//=============================================================================

#include "sys/cape_export.h"
#include "sys/cape_err.h"
#include "aio/cape_aio_ctx.h"
#include "stc/cape_stream.h"

#include <sys/types.h>

//=============================================================================

/*
 * \ brief This class implements the socket handler for the AIO subsystem. The constructor takes a handle (socket file descriptor) and registers
           it on the AIO subsystem. All events on the socket can be catched by the callback functions. The socket can react on recv and write events.
           The write events notification will be turned off automatically after a write event got catched. This must be turned on 
           with 'cape_aio_socket_markSent' again. Write event should be considered as ask the system if the write buffer is ready?
           
           This class is referenced counted. If you need a private copy, please use the 'cape_aio_socket_inref' function to ensure 
           the socket will not be deleted in the meanwhile. The 'cape_aio_socket_unref' releases the ownership.
           
           By using the function 'cape_aio_socket_listen' the ownership will move to the AIO subsystem. The object will be released there. If you
           use only the callback functions, it is safe to access the object everytime.
 */

//-----------------------------------------------------------------------------

struct CapeAioSocket_s; typedef struct CapeAioSocket_s* CapeAioSocket;

//-----------------------------------------------------------------------------

__CAPE_LIBEX   CapeAioSocket     cape_aio_socket_new           (void* handle);                           ///< constructor to allocate memory for the object

__CAPE_LIBEX   void              cape_aio_socket_inref         (CapeAioSocket);                          ///< increase the reference counter of the object

__CAPE_LIBEX   void              cape_aio_socket_unref         (CapeAioSocket);                          ///< decrease the reference counter of the object

//-----------------------------------------------------------------------------

__CAPE_LIBEX   void              cape_aio_socket_add           (CapeAioSocket*, CapeAioContext);         ///< turn on events on the socket

__CAPE_LIBEX   void              cape_aio_socket_add_w         (CapeAioSocket*, CapeAioContext);         ///< turn on write events on the socket

__CAPE_LIBEX   void              cape_aio_socket_add_r         (CapeAioSocket*, CapeAioContext);         ///< turn on read events on the socket

__CAPE_LIBEX   void              cape_aio_socket_add_b         (CapeAioSocket*, CapeAioContext);         ///< turn on write / read events on the socket

__CAPE_LIBEX   void              cape_aio_socket_change_w      (CapeAioSocket, CapeAioContext);         ///< turn on read events

__CAPE_LIBEX   void              cape_aio_socket_change_r      (CapeAioSocket, CapeAioContext);         ///< turn on write events

__CAPE_LIBEX   void              cape_aio_socket_close         (CapeAioSocket, CapeAioContext);          ///< turn off all events and disconnect

//-----------------------------------------------------------------------------
// **** DEPRICATED METHODS ****

__CAPE_LIBEX   void              cape_aio_socket_markSent      (CapeAioSocket, CapeAioContext);          ///< turn on 'write' events on the socket

/* depricated */
__CAPE_LIBEX   void              cape_aio_socket_listen        (CapeAioSocket*, CapeAioContext);         ///< turn on 'receive' events on the socket

//-----------------------------------------------------------------------------

typedef void       (__STDCALL *fct_cape_aio_socket_onSent)     (void* ptr, CapeAioSocket socket, void* userdata);
typedef void       (__STDCALL *fct_cape_aio_socket_onRecv)     (void* ptr, CapeAioSocket socket, const char* bufdat, number_t buflen);
typedef void       (__STDCALL *fct_cape_aio_socket_onDone)     (void* ptr, void* userdata);

__CAPE_LIBEX   void                 cape_aio_socket_callback       (CapeAioSocket, void*, fct_cape_aio_socket_onSent, fct_cape_aio_socket_onRecv, fct_cape_aio_socket_onDone);

//-----------------------------------------------------------------------------

// WARNING: can only be used in the onSent callback function, to avoid race-conditions
__CAPE_LIBEX   void                 cape_aio_socket_send           (CapeAioSocket, CapeAioContext, const char* bufdata, unsigned long buflen, void* userdata);   

//=============================================================================

struct CapeAioAccept_s; typedef struct CapeAioAccept_s* CapeAioAccept;

//-----------------------------------------------------------------------------

__CAPE_LIBEX   CapeAioAccept        cape_aio_accept_new            (void* handle);                       ///< constructor to allocate memory for the object

__CAPE_LIBEX   void                 cape_aio_accept_del            (CapeAioAccept*);                     ///< destructor to free memory

//-----------------------------------------------------------------------------

typedef void       (__STDCALL *fct_cape_aio_accept_onConnect)  (void* ptr, void* handle, const char* remote_host);
typedef void       (__STDCALL *fct_cape_aio_accept_onDone)     (void* ptr);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   void                 cape_aio_accept_callback       (CapeAioAccept, void*, fct_cape_aio_accept_onConnect, fct_cape_aio_accept_onDone);

__CAPE_LIBEX   void                 cape_aio_accept_add            (CapeAioAccept*, CapeAioContext);     ///< turn on 'accept' events, ownership moves to AioContext

//=============================================================================

struct CapeAioSocketUdp_s; typedef struct CapeAioSocketUdp_s* CapeAioSocketUdp;

__CAPE_LIBEX   CapeAioSocketUdp     cape_aio_socket__udp__new      (void* handle);                       ///< constructor to allocate memory for the object

__CAPE_LIBEX   void                 cape_aio_socket__upd__del      (CapeAioSocketUdp*);                  ///< destructor to free memory

__CAPE_LIBEX   void                 cape_aio_socket__udp__add      (CapeAioSocketUdp*, CapeAioContext, int mode);       ///< turn on events on the socket

__CAPE_LIBEX   void                 cape_aio_socket__udp__set      (CapeAioSocketUdp, CapeAioContext, int mode);        ///< turn on events on the socket

__CAPE_LIBEX   void                 cape_aio_socket__udp__rm       (CapeAioSocketUdp, CapeAioContext);                  ///< remove all events

typedef void       (__STDCALL *fct_cape_aio_socket__on_sent_ready)    (void* ptr, CapeAioSocketUdp, void* userdata);
typedef void       (__STDCALL *fct_cape_aio_socket__on_recv_from)     (void* ptr, CapeAioSocketUdp, const char* bufdat, number_t buflen, const char* host);

__CAPE_LIBEX   void                 cape_aio_socket__udp__cb       (CapeAioSocketUdp, void* ptr, fct_cape_aio_socket__on_sent_ready on_send, fct_cape_aio_socket__on_recv_from on_recv, fct_cape_aio_socket_onDone on_done);                       ///< set callbacks

__CAPE_LIBEX   void                 cape_aio_socket__udp__send     (CapeAioSocketUdp, CapeAioContext, const char* bufdat, unsigned long buflen, void* userdata, const char* host, number_t port);                       

//=============================================================================

struct CapeAioSocketIcmp_s; typedef struct CapeAioSocketIcmp_s* CapeAioSocketIcmp;

__CAPE_LIBEX   CapeAioSocketIcmp    cape_aio_socket__icmp__new     (void* handle);                       ///< constructor to allocate memory for the object

__CAPE_LIBEX   void                 cape_aio_socket__icmp__del     (CapeAioSocketIcmp*);                  ///< destructor to free memory

__CAPE_LIBEX   void                 cape_aio_socket__icmp__add     (CapeAioSocketIcmp*, CapeAioContext);       ///< turn on events on the socket

typedef void       (__STDCALL *fct_cape_aio_socket__on_pong)       (void* ptr, CapeAioSocketIcmp, number_t ms_second, int timeout);

__CAPE_LIBEX   void                 cape_aio_socket__icmp__cb      (CapeAioSocketIcmp, void* ptr, fct_cape_aio_socket__on_pong on_pong, fct_cape_aio_socket_onDone on_done);                       ///< set callbacks

__CAPE_LIBEX   void                 cape_aio_socket__icmp__ping    (CapeAioSocketIcmp, CapeAioContext, const char* host, double timeout_in_ms);                       

//=============================================================================

/*
 \ brief This class implements a cached sendbuffer for the AIO socket subsystem. If you are not sure how to use the CapeAioSocket class
           please consider CapeAioSocketCache instead. 
*/

//-----------------------------------------------------------------------------

struct CapeAioSocketCache_s; typedef struct CapeAioSocketCache_s* CapeAioSocketCache;

// callback prototypes
typedef void  (__STDCALL *fct_cape_aio_socket_cache__on_recv)      (void* ptr, const char* bufdat, number_t buflen);
typedef void  (__STDCALL *fct_cape_aio_socket_cache__on_event)     (void* ptr);


//-----------------------------------------------------------------------------

__CAPE_LIBEX  CapeAioSocketCache  cape_aio_socket_cache_new     (CapeAioContext);                                           ///< constructor to allocate memory for the object

__CAPE_LIBEX  void                cape_aio_socket_cache_del     (CapeAioSocketCache*);                                      ///< destructor to free memory

__CAPE_LIBEX  void                cape_aio_socket_cache_set     (CapeAioSocketCache, void* handle, void* ptr, fct_cape_aio_socket_cache__on_recv, fct_cape_aio_socket_cache__on_event on_retry, fct_cape_aio_socket_cache__on_event on_connect);

__CAPE_LIBEX  void                cape_aio_socket_cache_clr     (CapeAioSocketCache);                                       ///< stops all operations (disconnect)

__CAPE_LIBEX  int                 cape_aio_socket_cache_send_s  (CapeAioSocketCache, CapeStream* p_stream, CapeErr err);    ///< sends the content of the stream to the socket

__CAPE_LIBEX  void                cape_aio_socket_cache_retry   (CapeAioSocketCache, int auto_reconnect);

__CAPE_LIBEX  int                 cape_aio_socket_cache_active  (CapeAioSocketCache);                                       ///< returns true if connection is active

//-----------------------------------------------------------------------------

                                  /* send the stream as smaller packets -> use this to simulate real TCP connections */
__CAPE_LIBEX  int                 cape_aio_socket_cache_simp    (CapeAioSocketCache, CapeStream* p_stream, number_t block_size, number_t delay, CapeErr err);

//=============================================================================

#endif
