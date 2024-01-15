#include "qwebs_prot_ws.h"
#include "qwebs.h"
#include "qwebs_connection.h"

// cape includes
#include <sys/cape_types.h>
#include <sys/cape_file.h>
#include <sys/cape_log.h>
#include <stc/cape_map.h>
#include <stc/cape_cursor.h>

// qcrypt includes
#include <qcrypt.h>

//-----------------------------------------------------------------------------

#define RFC_WEBSOCKET_FRAME__CONTINUATION    0x0
#define RFC_WEBSOCKET_FRAME__TEXT            0x1
#define RFC_WEBSOCKET_FRAME__BINARY          0x2
#define RFC_WEBSOCKET_FRAME__CLOSED          0x8
#define RFC_WEBSOCKET_FRAME__PING            0x9
#define RFC_WEBSOCKET_FRAME__PONG            0xa

//-----------------------------------------------------------------------------

struct QWebsProtWebsocketConnection_s
{
  QWebsProtWebsocket ws;     // reference
  QWebsConnection conn;      // reference
  
  void* conn_ptr;
  
  number_t state;
  
  int fin;
  int rsv1;
  int rsv2;
  int rsv3;
  int mask;
  
  number_t data_size;
  CapeString masking_key;
  cape_uint8 opcode;
  
  CapeStream buffer;

};

//-----------------------------------------------------------------------------

#define QWEBS_PROT_WEBSOCKET_RECV__NONE      0
#define QWEBS_PROT_WEBSOCKET_RECV__HEADER1   1
#define QWEBS_PROT_WEBSOCKET_RECV__LENGTH    2
#define QWEBS_PROT_WEBSOCKET_RECV__PAYLOAD   3

//-----------------------------------------------------------------------------

QWebsProtWebsocketConnection qwebs_prot_websocket_connection_new (QWebsProtWebsocket ws, QWebsConnection conn)
{
  QWebsProtWebsocketConnection self = CAPE_NEW (struct QWebsProtWebsocketConnection_s);

  self->ws = ws;
  self->conn = conn;
  
  self->conn_ptr = NULL;
  
  self->state = QWEBS_PROT_WEBSOCKET_RECV__NONE;
  
  self->fin = 0;
  self->rsv1 = 0;
  self->rsv2 = 0;
  self->rsv3 = 0;
  self->mask = 0;
  
  self->masking_key = NULL;
  self->opcode = 0;
  
  self->buffer = NULL;
  
  return self;
}

//-----------------------------------------------------------------------------

void qwebs_prot_websocket_connection_del (QWebsProtWebsocketConnection* p_self)
{
  if (*p_self)
  {
    QWebsProtWebsocketConnection self = *p_self;
    
    cape_str_del (&(self->masking_key));
    
    CAPE_DEL (p_self, struct QWebsProtWebsocketConnection_s);
  }
}

//-----------------------------------------------------------------------------

/*
void qwebs_prot_websocket__encode_payload (char* bufdat, number_t buflen, const CapeString m)
{
  number_t i;
  
  for (i = 0; i < buflen; i++)
  {
    bufdat[i] = bufdat[i] ^ m[i % 4];
  }
}
*/

//-----------------------------------------------------------------------------

void qwebs_prot_websocket_send__frame (QWebsProtWebsocketConnection self, number_t opcode, const char* bufdat, number_t buflen)
{
  // local objects
  CapeStream s = cape_stream_new ();
  number_t size_type = 0;
  
  /* the server is not allowed to send masked payload
   * -> mask was set to 0
   */

  {
    cape_uint8 bits01 = opcode;  // opcode text
    
    bits01 |= 0B10000000;   // fin
    
    cape_stream_append_08 (s, bits01);
  }
  {
    cape_uint8 bits02 = 0;
    
    if (buflen < 126)
    {
      bits02 = buflen;
    }
    else if (buflen < 65535)
    {
      bits02 = 126;
      size_type = 1;
    }
    else
    {
      bits02 = 127;
      size_type = 2;
    }
    
    cape_stream_append_08 (s, bits02);
  }
  
  switch (size_type)
  {
    case 1:
    {
      cape_stream_append_16 (s, buflen, TRUE);
      break;
    }
    case 2:
    {
      cape_stream_append_32 (s, buflen, FALSE);
      break;
    }
  }
  
  // add the message to the buffer
  cape_stream_append_buf (s, bufdat, buflen);
  
  qwebs_connection_send (self->conn, &s);
}

//-----------------------------------------------------------------------------

void qwebs_prot_websocket_send_s (QWebsProtWebsocketConnection self, const CapeString message)
{
  if (self)
  {
    number_t data_size = cape_str_size (message);
    
    qwebs_prot_websocket_send__frame (self, RFC_WEBSOCKET_FRAME__TEXT, message, data_size);
  }
}

//-----------------------------------------------------------------------------

void qwebs_prot_websocket_send_buf (QWebsProtWebsocketConnection self, const char* bufdat, number_t buflen)
{
  qwebs_prot_websocket_send__frame (self, RFC_WEBSOCKET_FRAME__BINARY, bufdat, buflen);
}

//-----------------------------------------------------------------------------

struct QWebsProtWebsocket_s
{
  QWebs qwebs;
  
  void* user_ptr;
  fct_qwebs_prot_websocket__on_conn on_conn;
  fct_qwebs_prot_websocket__on_init on_init;
  fct_qwebs_prot_websocket__on_msg on_msg;
  fct_qwebs_prot_websocket__on_done on_done;
};

//-----------------------------------------------------------------------------

QWebsProtWebsocket qwebs_prot_websocket_new (QWebs qwebs)
{
  QWebsProtWebsocket self = CAPE_NEW (struct QWebsProtWebsocket_s);
  
  self->qwebs = qwebs;
  
  self->user_ptr = NULL;
  
  self->on_conn = NULL;
  self->on_init = NULL;
  self->on_msg = NULL;
  self->on_done = NULL;
  
  return self;
}

//-----------------------------------------------------------------------------

void* __STDCALL qwebs_prot_websocket__on_upgrade (void* user_ptr, QWebsRequest request, CapeMap return_header, CapeErr err)
{
  QWebsProtWebsocket self = user_ptr;
  QWebsProtWebsocketConnection ret = NULL;
  
  const CapeString key;
  CapeMapNode n;
  
  // local objects
  CapeString accept_key__text = NULL;
  CapeStream accept_key__hash = NULL;
  
  cape_log_fmt (CAPE_LL_DEBUG, "QWEBS", "on upgrade", "websocket connection seen");
  
  // try to find the appropriate header entry
  n = cape_map_find (qwebs_request_headers (request), (void*)"Sec-WebSocket-Key");
  if (NULL == n)
  {
    cape_log_msg (CAPE_LL_WARN, "QWEBS", "on upgrade", "request has no 'Sec-WebSocket-Key'");
    goto exit_and_cleanup;
  }
  
  key = cape_map_node_value (n);
  if (NULL == key)
  {
    cape_log_msg (CAPE_LL_WARN, "QWEBS", "on upgrade", "header entry 'Sec-WebSocket-Key' is invalid");
    goto exit_and_cleanup;
  }
  
  // see RFC, concat the defined UUID as accept key
  accept_key__text = cape_str_catenate_2 (key, "258EAFA5-E914-47DA-95CA-C5AB0DC85B11");
  
  // hash the accept key
  accept_key__hash = qcrypt__hash_sha__bin_o  (accept_key__text, cape_str_size (accept_key__text), err);
  if (NULL == accept_key__hash)
  {
    cape_log_msg (CAPE_LL_WARN, "QWEBS", "on upgrade", "can't create hash for the accept-key");
    goto exit_and_cleanup;
  }
  
  // add accept key as base64 as return header
  {
    CapeString accept_key = qcrypt__encode_base64_m (accept_key__hash);
    cape_map_insert (return_header, cape_str_cp ("Sec-WebSocket-Accept"), accept_key);
  }
  
  ret = qwebs_prot_websocket_connection_new (self, qwebs_request_conn (request));
  
  if (self->on_conn)
  {
    ret->conn_ptr = self->on_conn (self->user_ptr, ret, qwebs_request_query (request));
  }
  
exit_and_cleanup:
  
  cape_str_del (&accept_key__text);
  cape_stream_del (&accept_key__hash);
  
  return ret;
}

//-----------------------------------------------------------------------------

void __STDCALL qwebs_prot_websocket__on_switched (void* conn_ptr)
{
  QWebsProtWebsocketConnection self = conn_ptr;

  if (self->ws->on_init)
  {
    self->ws->on_init (self->conn_ptr);
  }
}

//-----------------------------------------------------------------------------

void qwebs_prot_websocket__decode_header1 (QWebsProtWebsocketConnection self, CapeCursor cursor)
{
  {
    cape_uint8 bits01 = cape_cursor_scan_08 (cursor);
    
    self->fin  = (bits01 & 0B10000000) == 0B10000000;
    self->rsv1 = (bits01 & 0B01000000) == 0B01000000;
    self->rsv2 = (bits01 & 0B00100000) == 0B00100000;
    self->rsv3 = (bits01 & 0B00010000) == 0B00010000;
    
    self->opcode = (bits01 << 4);
    self->opcode = self->opcode >> 4;
  }
  {
    cape_uint8 bits02 = cape_cursor_scan_08 (cursor);
    
    self->mask = (bits02 & 0B10000000) == 0B10000000;
    
    self->data_size = bits02 & ~0B10000000;
  }
  
  //cape_log_fmt (CAPE_LL_TRACE, "QWEBS", "websocket", "frame header: fin = %i, rsv1 = %i, rsv2 = %i, rsv3 = %i, opcode = %i", self->fin, self->rsv1, self->rsv2, self->rsv3, self->opcode);
}

//-----------------------------------------------------------------------------

void qwebs_prot_websocket__decode_payload (QWebsProtWebsocketConnection self, CapeCursor cursor)
{
  number_t i;
  
  CapeString h = cape_cursor_scan_s (cursor, self->data_size);
  
  if (self->masking_key)
  {
    for (i = 0; i < self->data_size; i++)
    {
      h[i] = h[i] ^ self->masking_key[i % 4];
    }
  }
  
  if (self->ws->on_msg)
  {
    self->ws->on_msg (self->conn_ptr, h);
  }
  
  cape_str_del (&h);
}

//-----------------------------------------------------------------------------

void qwebs_prot_websocket__adjust_buffer (QWebsProtWebsocketConnection self, CapeCursor cursor)
{
  if (NULL == self->buffer)   // the cursor is running on the local buffer
  {
    self->buffer = cape_stream_new ();
    
    // copy the rest of the local buffer into the out buffer
    cape_stream_append_buf (self->buffer, cape_cursor_dpos (cursor), cape_cursor_tail (cursor));
  }
  else if (cape_cursor_apos (cursor) > 0)  // the cursor is running on self->buffer
  {
    CapeStream h = cape_stream_new ();
    
    // shift the buffer
    cape_stream_append_buf (h, cape_cursor_dpos (cursor), cape_cursor_tail (cursor));
    
    // replace our buffer with the shifted buffer
    cape_stream_del (&(self->buffer));
    
    self->buffer = h;
    h = NULL;
  }
}

//-----------------------------------------------------------------------------

void __STDCALL qwebs_prot_websocket__on_recv (void* user_ptr, QWebsConnection conn, const char* bufdat, number_t buflen)
{
  QWebsProtWebsocketConnection self = user_ptr;

  // local objects
  CapeCursor cursor = cape_cursor_new ();
  
  //cape_log_fmt (CAPE_LL_TRACE, "QWEBS", "websocket", "received buffer with len = %i", buflen);

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
  
  if (self->state == QWEBS_PROT_WEBSOCKET_RECV__NONE)
  {
    if (cape_cursor__has_data (cursor, 2))
    {
      qwebs_prot_websocket__decode_header1 (self, cursor);
      
      self->state = QWEBS_PROT_WEBSOCKET_RECV__HEADER1;
    }
    else
    {
      qwebs_prot_websocket__adjust_buffer (self, cursor);
    }
  }
  
  if (self->state == QWEBS_PROT_WEBSOCKET_RECV__HEADER1)
  {
    if (self->data_size == 126)
    {
      if (cape_cursor__has_data (cursor, 2))
      {
        self->data_size = cape_cursor_scan_16 (cursor, TRUE);

        self->state = QWEBS_PROT_WEBSOCKET_RECV__LENGTH;
      }
      else
      {
        qwebs_prot_websocket__adjust_buffer (self, cursor);
      }
    }
    else if (self->data_size == 127)
    {
      if (cape_cursor__has_data (cursor, 6))
      {
        self->data_size = cape_cursor_scan_32 (cursor, TRUE);

        self->state = QWEBS_PROT_WEBSOCKET_RECV__LENGTH;
      }
      else
      {
        qwebs_prot_websocket__adjust_buffer (self, cursor);
      }
    }
    else
    {
      self->state = QWEBS_PROT_WEBSOCKET_RECV__LENGTH;
    }
  }
  
  if (self->state == QWEBS_PROT_WEBSOCKET_RECV__LENGTH)
  {
    if (self->mask)
    {
      if (cape_cursor__has_data (cursor, 4))
      {
        cape_str_del (&(self->masking_key));
        self->masking_key = cape_cursor_scan_s (cursor, 4);
        
        self->state = QWEBS_PROT_WEBSOCKET_RECV__PAYLOAD;
      }
      else
      {
        qwebs_prot_websocket__adjust_buffer (self, cursor);
      }
    }
    else
    {
      self->state = QWEBS_PROT_WEBSOCKET_RECV__PAYLOAD;
    }
  }
  
  if (self->state == QWEBS_PROT_WEBSOCKET_RECV__PAYLOAD)
  {
    if (cape_cursor__has_data (cursor, self->data_size))
    {
      qwebs_prot_websocket__decode_payload (self, cursor);

      cape_stream_del (&(self->buffer));
      
      self->state = QWEBS_PROT_WEBSOCKET_RECV__NONE;
    }
    else
    {
      qwebs_prot_websocket__adjust_buffer (self, cursor);
    }
  }
  
  cape_cursor_del (&cursor);
}

//-----------------------------------------------------------------------------

void __STDCALL qwebs_prot_websocket__on_del (void** p_user_ptr)
{
  QWebsProtWebsocketConnection* p_self = (QWebsProtWebsocketConnection*)p_user_ptr;

  cape_log_fmt (CAPE_LL_DEBUG, "QWEBS", "websocket", "websocket connection dropped");

  if (*p_self)
  {
    QWebsProtWebsocketConnection self = *p_self;
    
    if (self->ws->on_done)
    {
      self->ws->on_done (self->conn_ptr);
      self->conn_ptr = NULL;
    }
    
    CAPE_DEL (p_self, struct QWebsProtWebsocketConnection_s);
  }
}

//-----------------------------------------------------------------------------

int qwebs_prot_websocket_init (QWebsProtWebsocket self, CapeErr err)
{
  // register a handling for websockets
  return qwebs_on_upgrade (self->qwebs, "websocket", self, qwebs_prot_websocket__on_upgrade, qwebs_prot_websocket__on_switched, qwebs_prot_websocket__on_recv, qwebs_prot_websocket__on_del, err);
}

//-----------------------------------------------------------------------------

void qwebs_prot_websocket_cb (QWebsProtWebsocket self, void* user_ptr, fct_qwebs_prot_websocket__on_conn on_conn, fct_qwebs_prot_websocket__on_init on_init, fct_qwebs_prot_websocket__on_msg on_msg, fct_qwebs_prot_websocket__on_done on_done)
{
  self->user_ptr = user_ptr;
  
  self->on_conn = on_conn;
  self->on_init = on_init;
  self->on_msg = on_msg;
  self->on_done = on_done;
}

//-----------------------------------------------------------------------------
