#include "qbus_agent.h"

// cape includes
#include <sys/cape_log.h>
#include <sys/cape_socket.h>
#include <stc/cape_cursor.h>
#include <stc/cape_map.h>

//-----------------------------------------------------------------------------

struct QBusAgentContext_s
{
  CapeStream buffer;
  int state;
  
}; typedef struct QBusAgentContext_s* QBusAgentContext;

//-----------------------------------------------------------------------------

QBusAgentContext qbus_agent_context_new ()
{
  QBusAgentContext self = CAPE_NEW (struct QBusAgentContext_s);

  self->buffer = cape_stream_new ();
  self->state = 0;
  
  return self;
}

//-----------------------------------------------------------------------------

void qbus_agent_context_del (QBusAgentContext* p_self)
{
  if (*p_self)
  {
    QBusAgentContext self = *p_self;
    
    cape_stream_del (&(self->buffer));
    
    CAPE_DEL (p_self, struct QBusAgentContext_s);
  }
}

//-----------------------------------------------------------------------------

void qbus_agent_context_adjust_buffer (QBusAgentContext self, CapeCursor cursor)
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

number_t qbus_agent_context_parse (QBusAgentContext self, const char* bufdat, number_t buflen)
{
  number_t ret = 0;
  
  // a boolean to signal that there is enough data received to continue
  int has_enogh_bytes_for_parsing = TRUE;

  // local objects
  CapeCursor cursor = cape_cursor_new ();
  
  // extend the current buffer with the data we received
  cape_stream_append_buf (self->buffer, bufdat, buflen);
  
  // use the current buffer for the cursor
  cape_cursor_set (cursor, cape_stream_data (self->buffer), cape_stream_size (self->buffer));
  
  while (has_enogh_bytes_for_parsing)
  {
    switch (self->state)
    {
      case 0:
      {
        // check for 32bits = 4bytes
        if (cape_cursor__has_data (cursor, 4))
        {
          cape_uint32 opcode = cape_cursor_scan_32 (cursor, TRUE);

          // transform
          ret = (number_t)opcode;
        }
        else
        {
          has_enogh_bytes_for_parsing = FALSE;
        }
       
        break;
      }      
    }
  }
  
  qbus_agent_context_adjust_buffer (self, cursor);
  
  cape_cursor_del (&cursor);
  
  return ret;
}

//-----------------------------------------------------------------------------

struct QBusAgent_s
{
  QBusRouter router;             // reference
  CapeMap buffer_matrix;         // stores multiple buffers
};

//-----------------------------------------------------------------------------

void __STDCALL qbus_agent_map_on_del (void* key, void* val)
{
  {
    CapeString h = key; cape_str_del (&h);
  }
  {
    QBusAgentContext s = val; qbus_agent_context_del (&s);
  }
}

//-----------------------------------------------------------------------------

QBusAgent qbus_agent_new (QBusRouter router)
{
  QBusAgent self = CAPE_NEW (struct QBusAgent_s);

  self->router = router;
  self->buffer_matrix = cape_map_new (cape_map__compare__s, qbus_agent_map_on_del, NULL);

  return self;
}

//-----------------------------------------------------------------------------

void qbus_agent_del (QBusAgent* p_self)
{
  if (*p_self)
  {
    QBusAgent self = *p_self;

    cape_map_del (&(self->buffer_matrix));

    CAPE_DEL (p_self, struct QBusAgent_s);
  }
}

//-----------------------------------------------------------------------------

void __STDCALL qbus_agent__on_sent_ready (void* user_ptr, CapeAioSocketUdp sock, void* userdata)
{
  QBusAgent self = user_ptr;

  
}

//-----------------------------------------------------------------------------

QBusAgentContext qbus_agent__get__context (QBusAgent self, const char* host)
{
  QBusAgentContext ret;
  
  CapeMapNode n = cape_map_find (self->buffer_matrix, (void*)host);
  if (n)
  {
    ret = cape_map_node_value (n);
  }
  else
  {
    ret = qbus_agent_context_new ();
    
    // insert into map, transfer ownership to map
    cape_map_insert (self->buffer_matrix, (void*)cape_str_cp (host), (void*)ret);
  }
  
  return ret;
}

//-----------------------------------------------------------------------------

void __STDCALL qbus_agent__on_recv_from (void* user_ptr, CapeAioSocketUdp sock, const char* bufdat, number_t buflen, const char* host)
{
  QBusAgent self = user_ptr;

  // fetch the context and parse the protocol
  switch (qbus_agent_context_parse (qbus_agent__get__context (self, host), bufdat, buflen))
  {
    case 101:
    {
      CapeUdc known_modules = qbus_router_list (self->router);
      
      break;
    }    
  }
}

//-----------------------------------------------------------------------------

void __STDCALL qbus_agent__on_done (void* user_ptr, void* userdata)
{
  QBusAgent self = user_ptr;
  
}

//-----------------------------------------------------------------------------

int qbus_agent_init (QBusAgent self, CapeAioContext aio, number_t port, CapeErr err)
{
  int res;
  
  // local objects
  CapeAioSocketUdp aio_socket = NULL;
  
  void* socket = cape_sock__udp__srv_new ("127.0.0.1", port, err);
  if (NULL == socket)
  {
    res = cape_err_code (err);
    goto cleanup_and_exit;
  }

  // create a new AIO event driven socket handler
  aio_socket = cape_aio_socket__udp__new (socket);

  // set the callbacks
  cape_aio_socket__udp__cb (aio_socket, self, qbus_agent__on_sent_ready, qbus_agent__on_recv_from, qbus_agent__on_done);
  
  // add to the AIO event broker
  cape_aio_socket__udp__add (&aio_socket, aio, CAPE_AIO_WRITE | CAPE_AIO_READ | CAPE_AIO_ERROR);  
  
  res = CAPE_ERR_NONE;
  
cleanup_and_exit:
  
  return res;
}

//-----------------------------------------------------------------------------
