#include "qbus_tcp.h"

// cape includes
#include "sys/cape_types.h"
#include "sys/cape_socket.h"
#include "sys/cape_log.h"
#include "sys/cape_mutex.h"
#include "stc/cape_stream.h"
#include "fmt/cape_json.h"
#include <aio/cape_aio_timer.h>
#include <aio/cape_aio_sock.h>

#include <stdio.h>

//-----------------------------------------------------------------------------

#if defined __WINDOWS_OS

#include <windows.h>

// fix for linking mariadb library
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "shlwapi.lib")

//------------------------------------------------------------------------------------------------------

// this might work?
/*
extern "C" BOOL WINAPI DllMain (HINSTANCE const instance, DWORD const reason, LPVOID const reserved)
{
  // Perform actions based on the reason for calling.
  switch (reason)
  {
    case DLL_PROCESS_ATTACH:
    {
      cape_log_msg (CAPE_LL_DEBUG, "ADBL", "load library", "MYSQL INIT");
      
      mysql_library_init (0, NULL, NULL);  
      
      break;
    }      
    case DLL_THREAD_ATTACH:
    {
      
      break;
    }      
    case DLL_THREAD_DETACH:
    {
      
      break;
    }      
    case DLL_PROCESS_DETACH:
    {
      cape_log_msg (CAPE_LL_DEBUG, "ADBL", "unload library", "MYSQL DONE");
      
      mysql_thread_end ();
      
      mysql_library_end ();
      
      break;
    }
  }
  
  return TRUE;  // Successful DLL_PROCESS_ATTACH.
}
*/
//------------------------------------------------------------------------------------------------------

#else

//------------------------------------------------------------------------------------------------------

void __attribute__ ((constructor)) library_init (void)
{
}

//------------------------------------------------------------------------------------------------------

void __attribute__ ((destructor)) library_fini (void)
{
}

#endif

//------------------------------------------------------------------------------------------------------

struct QbusPvdCtx_s
{
  CapeAioContext aio;
  CapeList connections;
};

//------------------------------------------------------------------------------------------------------

CapeAioContext qbus_pvd_conn_aio (QbusPvdEntity self);
void qbus_pvd_internal__on_done (QbusPvdEntity self, void** p_user_object);

//------------------------------------------------------------------------------------------------------

struct QbusPvdConnection_s
{
  QbusPvdEntity conn;     // reference
  CapeAioSocket aio_socket;
  
  void* user_object;
  fct_cape_aio_socket_onSent on_sent;
  fct_cape_aio_socket_onRecv on_recv;
  
};

//------------------------------------------------------------------------------------------------------

QbusPvdConnection qbus_pvd_phy_connection_new (QbusPvdEntity conn)
{
  QbusPvdConnection self = CAPE_NEW (struct QbusPvdConnection_s);
  
  self->conn = conn;
  self->aio_socket = NULL;

  self->user_object = NULL;
  self->on_sent = NULL;
  self->on_recv = NULL;
  
  return self;
}

//------------------------------------------------------------------------------------------------------

void qbus_pvd_phy_connection_del (QbusPvdConnection* p_self)
{
  if (*p_self)
  {
    QbusPvdConnection self = *p_self;
    
    if (self->aio_socket)
    {
      // to be gentle: close the socket
      cape_aio_socket_close (self->aio_socket, qbus_pvd_conn_aio (self->conn));
    }
    
    // call the destruction methods related to the user object
    qbus_pvd_internal__on_done (self->conn, &(self->user_object));
    
    CAPE_DEL (p_self, struct QbusPvdConnection_s);
  }
}

//------------------------------------------------------------------------------------------------------

void __STDCALL qbus_pvd_send (QbusPvdConnection self, const char* bufdat, number_t buflen, void* userdata)
{
  cape_aio_socket_send (self->aio_socket, qbus_pvd_conn_aio (self->conn), bufdat, buflen, userdata);
}

//------------------------------------------------------------------------------------------------------

void __STDCALL qbus_pvd_mark (QbusPvdConnection self)
{
  cape_aio_socket_markSent (self->aio_socket, qbus_pvd_conn_aio (self->conn));
}

//------------------------------------------------------------------------------------------------------

void __STDCALL qbus_pvd_cb_raw_set (QbusPvdConnection self, fct_cape_aio_socket_onSent on_sent, fct_cape_aio_socket_onRecv on_recv)
{
  self->on_sent = on_sent;
  self->on_recv = on_recv;
}

//------------------------------------------------------------------------------------------------------

struct QbusPvdEntity_s
{
  QbusPvdCtx ctx;        // reference
  
  void* user_data;
  fct_qbus_pvd_factory__on_new factory_on_new;
  fct_qbus_pvd_factory__on_del factory_on_del;
  fct_qbus_pvd_cb__on_connection on_connection_established;
  
  CapeString host;
  number_t port;
  
  CapeList phy_connections;
};

//------------------------------------------------------------------------------------------------------

void __STDCALL qbus_pvd_conn__phy_connections__on_del (void* ptr)
{
  QbusPvdConnection h = ptr; qbus_pvd_phy_connection_del (&h);
}

//------------------------------------------------------------------------------------------------------

QbusPvdEntity qbus_pvd_conn_new (QbusPvdCtx ctx, CapeUdc options, void* user_ptr, fct_qbus_pvd_factory__on_new on_new, fct_qbus_pvd_factory__on_del on_del, fct_qbus_pvd_cb__on_connection on_connection)
{
  QbusPvdEntity self = CAPE_NEW (struct QbusPvdEntity_s);
  
  self->ctx = ctx;
  
  // default callbacks for creating an external connection object
  self->user_data = user_ptr;
  self->factory_on_new = on_new;
  self->factory_on_del = on_del;
  self->on_connection_established = on_connection;
  
  self->host = cape_str_cp (cape_udc_get_s (options, "host", "127.0.0.1"));
  self->port = cape_udc_get_n (options, "port", 33340);
  
  self->phy_connections = cape_list_new (qbus_pvd_conn__phy_connections__on_del);
  
  return self;
}

//------------------------------------------------------------------------------------------------------

void qbus_pvd_conn_del (QbusPvdEntity* p_self)
{
  if (*p_self)
  {
    QbusPvdEntity self = *p_self;
    
    cape_str_del (&(self->host));
    cape_list_del (&(self->phy_connections));
    
    CAPE_DEL (p_self, struct QbusPvdEntity_s);
  }
}

//-----------------------------------------------------------------------------

CapeAioContext qbus_pvd_conn_aio (QbusPvdEntity self)
{
  return self->ctx->aio;
}

//-----------------------------------------------------------------------------

int __STDCALL qbus_pvd_internal__on_timer (void* ptr)
{
  CapeErr err = cape_err_new ();
  
  qbus_pvd_reconnect (ptr, err);
  
  cape_err_del (&err);
  
  // remove the timer
  return FALSE;
}

//-----------------------------------------------------------------------------

void qbus_pvd_internal__timer_enable (QbusPvdEntity self)
{
  int res;
  CapeErr err = cape_err_new ();
  
  CapeAioTimer timer = cape_aio_timer_new ();
  
  res = cape_aio_timer_set (timer, 10000, self, qbus_pvd_internal__on_timer, err);   // create timer with 10 seconds
  if (res)
  {
    // throw std::runtime_error (cape_err_text (err));
  }
  
  res = cape_aio_timer_add (&timer, self->ctx->aio);
  if (res)
  {
    // throw std::runtime_error (cape_err_text (err));
  }
  
  cape_err_del (&err);
}

//------------------------------------------------------------------------------------------------------

void qbus_pvd_internal__on_done (QbusPvdEntity self, void** p_user_object)
{
  if (self->factory_on_del)
  {
    self->factory_on_del (p_user_object);
  }
}

//------------------------------------------------------------------------------------------------------

void qbus_pvd_internal__connections_rm (QbusPvdEntity self, QbusPvdConnection phy_connection)
{
  CapeListCursor* cursor = cape_list_cursor_create (self->phy_connections, CAPE_DIRECTION_FORW);
  
  while (cape_list_cursor_next (cursor))
  {
    QbusPvdConnection phy_connection_loop = cape_list_node_data (cursor->node);
    
    if (phy_connection_loop == phy_connection)
    {
      cape_list_cursor_erase (self->phy_connections, cursor);
      break;
    }
  }
  
  cape_list_cursor_destroy (&cursor);
}

//-----------------------------------------------------------------------------

static void __STDCALL qbus_pvd_listen__on_sent (void* ptr, CapeAioSocket socket, void* userdata)
{
  QbusPvdConnection self = ptr;

  if (self->on_sent)
  {
    self->on_sent (self->user_object, socket, userdata);
  }
}

//-----------------------------------------------------------------------------

static void __STDCALL qbus_pvd_listen__on_recv (void* ptr, CapeAioSocket socket, const char* bufdat, number_t buflen)
{
  QbusPvdConnection self = ptr;

  if (self->on_recv)
  {
    self->on_recv (self->user_object, socket, bufdat, buflen);
  }
}

//-----------------------------------------------------------------------------

static void __STDCALL qbus_pvd_listen__on_done (void* ptr, void* userdata)
{
  QbusPvdConnection self = ptr;
  
  cape_log_fmt (CAPE_LL_TRACE, "QBUS", "on disconnect", "tear down qbus connection");
  
  // invalidate the socket
  self->aio_socket = NULL;
  
  qbus_pvd_internal__connections_rm (self->conn, self);
}

//-----------------------------------------------------------------------------

static void __STDCALL qbus_pvd_listen__on_connect (void* ptr, void* handle, const char* remote_addr)
{
  QbusPvdEntity self = ptr;
  
  cape_log_fmt (CAPE_LL_TRACE, "QBUS", "on connect", "new connection from %s", remote_addr);
  
  if (handle == NULL)
  {
    return;
  }
  
  // handle new connection
  {
    // create a new object for the created socket
    QbusPvdConnection phy_connection = qbus_pvd_phy_connection_new (self);

    // create a new handler for the created socket
    CapeAioSocket sock = cape_aio_socket_new (handle);

    // create a new object to handle this connection in an upper layer
    if (self->factory_on_new)
    {
      phy_connection->user_object = self->factory_on_new (self->user_data, phy_connection);
    }
    
    // store the socket handler for disconnect
    phy_connection->aio_socket = sock;

    // set qbus connection callbacks
    //qbus_connection_cb (qbus_connection, self->aio, sock, qbus_engine_tcp_send, qbus_engine_tcp_mark);
    
    // set callback
    cape_aio_socket_callback (sock, phy_connection, qbus_pvd_listen__on_sent, qbus_pvd_listen__on_recv, qbus_pvd_listen__on_done);
    
    // listen on the connection
    cape_aio_socket_listen (&sock, self->ctx->aio);
    
    // activate routing
    if (self->on_connection_established)
    {
      self->on_connection_established (phy_connection->user_object);
    }
    
    // finally add this connection the list
    cape_list_push_back (self->phy_connections, phy_connection);
  }
}

//-----------------------------------------------------------------------------

static void __STDCALL qbus_pvd_listen__on_accept_done (void* ptr)
{
  cape_log_msg (CAPE_LL_TRACE, "QBUS", "on accept done", "stopped listen");
}

//------------------------------------------------------------------------------------------------------

int __STDCALL qbus_pvd_listen (QbusPvdEntity self, CapeErr err)
{
  void* socket_handle = cape_sock__tcp__srv_new (self->host, self->port, err);
  
  if (socket_handle == NULL)
  {
    return cape_err_code (err);
  }
  
  {
    // create a new acceptor context for the AIO subsystem
    CapeAioAccept accept_event_handler = cape_aio_accept_new (socket_handle);
    
    // set callbacks
    cape_aio_accept_callback (accept_event_handler, self, qbus_pvd_listen__on_connect, qbus_pvd_listen__on_accept_done);
    
    // register at AIO
    cape_aio_accept_add (&accept_event_handler, self->ctx->aio);
  }
  
  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

void __STDCALL qbus_pvd_reconnect__on_sent (void* ptr, CapeAioSocket socket, void* userdata)
{
  QbusPvdConnection self = ptr;
  
  if (self->on_sent)
  {
    self->on_sent (self->user_object, socket, userdata);
  }
}

//-----------------------------------------------------------------------------

void __STDCALL qbus_pvd_reconnect__on_recv (void* ptr, CapeAioSocket socket, const char* bufdat, number_t buflen)
{
  QbusPvdConnection self = ptr;
  
  if (self->on_recv)
  {
    self->on_recv (self->user_object, socket, bufdat, buflen);
  }
}

//-----------------------------------------------------------------------------

void __STDCALL qbus_pvd_phy_connection__on_done (void* ptr, void* userdata)
{
  QbusPvdConnection self = ptr;
  
  // add timer for reconnection
  qbus_pvd_internal__timer_enable (self->conn);

  // invalidate the socket
  self->aio_socket = NULL;
  
  // remove this connection from the list
  qbus_pvd_internal__connections_rm (self->conn, self);
}

//------------------------------------------------------------------------------------------------------

int __STDCALL qbus_pvd_reconnect (QbusPvdEntity self, CapeErr err)
{
  void* sock = cape_sock__tcp__clt_new (self->host, self->port, err);
  if (sock == NULL)
  {
    return cape_err_code (err);
  }
  
  // handle new connection
  {
    QbusPvdConnection phy_connection = qbus_pvd_phy_connection_new (self);

    // create a new object to handle this connection in an upper layer
    if (self->factory_on_new)
    {
      phy_connection->user_object = self->factory_on_new (self->user_data, phy_connection);
    }

    CapeAioSocket s = cape_aio_socket_new (sock);
    
    // store the socket handler for disconnect
    phy_connection->aio_socket = s;

    // set callbacks
    // we can use the event handler directly
    cape_aio_socket_callback (s, phy_connection, qbus_pvd_reconnect__on_sent, qbus_pvd_reconnect__on_recv, qbus_pvd_phy_connection__on_done);
    
    // set qbus connection callbacks
    //qbus_connection_cb (qbus_connection, self->aio, s, qbus_engine_tcp_send, qbus_engine_tcp_mark);
    
    cape_aio_socket_listen (&s, self->ctx->aio);
    
    // activate routing
    if (self->on_connection_established)
    {
      self->on_connection_established (phy_connection->user_object);
    }

    cape_list_push_back (self->phy_connections, phy_connection);
  }
  
  return CAPE_ERR_NONE;
}

//------------------------------------------------------------------------------------------------------

void __STDCALL qbus_pvd_ctx__connections__on_del (void* ptr)
{
  QbusPvdEntity h = ptr; qbus_pvd_conn_del (&h);
}

//------------------------------------------------------------------------------------------------------

QbusPvdCtx __STDCALL qbus_pvd_ctx_new (CapeAioContext aio, CapeUdc options, CapeErr err)
{
  QbusPvdCtx self = CAPE_NEW (struct QbusPvdCtx_s);
  
  self->aio = aio;
  self->connections = cape_list_new (qbus_pvd_ctx__connections__on_del);
  
  return self;
}

//------------------------------------------------------------------------------------------------------

void __STDCALL qbus_pvd_ctx_del (QbusPvdCtx* p_self)
{
  if (*p_self)
  {
    QbusPvdCtx self = *p_self;

    cape_list_del (&(self->connections));
    
    CAPE_DEL (p_self, struct QbusPvdCtx_s);
  }
}

//------------------------------------------------------------------------------------------------------

QbusPvdEntity __STDCALL qbus_pvd_ctx_add (QbusPvdCtx self, CapeUdc options, void* user_ptr, fct_qbus_pvd_factory__on_new on_new, fct_qbus_pvd_factory__on_del on_del, fct_qbus_pvd_cb__on_connection on_connection)
{
  QbusPvdEntity ret = qbus_pvd_conn_new (self, options, user_ptr, on_new, on_del, on_connection);
  
  // store this object in connections
  cape_list_push_back (self->connections, ret);
  
  return ret;
}

//------------------------------------------------------------------------------------------------------

void __STDCALL qbus_pvd_ctx_rm (QbusPvdCtx self, QbusPvdEntity conn)
{
  CapeListCursor* cursor = cape_list_cursor_create (self->connections, CAPE_DIRECTION_FORW);
  
  while (cape_list_cursor_next (cursor))
  {
    QbusPvdEntity conn_loop = cape_list_node_data (cursor->node);
    
    if (conn_loop == conn)
    {
      cape_list_cursor_erase (self->connections, cursor);
      break;
    }
  }
  
  cape_list_cursor_destroy (&cursor);
}

//------------------------------------------------------------------------------------------------------
