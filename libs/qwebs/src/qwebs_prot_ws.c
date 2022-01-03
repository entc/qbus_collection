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

struct QWebsProtWebsocketConnection_s
{
  QWebsProtWebsocket ws;
  void* conn_ptr;
  
}; typedef struct QWebsProtWebsocketConnection_s* QWebsProtWebsocketConnection;

//-----------------------------------------------------------------------------

struct QWebsProtWebsocket_s
{
  QWebs qwebs;
  
  void* user_ptr;
  fct_qwebs_prot_websocket__on_conn on_conn;
  fct_qwebs_prot_websocket__on_msg on_msg;
};

//-----------------------------------------------------------------------------

QWebsProtWebsocket qwebs_prot_websocket_new (QWebs qwebs)
{
  QWebsProtWebsocket self = CAPE_NEW (struct QWebsProtWebsocket_s);
  
  self->qwebs = qwebs;
  
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
  
  ret = CAPE_NEW (struct QWebsProtWebsocketConnection_s);
  
  ret->ws = self;
  ret->conn_ptr = NULL;
  
  if (self->on_conn)
  {
    ret->conn_ptr = self->on_conn (self->user_ptr);
  }
  
exit_and_cleanup:
  
  return ret;
}

//-----------------------------------------------------------------------------

void __STDCALL qwebs_prot_websocket__on_recv (void* user_ptr, QWebsConnection conn, const char* bufdat, number_t buflen)
{
  QWebsProtWebsocketConnection self = user_ptr;

  // local objects
  CapeCursor cursor = cape_cursor_new();
  
  cape_cursor_set (cursor, bufdat, buflen);
  
  if (cape_cursor__has_data (cursor, 16))
  {
    CapeString masking_key = NULL;
    
    cape_uint8 bits01 = cape_cursor_scan_08 (cursor);
    
    int fin  = (bits01 & 0B10000000) == 0B10000000;
    int rsv1 = (bits01 & 0B01000000) == 0B01000000;
    int rsv2 = (bits01 & 0B00100000) == 0B00100000;
    int rsv3 = (bits01 & 0B00010000) == 0B00010000;
    
    cape_uint8 bits02 = cape_cursor_scan_08 (cursor);
    
    int mask = (bits02 & 0B10000000) == 0B10000000;
    
    cape_uint8 frlen = bits02 & ~0B10000000;
    
    if (frlen == 126)
    {
      cape_uint16 bits03 = cape_cursor_scan_16 (cursor, TRUE);
      
    }
    else if (frlen == 127)
    {
      cape_uint16 bits03 = cape_cursor_scan_16 (cursor, TRUE);
      cape_uint32 bits04 = cape_cursor_scan_32 (cursor, TRUE);
      
      
    }
    
    
    if (mask)
    {
      masking_key = cape_cursor_scan_s (cursor, 4);
      
      cape_log_fmt (CAPE_LL_TRACE, "AREA", "on recv", "with masking key");
    }
    
    
    cape_uint8 opcode = (bits01 << 4);
    
    opcode = opcode >> 4;
    
    cape_log_fmt (CAPE_LL_TRACE, "AREA", "on recv", "got something: bits01 = %i, bits02 = %i, fin = %i, rsv1 = %i, rsv2 = %i, rsv3 = %i, opcode = %i, len = %i", bits01, bits02, fin, rsv1, rsv2, rsv3, opcode, frlen);
    
    if (cape_cursor__has_data (cursor, frlen))
    {
      number_t i;
      
      CapeString h = cape_cursor_scan_s (cursor, frlen);
      
      if (masking_key)
      {
        for (i = 0; i < frlen; i++)
        {
          h[i] = h[i] ^ masking_key[i % 4];
        }
      }
      
      if (self->ws->on_msg)
      {
        self->ws->on_msg (self->conn_ptr, h);
      }
    }
    
  }
  
  cape_cursor_del (&cursor);
}

//-----------------------------------------------------------------------------

void __STDCALL qwebs_prot_websocket__on_del (void** p_user_ptr, QWebsConnection conn)
{
  QWebsProtWebsocketConnection* p_self = (QWebsProtWebsocketConnection*)p_user_ptr;
  
  cape_log_fmt (CAPE_LL_DEBUG, "AREA", "on upgrade", "websocket connection dropped");
  
  CAPE_DEL (p_self, struct QWebsProtWebsocketConnection_s);
}

//-----------------------------------------------------------------------------

int qwebs_prot_websocket_init (QWebsProtWebsocket self, CapeErr err)
{
  // register a handling for websockets
  return qwebs_on_upgrade (self->qwebs, "websocket", self, qwebs_prot_websocket__on_upgrade, qwebs_prot_websocket__on_recv, qwebs_prot_websocket__on_del, err);
}

//-----------------------------------------------------------------------------

void qwebs_prot_websocket_cb (QWebsProtWebsocket self, void* user_ptr, fct_qwebs_prot_websocket__on_conn on_conn, fct_qwebs_prot_websocket__on_msg on_msg)
{
  self->user_ptr = user_ptr;
  
  self->on_conn = on_conn;
  self->on_msg = on_msg;
}

//-----------------------------------------------------------------------------
