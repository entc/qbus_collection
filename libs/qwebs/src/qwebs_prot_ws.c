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
  QWebsProtWebsocket ws;     // owned
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

  self->ws = qwebs_prot_websocket_inc (ws);
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
    
    // decrease the reference counter for this websocket addon
    qwebs_prot_websocket_dec (&(self->ws), &(self->conn_ptr));
        
    cape_str_del (&(self->masking_key));
    cape_stream_del (&(self->buffer));
    
    CAPE_DEL (p_self, struct QWebsProtWebsocketConnection_s);
  }
}

//-----------------------------------------------------------------------------

void __STDCALL qwebs_prot_websocket_connection__on_del (void** p_user_ptr)
{
  QWebsProtWebsocketConnection* p_self = (QWebsProtWebsocketConnection*)p_user_ptr;
  
  cape_log_fmt (CAPE_LL_DEBUG, "QWEBS", "websocket", "websocket connection dropped");
  
  qwebs_prot_websocket_connection_del (p_self);
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
  if (self->conn)
  {
    // local objects
    CapeStream s = cape_stream_new ();
    number_t size_type = 0;
    
    /* the server is not allowed to send masked payload
     * -> mask was set to 0
     */
    
    cape_log_fmt (CAPE_LL_TRACE, "QWEBS", "send frame", "buflen = %lu", buflen);
    
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
        cape_stream_append_64 (s, buflen, TRUE);
        break;
      }
    }
    
    // add the message to the buffer
    cape_stream_append_buf (s, bufdat, buflen);
    
    qwebs_connection_send (self->conn, &s);
  }
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
  void* user_ptr;
  fct_qwebs_prot_websocket__on_conn on_conn;
  fct_qwebs_prot_websocket__on_init on_init;
  fct_qwebs_prot_websocket__on_msg on_msg;
  fct_qwebs_prot_websocket__on_done on_done;
  
  int refcnt;
};

//-----------------------------------------------------------------------------

QWebsProtWebsocket qwebs_prot_websocket_new ()
{
  QWebsProtWebsocket self = CAPE_NEW (struct QWebsProtWebsocket_s);
  
  self->user_ptr = NULL;
  
  self->on_conn = NULL;
  self->on_init = NULL;
  self->on_msg = NULL;
  self->on_done = NULL;
  
  // always start with one
  self->refcnt = 1;
  
  return self;
}

//-----------------------------------------------------------------------------

void qwebs_prot_websocket_del (QWebsProtWebsocket* p_self)
{
  if (*p_self)
  {
    QWebsProtWebsocket self = *p_self;
    
#if defined __BSD_OS || defined __LINUX_OS
    
#ifdef __GCC_HAVE_SYNC_COMPARE_AND_SWAP_16
    
    int val = (__sync_sub_and_fetch (&(self->refcnt), 1));
    
#else
    
    int val = --(self->refcnt);
    
#endif

#elif defined __WINDOWS_OS
    
    int var = InterlockedDecrement (&(self->refcnt));
    
#endif
    
    if (val == 0)
    {
      
      CAPE_DEL (p_self, struct QWebsProtWebsocket_s);
    }    
  }
}

//-----------------------------------------------------------------------------

QWebsProtWebsocket qwebs_prot_websocket_inc (QWebsProtWebsocket self)
{
#if defined __BSD_OS || defined __LINUX_OS
  
#ifdef __GCC_HAVE_SYNC_COMPARE_AND_SWAP_16
  
  __sync_add_and_fetch (&(self->refcnt), 1);
  
#else
  
  (self->refcnt)++;
  
#endif
  
#elif defined __WINDOWS_OS
  
  InterlockedIncrement (&(self->refcnt));
  
#endif
  return self;
}

//-----------------------------------------------------------------------------

void __STDCALL qwebs_prot_websocket_connection__on_switched (void* conn_ptr)
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
  
  cape_log_fmt (CAPE_LL_TRACE, "QWEBS", "websocket", "frame header: fin = %i, rsv1 = %i, rsv2 = %i, rsv3 = %i, opcode = %i", self->fin, self->rsv1, self->rsv2, self->rsv3, self->opcode);
}

//-----------------------------------------------------------------------------

void qwebs_prot_websocket_connection__decode_payload (QWebsProtWebsocketConnection self, CapeCursor cursor)
{
  number_t i;
  
  CapeString h = cape_cursor_scan_s (cursor, self->data_size);
  
  
  // handle some opcodes
  switch (self->opcode)
  {
    case RFC_WEBSOCKET_FRAME__TEXT:   // text frame
    {
      if (self->masking_key)
      {
        for (i = 0; i < self->data_size; i++)
        {
          h[i] = h[i] ^ self->masking_key[i % 4];
        }
      }
      
      // TODO: run this in a queue
      if (self->ws->on_msg)
      {
        self->ws->on_msg (self->conn_ptr, h);
      }
      
      break;
    }
    case RFC_WEBSOCKET_FRAME__CLOSED:   // connection close frame
    {
      cape_log_msg (CAPE_LL_DEBUG, "QWEBS", "payload", "retrieved connection closed");
      // TODO: close connection
      
      break;
    }
    case RFC_WEBSOCKET_FRAME__PING:   // ping
    {
      cape_log_msg (CAPE_LL_TRACE, "QWEBS", "payload", "retrieved PING request");

      qwebs_prot_websocket_send__frame (self, RFC_WEBSOCKET_FRAME__PONG, h, self->data_size);
      
      break;
    }
  }
  
  cape_str_del (&h);
}

//-----------------------------------------------------------------------------

void qwebs_prot_websocket_connection__adjust_buffer (QWebsProtWebsocketConnection self, CapeCursor cursor)
{
  // local objects
  CapeStream h = NULL;
  
  {
    // returns the bytes which had not been used for parsing
    number_t bytes_left_to_scan = cape_cursor_tail (cursor);
    
    cape_log_fmt (CAPE_LL_TRACE, "QWEBS", "on recv", "adjust buffer = %lu", bytes_left_to_scan);
    
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

void __STDCALL qwebs_prot_websocket_connection__on_recv (void* user_ptr, QWebsConnection conn, const char* bufdat, number_t buflen)
{
  QWebsProtWebsocketConnection self = user_ptr;
  int has_enogh_bytes_for_parsing = TRUE;
  
  // local objects
  CapeCursor cursor = cape_cursor_new ();
  
  cape_log_fmt (CAPE_LL_TRACE, "QWEBS", "websocket", "received buffer with len = %i", buflen);

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
      case QWEBS_PROT_WEBSOCKET_RECV__NONE:
      {
        if (cape_cursor__has_data (cursor, 2))
        {
          qwebs_prot_websocket__decode_header1 (self, cursor);
          
          self->state = QWEBS_PROT_WEBSOCKET_RECV__HEADER1;
          
          cape_log_fmt (CAPE_LL_TRACE, "QWEBS", "on recv", "payload length from header = %lu", self->data_size);
        }
        else
        {
          has_enogh_bytes_for_parsing = FALSE;
        }
        
        break;
      }
      case QWEBS_PROT_WEBSOCKET_RECV__HEADER1:
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
            has_enogh_bytes_for_parsing = FALSE;
          }
        }
        else if (self->data_size == 127)
        {
          if (cape_cursor__has_data (cursor, 8))
          {
            self->data_size = cape_cursor_scan_64 (cursor, TRUE);
            
            self->state = QWEBS_PROT_WEBSOCKET_RECV__LENGTH;
          }
          else
          {
            has_enogh_bytes_for_parsing = FALSE;
          }
        }
        else
        {
          self->state = QWEBS_PROT_WEBSOCKET_RECV__LENGTH;
        }
       
        break;
      }
      case QWEBS_PROT_WEBSOCKET_RECV__LENGTH:
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
            has_enogh_bytes_for_parsing = FALSE;
          }
        }
        else
        {
          self->state = QWEBS_PROT_WEBSOCKET_RECV__PAYLOAD;
        }
        
        break;
      }
      case QWEBS_PROT_WEBSOCKET_RECV__PAYLOAD:
      {        
        if (cape_cursor__has_data (cursor, self->data_size))
        {
          cape_log_fmt (CAPE_LL_TRACE, "QWEBS", "on recv", "payload length = %lu -> decode payload", self->data_size);
          
          // travers the cursor by self->data_size
          qwebs_prot_websocket_connection__decode_payload (self, cursor);
                    
          self->state = QWEBS_PROT_WEBSOCKET_RECV__NONE;
        }
        else
        {
          cape_log_fmt (CAPE_LL_TRACE, "QWEBS", "on recv", "payload length = %lu -> continue", self->data_size);

          has_enogh_bytes_for_parsing = FALSE;
        }
        
        break;
      }
    }
  }
  
  qwebs_prot_websocket_connection__adjust_buffer (self, cursor);
  
  cape_cursor_del (&cursor);
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

int qwebs_prot_websocket_reg (QWebsProtWebsocket self, QWebs qwebs, CapeErr err)
{
  // register a handling for websockets
  return qwebs_on_upgrade (qwebs, "websocket", self, qwebs_prot_websocket__on_upgrade, qwebs_prot_websocket_connection__on_switched, qwebs_prot_websocket_connection__on_recv, qwebs_prot_websocket_connection__on_del, err);
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

void qwebs_prot_websocket_dec (QWebsProtWebsocket* p_self, void** p_user_ptr)
{
  if (*p_self)
  {
    QWebsProtWebsocket self = *p_self;
    
    if (self->on_done)
    {
      self->on_done (*p_user_ptr);
      *p_user_ptr = NULL;
    }
    
    qwebs_prot_websocket_del (p_self);
  }
}

//-----------------------------------------------------------------------------
