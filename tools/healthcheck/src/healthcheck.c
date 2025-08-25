#include "healthcheck.h"

// cape includes
#include <sys/cape_socket.h>
#include <aio/cape_aio_sock.h>
#include <aio/cape_aio_ctx.h>
#include <aio/cape_aio_timer.h>

//-----------------------------------------------------------------------------

struct Healthcheck_s
{
  CapeAioContext aio;
  CapeAioSocketUdp aio_socket;
  
  number_t port;
  int wait_for_response;
};

//-----------------------------------------------------------------------------

Healthcheck healthcheck_new ()
{
  Healthcheck self = CAPE_NEW (struct Healthcheck_s);
  
  self->aio = cape_aio_context_new ();
  self->aio_socket = NULL;
  
  self->port = 10162;
  self->wait_for_response = TRUE;
  
  return self;
}

//-----------------------------------------------------------------------------

void healthcheck_del (Healthcheck* p_self)
{
  if (*p_self)
  {
    Healthcheck self = *p_self;

    
    cape_aio_context_del (&(self->aio));
     
    CAPE_DEL(p_self, struct Healthcheck_s);
  }
}

//-----------------------------------------------------------------------------

void __STDCALL healthcheck__on_sent_ready (void* user_ptr, CapeAioSocketUdp sock, void* userdata)
{
  if (userdata)
  {
    CapeStream s = userdata; cape_stream_del (&s);
  }
}

//-----------------------------------------------------------------------------

void __STDCALL healthcheck__on_recv_from (void* user_ptr, CapeAioSocketUdp sock, const char* bufdat, number_t buflen, const char* host)
{
  Healthcheck self = user_ptr;
  
  self->wait_for_response = FALSE;
}

//-----------------------------------------------------------------------------

void __STDCALL healthcheck__on_done (void* user_ptr, void* userdata)
{
  if (userdata)
  {
    CapeStream s = userdata; cape_stream_del (&s);
  }
}

//-----------------------------------------------------------------------------

int __STDCALL healthcheck__on_timer (void* user_ptr)
{
  Healthcheck self = user_ptr;
  
  self->wait_for_response = FALSE;

  return FALSE;
}

//-----------------------------------------------------------------------------

int healthcheck_init (Healthcheck self, CapeErr err)
{
  int res;
  
  // local objects
  CapeAioSocketUdp aio_socket = NULL;
  CapeAioTimer timer = NULL;

  // start the AIO context
  res = cape_aio_context_open (self->aio, err);
  if (res)
  {
    goto cleanup_and_exit;
  }
  
  // try to create a listening socket
  void* socket = cape_sock__udp__srv_new ("127.0.0.1", self->port, err);
  if (NULL == socket)
  {
    res = cape_err_code (err);
    goto cleanup_and_exit;
  }
  
  // create a new AIO event driven socket handler
  aio_socket = cape_aio_socket__udp__new (socket);
  
  // set the callbacks
  cape_aio_socket__udp__cb (aio_socket, self, healthcheck__on_sent_ready, healthcheck__on_recv_from, healthcheck__on_done);
  
  // transfer ownership
  self->aio_socket = aio_socket;

  // add to the AIO event broker
  cape_aio_socket__udp__add (&aio_socket, self->aio, CAPE_AIO_READ | CAPE_AIO_ERROR);

  // allocate a new timer
  timer = cape_aio_timer_new ();

  cape_aio_timer_set (timer, 3000, self, healthcheck__on_timer, err);
  if (res)
  {
    goto cleanup_and_exit;
  }
  
  res = cape_aio_timer_add (&timer, self->aio);
  
cleanup_and_exit:
  
  return res;
}

//-----------------------------------------------------------------------------

void healthceck_send_request (Healthcheck self, const CapeString host, number_t port)
{
  CapeStream s = cape_stream_new ();
  
  cape_stream_append_32 (s, 101, TRUE);
  
  cape_stream_append_64 (s, self->port, TRUE);

  // this will send the UDP message
  cape_aio_socket__udp__send (self->aio_socket, self->aio, cape_stream_data (s), cape_stream_size (s), s, host, port); 
}

//-----------------------------------------------------------------------------

void healthceck_wait (Healthcheck self, CapeErr err)
{
  while (self->wait_for_response && (CAPE_ERR_NONE == cape_aio_context_next (self->aio, 100, err)))
  {
    
  }
}

//-----------------------------------------------------------------------------

int main (int argc, char *argv[])
{
  int res;
  
  // local objects
  CapeErr err = cape_err_new ();
  Healthcheck hc = healthcheck_new ();
  
  if (healthcheck_init (hc, err))
  {
    res = 2;
    goto cleanup_and_exit;
  }
  
  healthceck_send_request (hc, "127.0.0.1", 10161);
  
  healthceck_wait (hc, err);
  
cleanup_and_exit:

  healthcheck_del (&hc);
  
  cape_err_del (&err);  
  return res;
}
