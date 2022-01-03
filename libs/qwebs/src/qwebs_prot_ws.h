#ifndef __QWEBS_PROTOCOL_WEBSOCKET__H
#define __QWEBS_PROTOCOL_WEBSOCKET__H 1

#include "qwebs_structs.h"

// cape includes
#include "sys/cape_export.h"
#include "sys/cape_types.h"
#include "sys/cape_err.h"
#include "stc/cape_str.h"
#include "stc/cape_stream.h"

//-----------------------------------------------------------------------------

struct QWebsProtWebsocket_s; typedef struct QWebsProtWebsocket_s* QWebsProtWebsocket;

//-----------------------------------------------------------------------------

__CAPE_LIBEX   QWebsProtWebsocket    qwebs_prot_websocket_new    (QWebs webs);

__CAPE_LIBEX   int                   qwebs_prot_websocket_init   (QWebsProtWebsocket, CapeErr err);

//-----------------------------------------------------------------------------

               /* called if a new connection was established, return void* as conn_ptr */
typedef void*  (__STDCALL *fct_qwebs_prot_websocket__on_conn)    (void* user_ptr);

               /* a new message was received */
typedef void   (__STDCALL *fct_qwebs_prot_websocket__on_msg)     (void* conn_ptr, CapeString message);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   void                  qwebs_prot_websocket_cb     (QWebsProtWebsocket, void* user_ptr, fct_qwebs_prot_websocket__on_conn, fct_qwebs_prot_websocket__on_msg);

//-----------------------------------------------------------------------------

#endif
