#ifndef __QBUS_TYPES__H
#define __QBUS_TYPES__H 1

// cape includes
#include <sys/cape_err.h>
#include <stc/cape_udc.h>
#include <aio/cape_aio_ctx.h>
#include <aio/cape_aio_sock.h>

//-----------------------------------------------------------------------------

struct QBus_s; typedef struct QBus_s* QBus; // use a simple version

//-----------------------------------------------------------------------------

struct QBusMessage_s
{
  number_t mtype;
  
  CapeUdc clist;    // list of all parameters
  
  CapeUdc cdata;    // public object as parameters
  
  CapeUdc pdata;    // private object as parameters
  
  CapeUdc rinfo;
  
  CapeUdc files;    // if the content is too big, payload is stored in temporary files
  
  CapeStream blob;  // binary blob within the CapeStream
  
  CapeErr err;
  
  CapeString module_ident;  // don't change
  
  CapeString method_ident;  // don't change
  
}; typedef struct QBusMessage_s* QBusM;

//-----------------------------------------------------------------------------
// object typedefs and definitions

typedef struct QbusPvdCtx_s* QbusPvdCtx;
typedef struct QbusPvdEntity_s* QbusPvdEntity;
typedef struct QbusPvdConnection_s* QbusPvdConnection;

//-----------------------------------------------------------------------------

                       /* factory callback to create an user defined object to handle the upper layer logic */
typedef void*          (__STDCALL *fct_qbus_pvd_factory__on_new)   (void* user_ptr, QbusPvdConnection connection);

                       /* factory callback to release the upper layer object */
typedef void           (__STDCALL *fct_qbus_pvd_factory__on_del)   (void** object_ptr);

                       /* callback after a new connection was established */
typedef void           (__STDCALL *fct_qbus_pvd_cb__on_connection) (void* object_ptr);

//-----------------------------------------------------------------------------
// library functions

typedef int            (__STDCALL *fct_qbus_pvd_init)         (CapeErr);

typedef void           (__STDCALL *fct_qbus_pvd_done)         (void);

//-----------------------------------------------------------------------------
// context functions

typedef QbusPvdCtx     (__STDCALL *fct_qbus_pvd_ctx_new)      (CapeAioContext, CapeUdc, CapeErr);

typedef void           (__STDCALL *fct_qbus_pvd_ctx_del)      (QbusPvdCtx*);

typedef QbusPvdEntity  (__STDCALL *fct_qbus_pvd_ctx_add)      (QbusPvdCtx, CapeUdc options, void* user_ptr, fct_qbus_pvd_factory__on_new, fct_qbus_pvd_factory__on_del, fct_qbus_pvd_cb__on_connection);

//-----------------------------------------------------------------------------
// entity functions

typedef void           (__STDCALL *fct_qbus_pvd_ctx_rm)       (QbusPvdCtx, QbusPvdEntity);

typedef int            (__STDCALL *fct_qbus_pvd_listen)       (QbusPvdEntity, CapeErr);

typedef int            (__STDCALL *fct_qbus_pvd_reconnect)    (QbusPvdEntity, CapeErr);

//-----------------------------------------------------------------------------
// connection functions

typedef void           (__STDCALL *fct_qbus_pvd_send)         (QbusPvdConnection, const char* bufdat, number_t buflen, void* userdata);

typedef void           (__STDCALL *fct_qbus_pvd_mark)         (QbusPvdConnection);

typedef void           (__STDCALL *fct_qbus_pvd_cb_raw_set)   (QbusPvdConnection, fct_cape_aio_socket_onSent, fct_cape_aio_socket_onRecv);

//-----------------------------------------------------------------------------

typedef struct
{
  fct_qbus_pvd_init            pvd_init;
  fct_qbus_pvd_done            pvd_done;

  fct_qbus_pvd_ctx_new         pvd_ctx_new;
  fct_qbus_pvd_ctx_del         pvd_ctx_del;
  fct_qbus_pvd_ctx_add         pvd_ctx_add;
  fct_qbus_pvd_ctx_rm          pvd_ctx_rm;
  fct_qbus_pvd_listen          pvd_listen;
  fct_qbus_pvd_reconnect       pvd_reconnect;

  fct_qbus_pvd_send            pvd_send;
  fct_qbus_pvd_mark            pvd_mark;
  fct_qbus_pvd_cb_raw_set      pvd_cb_raw_set;
  
} QbusPvd;

//-----------------------------------------------------------------------------

typedef struct QbusLogCtx_s* QbusLogCtx;

//-----------------------------------------------------------------------------

typedef QbusLogCtx     (__STDCALL *fct_qbus_log_dst_new)      (CapeAioContext, CapeUdc, CapeErr);

typedef void           (__STDCALL *fct_qbus_log_dst_del)      (QbusLogCtx*);

typedef void           (__STDCALL *fct_qbus_log_dst_msg)      (QbusLogCtx, const CapeString remote, const CapeString message);

//-----------------------------------------------------------------------------

typedef struct
{
  fct_qbus_log_dst_new         log_dst_new;
  fct_qbus_log_dst_del         log_dst_del;
  fct_qbus_log_dst_msg         log_dst_msg;
  
} QbusLogDst;


#endif
