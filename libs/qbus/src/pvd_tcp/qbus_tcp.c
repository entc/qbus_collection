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
  
};

//------------------------------------------------------------------------------------------------------

QbusPvdCtx qbus_pvd_ctx_new (CapeAioContext aio, CapeUdc options, CapeErr err)
{
  QbusPvdCtx self = CAPE_NEW (struct QbusPvdCtx_s);

  self->aio = aio;
  
  return self;
}

//------------------------------------------------------------------------------------------------------

int qbus_pvd_ctx_del (QbusPvdCtx* p_self)
{
  if (*p_self)
  {
    QbusPvdCtx self = *p_self;
    
    CAPE_DEL (p_self, struct QbusPvdCtx_s);
  }
}

//------------------------------------------------------------------------------------------------------

struct QbusPvdConn_s
{
  QbusPvdCtx ctx;        // reference

  void* user_data;
  fct_qbus_pvd_factory__on_new factory_on_new;
  fct_qbus_pvd_factory__on_del factory_on_del;
  fct_qbus_pvd_cb__on_connection on_connection_established;
  
  void* user_object;
  fct_cape_aio_socket_onSent on_sent;
  fct_cape_aio_socket_onRecv on_recv;
  
  CapeString host;
  number_t port;
};

//------------------------------------------------------------------------------------------------------

QbusPvdConn qbus_pvd_new (QbusPvdCtx ctx, CapeUdc options, void* user_ptr, fct_qbus_pvd_factory__on_new on_new, fct_qbus_pvd_factory__on_del on_del, fct_qbus_pvd_cb__on_connection on_connection)
{
  QbusPvdConn self = CAPE_NEW (struct QbusPvdConn_s);
  
  self->ctx = ctx;

  // default callbacks for creating an external connection object
  self->user_data = user_ptr;
  self->factory_on_new = on_new;
  self->factory_on_del = on_del;
  self->on_connection_established = on_connection;
  
  self->user_object = NULL;
  self->on_sent = NULL;
  self->on_recv = NULL;
  
  self->host = cape_str_cp (cape_udc_get_s (options, "host", "127.0.0.1"));
  self->port = cape_udc_get_n (options, "port", 33340);
  
  return self;
}

//------------------------------------------------------------------------------------------------------

void qbus_pvd_del (QbusPvdConn* p_self)
{
  if (*p_self)
  {
    QbusPvdConn self = *p_self;
    
    cape_str_del (&(self->host));
    
    CAPE_DEL (p_self, struct QbusPvdConn_s);
  }
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

void qbus_pvd_internal__timer_enable (QbusPvdConn self)
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

//-----------------------------------------------------------------------------

static void __STDCALL qbus_pvd_listen__on_sent (void* ptr, CapeAioSocket socket, void* userdata)
{
  QbusPvdConn self = ptr;

  if (self->on_sent)
  {
    self->on_sent (self->user_object, socket, userdata);
  }
}

//-----------------------------------------------------------------------------

static void __STDCALL qbus_pvd_listen__on_recv (void* ptr, CapeAioSocket socket, const char* bufdat, number_t buflen)
{
  QbusPvdConn self = ptr;

  if (self->on_recv)
  {
    self->on_recv (self->user_object, socket, bufdat, buflen);
  }
}

//-----------------------------------------------------------------------------

static void __STDCALL qbus_pvd_listen__on_done (void* ptr, void* userdata)
{
  QbusPvdConn self = ptr;
  
  if (self->factory_on_del)
  {
    self->factory_on_del (&(self->user_object));
  }
}

//-----------------------------------------------------------------------------

static void __STDCALL qbus_pvd_listen__on_connect (void* ptr, void* handle, const char* remote_addr)
{
  QbusPvdConn self = ptr;
  
  cape_log_fmt (CAPE_LL_TRACE, "QBUS", "on connect", "new connection from %s", remote_addr);
  
  if (handle == NULL)
  {
    return;
  }
  
  // handle new connection
  {
    // create a new object to handle this connection in an upper layer
    if (self->factory_on_new)
    {
      self->user_object = self->factory_on_new (self->user_data);
    }
    
    // create a new handler for the created socket
    CapeAioSocket sock = cape_aio_socket_new (handle);
    
    // set qbus connection callbacks
    //qbus_connection_cb (qbus_connection, self->aio, sock, qbus_engine_tcp_send, qbus_engine_tcp_mark);
    
    // set callback
    cape_aio_socket_callback (sock, self, qbus_pvd_listen__on_sent, qbus_pvd_listen__on_recv, qbus_pvd_listen__on_done);
    
    // listen on the connection
    cape_aio_socket_listen (&sock, self->ctx->aio);
    
    // activate routing
    if (self->on_connection_established)
    {
      self->on_connection_established (self->user_object);
    }
  }
}

//-----------------------------------------------------------------------------

static void __STDCALL qbus_pvd_listen__on_accept_done (void* ptr)
{
  cape_log_msg (CAPE_LL_TRACE, "QBUS", "on accept done", "stopped listen");
}

//------------------------------------------------------------------------------------------------------

int qbus_pvd_listen (QbusPvdConn self, CapeErr err)
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
  QbusPvdConn self = ptr;
  
  if (self->on_sent)
  {
    self->on_sent (self->user_object, socket, userdata);
  }
}

//-----------------------------------------------------------------------------

void __STDCALL qbus_pvd_reconnect__on_recv (void* ptr, CapeAioSocket socket, const char* bufdat, number_t buflen)
{
  QbusPvdConn self = ptr;
  
  if (self->on_recv)
  {
    self->on_recv (self->user_object, socket, bufdat, buflen);
  }
}

//-----------------------------------------------------------------------------

void __STDCALL qbus_pvd_reconnect__on_done (void* ptr, void* userdata)
{
  QbusPvdConn self = ptr;
  
  if (self->factory_on_del)
  {
    self->factory_on_del (&(self->user_object));
  }

  // add timer for reconnection
  qbus_pvd_internal__timer_enable (self);
}

//------------------------------------------------------------------------------------------------------

int qbus_pvd_reconnect (QbusPvdConn self, CapeErr err)
{
  void* sock = cape_sock__tcp__clt_new (self->host, self->port, err);
  if (sock == NULL)
  {
    return cape_err_code (err);
  }
  
  // handle new connection
  {
    // create a new object to handle this connection in an upper layer
    if (self->factory_on_new)
    {
      self->user_object = self->factory_on_new (self->user_data);
    }

    CapeAioSocket s = cape_aio_socket_new (sock);
    // set callbacks
    // we can use the event handler directly
    
    cape_aio_socket_callback (s, self, qbus_pvd_reconnect__on_sent, qbus_pvd_reconnect__on_recv, qbus_pvd_reconnect__on_done);
    
    // set qbus connection callbacks
    //qbus_connection_cb (qbus_connection, self->aio, s, qbus_engine_tcp_send, qbus_engine_tcp_mark);
    
    cape_aio_socket_listen (&s, self->ctx->aio);
    
    // activate routing
    if (self->on_connection_established)
    {
      self->on_connection_established (self->user_object);
    }
  }
  
  return CAPE_ERR_NONE;
}

//------------------------------------------------------------------------------------------------------

void qbus_pvd_cb_raw_set (QbusPvdConn self, fct_cape_aio_socket_onSent on_sent, fct_cape_aio_socket_onRecv on_recv)
{
  self->on_sent = on_sent;
  self->on_recv = on_recv;
}

//------------------------------------------------------------------------------------------------------
