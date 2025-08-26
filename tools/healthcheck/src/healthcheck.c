#include "healthcheck.h"

// cape includes
#include <sys/cape_socket.h>
#include <aio/cape_aio_sock.h>
#include <aio/cape_aio_ctx.h>
#include <aio/cape_aio_timer.h>
#include <stc/cape_cursor.h>
#include <sys/cape_log.h>

#define HEALTHCHECK_HEALTHY        0
#define HEALTHCHECK_UNHEALTHY      1
#define HEALTHCHECK_INITIALIZING   2

//-----------------------------------------------------------------------------

struct Healthcheck_s
{
  CapeAioContext aio;
  CapeAioSocketUdp aio_socket;

  number_t port;
  int wait_for_response;

  CapeStream buffer;
  int state;

  cape_uint32 opcode;
  number_t size;

  int health_state;
};

//-----------------------------------------------------------------------------

Healthcheck healthcheck_new ()
{
  Healthcheck self = CAPE_NEW (struct Healthcheck_s);

  self->aio = cape_aio_context_new ();
  self->aio_socket = NULL;

  self->port = 10162;
  self->wait_for_response = TRUE;

  self->buffer = NULL;
  self->state = 0;

  self->health_state = HEALTHCHECK_HEALTHY;

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

int healthcheck_state (Healthcheck self)
{
  return self->health_state;
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

void healthcheck_adjust_buffer (Healthcheck self, CapeCursor cursor)
{
  // local objects
  CapeStream h = NULL;

  {
    // returns the bytes which had not been used for parsing
    number_t bytes_left_to_scan = cape_cursor_tail (cursor);

    if (bytes_left_to_scan > 0)
    {
      h = cape_stream_new ();

      // shift the buffer
      // travers the cursor (to the end)
      cape_stream_append_buf (h, cape_cursor_tpos (cursor, bytes_left_to_scan), bytes_left_to_scan);
    }
  }

  // replace the buffer
  cape_stream_replace_mv (&(self->buffer), &h);
}

//-----------------------------------------------------------------------------

void __STDCALL healthcheck__on_recv_from (void* user_ptr, CapeAioSocketUdp sock, const char* bufdat, number_t buflen, const char* host)
{
  Healthcheck self = user_ptr;

  // a boolean to signal that there is enough data received to continue
  int has_enogh_bytes_for_parsing = TRUE;

  // local objects
  CapeCursor cursor = cape_cursor_new ();

  if (self->buffer)
  {
    // extend the current buffer with the data we received
    cape_stream_append_buf (self->buffer, bufdat, buflen);

    // use the current buffer for the cursor
    cape_cursor_set (cursor, cape_stream_data (self->buffer), cape_stream_size (self->buffer));
  }
  else
  {
    cape_cursor_set (cursor, bufdat, buflen);
  }

  while (has_enogh_bytes_for_parsing)
  {
    switch (self->state)
    {
      case 0:
      {
        // check for 32bits = 4bytes
        if (cape_cursor__has_data (cursor, 4))
        {
          self->opcode = cape_cursor_scan_32 (cursor, TRUE);
          self->state = 1;
        }
        else
        {
          has_enogh_bytes_for_parsing = FALSE;
        }

        break;
      }
      case 1:
      {
        // check for 64bits = 8bytes
        if (cape_cursor__has_data (cursor, 8))
        {
          self->size = cape_cursor_scan_64 (cursor, TRUE);
          self->state = 2;
        }
        else
        {
          has_enogh_bytes_for_parsing = FALSE;
        }

        break;
      }
      case 2:
      {
        if (cape_cursor__has_data (cursor, self->size))
        {
          CapeString json_encoded_text = cape_cursor_scan_s (cursor, self->size);

          cape_log_fmt (CAPE_LL_TRACE, "HEALTH", "received", "from agent: %s", json_encoded_text);

          self->state = 0;
          self->wait_for_response = FALSE;
        }
        else
        {
          has_enogh_bytes_for_parsing = FALSE;
        }

        break;
      }
    }
  }

  healthcheck_adjust_buffer (self, cursor);

  cape_cursor_del (&cursor);
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

  cape_log_fmt (CAPE_LL_WARN, "HEALTH", "on timer", "reached timeout -> abort check");

  self->wait_for_response = FALSE;
  self->health_state = HEALTHCHECK_UNHEALTHY;

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
  void* socket = cape_sock__udp__srv_new ("localhost", self->port, err);
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
  while (self->wait_for_response && (CAPE_ERR_NONE == cape_aio_context_next (self->aio, 100, err)));
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
    res = HEALTHCHECK_INITIALIZING;
    goto cleanup_and_exit;
  }

  healthceck_send_request (hc, "localhost", 10161);

  healthceck_wait (hc, err);

  res = healthcheck_state (hc);

cleanup_and_exit:

  cape_log_fmt (CAPE_LL_INFO, "HEALTH", "done", "return state = %i", res);

  healthcheck_del (&hc);

  cape_err_del (&err);
  return res;
}

//-----------------------------------------------------------------------------
