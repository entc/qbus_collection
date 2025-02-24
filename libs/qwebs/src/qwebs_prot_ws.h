#ifndef __QWEBS_PROTOCOL_WEBSOCKET__H
#define __QWEBS_PROTOCOL_WEBSOCKET__H 1

#include "qwebs_structs.h"

// cape includes
#include "sys/cape_export.h"
#include "sys/cape_types.h"
#include "sys/cape_err.h"
#include "stc/cape_str.h"
#include "stc/cape_map.h"
#include "stc/cape_stream.h"

//-----------------------------------------------------------------------------

struct QWebsProtWebsocket_s; typedef struct QWebsProtWebsocket_s* QWebsProtWebsocket;
struct QWebsProtWebsocketConnection_s; typedef struct QWebsProtWebsocketConnection_s* QWebsProtWebsocketConnection;

//-----------------------------------------------------------------------------

__CAPE_LIBEX   QWebsProtWebsocket   qwebs_prot_websocket_new        ();

__CAPE_LIBEX   void                 qwebs_prot_websocket_del        (QWebsProtWebsocket*);

                                    /* register this websockets implementation as http upgrade */
__CAPE_LIBEX   int                  qwebs_prot_websocket_reg        (QWebsProtWebsocket, QWebs, CapeErr err);

__CAPE_LIBEX   void                 qwebs_prot_websocket_dec        (QWebsProtWebsocket*, void** p_user_ptr);

//-----------------------------------------------------------------------------

               /* called if a new connection was established, return void* as conn_ptr */
typedef void*  (__STDCALL *fct_qwebs_prot_websocket__on_conn)        (void* user_ptr, QWebsProtWebsocketConnection connection, CapeMap query);

               /* called after upgrade was done */
typedef void   (__STDCALL *fct_qwebs_prot_websocket__on_init)        (void* conn_ptr);

               /* a new message was received */
typedef void   (__STDCALL *fct_qwebs_prot_websocket__on_msg)         (void* conn_ptr, CapeString message);

               /* called after upgrade was done */
typedef void   (__STDCALL *fct_qwebs_prot_websocket__on_done)        (void* conn_ptr);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   void                 qwebs_prot_websocket_cb         (QWebsProtWebsocket, void* user_ptr, fct_qwebs_prot_websocket__on_conn, fct_qwebs_prot_websocket__on_init, fct_qwebs_prot_websocket__on_msg, fct_qwebs_prot_websocket__on_done);

__CAPE_LIBEX   void                 qwebs_prot_websocket_send_s     (QWebsProtWebsocketConnection connection, const CapeString message);

__CAPE_LIBEX   void                 qwebs_prot_websocket_send_buf   (QWebsProtWebsocketConnection connection, const char* bufdat, number_t buflen);

//-----------------------------------------------------------------------------

#endif
