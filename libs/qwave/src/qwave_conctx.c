#include "qwave_conctx.h"
#include "qwave.h"
#include "http_parser.h"
#include "qwave_reqctx.h"

// cape includes
#include <sys/cape_log.h>
#include <sys/cape_socket.h>
#include <sys/cape_file.h>
#include <stc/cape_stream.h>
#include <stc/cape_map.h>
#include <fmt/cape_tokenizer.h>
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

#define QWAVE_PROT_WEBSOCKET_RECV__NONE      0
#define QWAVE_PROT_WEBSOCKET_RECV__HEADER1   1
#define QWAVE_PROT_WEBSOCKET_RECV__LENGTH    2
#define QWAVE_PROT_WEBSOCKET_RECV__PAYLOAD   3

//-----------------------------------------------------------------------------

struct QWaveConctx_s
{
    QWaveConfig config;                       // reference
    QWaveResponse response;                   // reference
    CapeQueue queue;                          // reference
    QWaveAioctx aioctx;                       // reference
    
    number_t reference_counter;               // the reference counter for requests
    int close_connection;                     // tell the context to close the connection
    
    CapeStream buffer;                        // receive buffer
    
    QWaveAioctxEvent connection_handle;       // AIO event handle

    // http parser
    http_parser parser;                       // external parser instance
    http_parser_settings settings;            // external parser settings
    
    fct_qwave__on_upgrade on_upgrade;         // callback for upgrade
    int ws_state;                             // websockets protocol state
    number_t ws_data_size;                    // websocket packet size
    cape_uint8 ws_opcode;
    CapeString ws_masking_key;
    
    int ws_fin;
    int ws_rsv1;
    int ws_rsv2;
    int ws_rsv3;
    int ws_mask;
    
    void* ws_user_ptr;
    fct_qwave__on_ws_message ws_on_message;
};

//-----------------------------------------------------------------------------

/*
CapeMap qwave_conctx__internal__convert_query (const CapeString query)
{
    CapeMap ret = cape_map_new (NULL, qwave_conctx__intern__on_headers_del, NULL);
    
    CapeList values = cape_tokenizer_buf__noempty (query, cape_str_size (query), '&');
    
    CapeListCursor* cursor = cape_list_cursor_new (values, CAPE_DIRECTION_FORW);
    while (cape_list_cursor_next (cursor))
    {
        const CapeString value = cape_list_node_data (cursor->node);
        
        CapeString key = NULL;
        CapeString val = NULL;
        
        if (cape_tokenizer_split (value, '=', &key, &val))
        {
            cape_map_insert (ret, key, val);
        }
        else
        {
            cape_str_del (&key);
            cape_str_del (&val);
        }
    }
    
    cape_list_cursor_del (&cursor);
    
    cape_list_del (&values);
    return ret;
}
*/
//-----------------------------------------------------------------------------

static int qwave_conctx__internal__on_message_begin (http_parser* parser)
{
    qwave_reqctx_clr (parser->data);
    
    return 0;
}

//-----------------------------------------------------------------------------

static int qwave_conctx__internal__on_url (http_parser* parser, const char *at, size_t length)
{
    // copy url
    CapeString url = cape_str_sub (at, length);
    
    qwave_reqctx_set_url (parser->data, &url);
        
    return 0;
}

//-----------------------------------------------------------------------------

static int qwave_conctx__internal__on_header_field (http_parser* parser, const char *at, size_t length)
{
    // copy field
    CapeString field = cape_str_sub (at, length);

    qwave_reqctx_set_ohf (parser->data, &field);
    
    return 0;
}

//-----------------------------------------------------------------------------

static int qwave_conctx__internal__on_header_value (http_parser* parser, const char *at, size_t length)
{
    // copy value
    CapeString value = cape_str_sub (at, length);
    
    qwave_reqctx_set_ohv (parser->data, &value);
    
    return 0;
}

//-----------------------------------------------------------------------------

static int qwave_conctx__internal__on_body (http_parser* parser, const char* at, size_t length)
{
    QWaveReqctx self = parser->data;
    
    /*
     *  printf ("------ BODY ---------------------------------------------------------------------\n");
     *  printf ("%.*s\n", (int)length, at);
     */
    
    /*
    if (self->api)
    {
        cape_stream_append_buf (self->body_value, at, length);
    }
    */
    
    return 0;
}

//-----------------------------------------------------------------------------

static int qwave_conctx__internal__on_message_complete (http_parser* parser)
{
    qwave_reqctx_set_complete (parser->data);
      
    return 0;
}

//-----------------------------------------------------------------------------

QWaveConctx qwave_conctx_new (QWaveConfig config, QWaveResponse response, CapeQueue queue, QWaveAioctx aio, QWaveAioctxEvent event, fct_qwave__on_upgrade on_upgrade)
{
    QWaveConctx self = CAPE_NEW (struct QWaveConctx_s);
    
    self->config = config;
    self->response =response;
    self->queue = queue;
    self->aioctx = aio;
    
    self->reference_counter = 0;
    self->close_connection = FALSE;
    
    self->buffer = cape_stream_new ();
    
    self->connection_handle = event;

    self->on_upgrade = on_upgrade;
    self->ws_state = QWAVE_PROT_WEBSOCKET_RECV__NONE;
    self->ws_masking_key = NULL;
    
    http_parser_init (&(self->parser), HTTP_REQUEST);
    
    // initialize the HTTP parser
    http_parser_settings_init (&(self->settings));
    
    // set some callbacks
    self->settings.on_message_begin = qwave_conctx__internal__on_message_begin;
    self->settings.on_url = qwave_conctx__internal__on_url;
    self->settings.on_status = NULL;
    self->settings.on_header_field = qwave_conctx__internal__on_header_field;
    self->settings.on_header_value = qwave_conctx__internal__on_header_value;
    self->settings.on_headers_complete = NULL;
    self->settings.on_body = qwave_conctx__internal__on_body;
    self->settings.on_message_complete = qwave_conctx__internal__on_message_complete;
    self->settings.on_chunk_header = NULL;
    self->settings.on_chunk_complete = NULL;
    
    self->parser.data = NULL;
        
    self->ws_user_ptr = NULL;
    self->ws_on_message = NULL;
    
    return self;
}

//-----------------------------------------------------------------------------

void qwave_conctx_del (QWaveConctx* p_self)
{
    if (*p_self)
    {
        QWaveConctx self = *p_self;
        
        cape_str_del (&(self->ws_masking_key));
        cape_stream_del (&(self->buffer));
        
        CAPE_DEL (p_self, struct QWaveConctx_s);
    }
}

//-----------------------------------------------------------------------------

QWaveConctx qwave_conctx_reqinc (QWaveConctx self)
{
    self->reference_counter++;
    
    return self;
}

//-----------------------------------------------------------------------------

void qwave_conctx_shutdown (QWaveConctx self)
{
    if ((self->reference_counter == 0) && (self->close_connection))
    {
        QWaveAioctxEvent event = self->connection_handle;

        // close write part of the socket
        cape_sock__shutdown (qwave_aioctx_event_get (self->connection_handle));

        // self will be destroyed in the process
        qwave_aioctx_rm (self->aioctx, &event);
    }
}

//-----------------------------------------------------------------------------

void qwave_conctx_reqdec (QWaveConctx self)
{
    self->reference_counter--;

    cape_sock__touch (qwave_aioctx_event_get (self->connection_handle), NULL);
}

//-----------------------------------------------------------------------------

void qwave_conctx_close (QWaveConctx self)
{
    self->close_connection = TRUE;
    
    //cape_sock__shutdown__rd (qwave_aioctx_event_get (self->connection_handle));
    
    qwave_conctx_shutdown (self);
}

//-----------------------------------------------------------------------------

void __STDCALL qwave_conctx__on_event (void* ptr, number_t pos, number_t queue_size)
{
    QWaveReqctx request_context = ptr;

    qwave_reqctx_exec (request_context);

    qwave_reqctx_dec (&request_context);
}

//-----------------------------------------------------------------------------

int qwave_conctx_read (QWaveConctx self)
{    
    int ret = TRUE;
    int con = TRUE;
    
    // local objects
    CapeErr err = cape_err_new ();
        
    while (con)
    {
        switch (cape_sock__recv (qwave_aioctx_event_get (self->connection_handle), self->buffer, 1024, err))
        {
            case CAPE_ERR_NONE:
            {
                if (NULL == self->parser.data)
                {
                    // create a new request object to track this request
                    self->parser.data = qwave_reqctx_new (qwave_conctx_reqinc (self), self->config);
                }
                
                size_t parsed_bytes = http_parser_execute (&(self->parser), &(self->settings), cape_stream_data (self->buffer), cape_stream_size (self->buffer));
                
                if (self->parser.http_errno > 0)
                {
                    CapeString h = cape_str_catenate_3 (http_errno_name (self->parser.http_errno), " : ", http_errno_description ((enum http_errno)self->parser.http_errno));
                    
                    cape_log_fmt (CAPE_LL_ERROR, "QWAVE", "read", "parser returned an error [%i]: %s", self->parser.http_errno, h);
                    
                    cape_str_del (&h);
                    
                    con = FALSE;
                    ret = FALSE;
                }
                   
                if (qwave_reqctx_is_complete (self->parser.data))
                {
                    int keep_alive = http_should_keep_alive (&(self->parser));
                    
                    qwave_reqctx_set (self->parser.data, self->parser.upgrade, keep_alive, http_method_str (self->parser.method));
                    
                    cape_stream_shift_l (self->buffer, parsed_bytes);
                                          
                    cape_queue_add (self->queue, NULL, qwave_conctx__on_event, NULL, NULL, self->parser.data, 0);
                    
                    self->parser.data = NULL;
                }
                   
                /*
                if (http_body_is_final (&(self->parser)))
                {
                    cape_log_fmt (CAPE_LL_TRACE, "QWAVE", "read", "parser finished");

                    con = FALSE;
                    ret = TRUE;                    
                }
                */

                break;
            }
            case CAPE_ERR_EOF:
            {
                cape_log_fmt (CAPE_LL_TRACE, "QWAVE", "read", "connection shutdown detected [%li]", qwave_aioctx_event_get (self->connection_handle));
                
                ret = FALSE;
                con = FALSE;
                break;
            }            
            case CAPE_ERR_CONTINUE:
            {            
                con = FALSE;
                break;
            }
            default:
            {
                ret = FALSE;
                con = FALSE;
                break;
            }
        }
    }
        
    cape_err_del (&err);
    
    return ret;
}

//-----------------------------------------------------------------------------

void qwave_conctx_send (QWaveConctx self, CapeStream* p_output)
{
    int res;
    
    // local objects
    CapeErr err = cape_err_new ();
    CapeStream s = cape_stream_mv (p_output);
    
    //printf ("send file #1 [%i]\n", (int)(number_t)qwave_aioctx_event_get (self->connection_handle));
    
    res = cape_sock__send (qwave_aioctx_event_get (self->connection_handle), s, err);
    if (res)
    {
        
        
    }

    //printf ("send file #2 [%i], %i -> %li\n", (int)(number_t)qwave_aioctx_event_get (self->connection_handle), res, cape_stream_size (s));
    
    cape_stream_del (&s);
    cape_err_del (&err);
}

//-----------------------------------------------------------------------------

void qwave_conctx_send_file (QWaveConctx self, const CapeString site, const CapeString path, int keep_alive)
{
    // local objects
    CapeErr err = cape_err_new ();
    CapeString file_absolute = NULL;
    CapeString file_relative = NULL;
    
    // construct the relative path
    file_relative = cape_fs_path_merge (site, path);
    
    // construct the absolute path
    file_absolute = cape_fs_path_rebuild (file_relative, err);
    
    if (NULL == file_absolute)
    {
        
    }
    else
    {
        CapeStream s = cape_stream_new ();
        
        cape_log_fmt (CAPE_LL_TRACE, "QWAVE", "read file", "path: %s", file_absolute);
        
        // this will fillup the stream with a valid http response
        qwave_response_file (self->response, s, file_absolute, keep_alive);

        // send the response to the client (browser)
        qwave_conctx_send (self, &s);
    }
    
    cape_str_del (&file_relative);
    cape_str_del (&file_absolute);
    cape_err_del (&err);    
}

//-----------------------------------------------------------------------------

void qwave_conctx_upgrade (QWaveConctx self, const CapeString key)
{
    // local objects
    CapeErr err = cape_err_new ();
    CapeString accept_key__text = NULL;
    CapeStream accept_key__hash = NULL;
    CapeString accept_key = NULL;
    
    // see RFC, concat the defined UUID as accept key
    accept_key__text = cape_str_catenate_2 (key, "258EAFA5-E914-47DA-95CA-C5AB0DC85B11");
    
    // hash the accept key
    accept_key__hash = qcrypt__hash_sha__bin_o  (accept_key__text, cape_str_size (accept_key__text), err);
    
    if (NULL == accept_key__hash)
    {
        cape_log_msg (CAPE_LL_WARN, "QWEBS", "on upgrade", "can't create hash for the accept-key");
        
    }
    
    accept_key = qcrypt__encode_base64_m (accept_key__hash);
    
    {
        CapeStream s = cape_stream_new ();
        
        qwave_response_upgrade (self->response, s, accept_key);
        
        // send the response to the client (browser)
        qwave_conctx_send (self, &s);
    }

    // reset values
    self->ws_state = QWAVE_PROT_WEBSOCKET_RECV__NONE;
    cape_stream_clr (self->buffer);
    
    // reset the callbacks
    if (self->on_upgrade)
    {
        self->on_upgrade (self, self->connection_handle);
    }
    
    cape_str_del (&accept_key);
    cape_stream_del (&accept_key__hash);
    cape_str_del (&accept_key__text);
    cape_err_del (&err);
}

//-----------------------------------------------------------------------------

void qwave_conctx_ws__decode_header1 (QWaveConctx self, CapeCursor cursor)
{
    {
        cape_uint8 bits01 = cape_cursor_scan_08 (cursor);
        
        self->ws_fin  = (bits01 & 0B10000000) == 0B10000000;
        self->ws_rsv1 = (bits01 & 0B01000000) == 0B01000000;
        self->ws_rsv2 = (bits01 & 0B00100000) == 0B00100000;
        self->ws_rsv3 = (bits01 & 0B00010000) == 0B00010000;
        
        self->ws_opcode = (bits01 << 4);
        self->ws_opcode = self->ws_opcode >> 4;
    }
    {
        cape_uint8 bits02 = cape_cursor_scan_08 (cursor);
        
        self->ws_mask = (bits02 & 0B10000000) == 0B10000000;
        self->ws_data_size = bits02 & ~0B10000000;
    }
    
    //cape_log_fmt (CAPE_LL_TRACE, "QWEBS", "websocket", "frame header: fin = %i, rsv1 = %i, rsv2 = %i, rsv3 = %i, opcode = %i", self->ws_fin, self->ws_rsv1, self->ws_rsv2, self->ws_rsv3, self->ws_opcode);
}

//-----------------------------------------------------------------------------

void qwave_conctx_ws__send_frame (QWaveConctx self, number_t opcode, const char* bufdat, number_t buflen)
{
    // local objects
    CapeStream s = cape_stream_new ();
    number_t size_type = 0;
    
    /* the server is not allowed to send masked payload
     * -> mask was set to 0
     */
    
    //cape_log_fmt (CAPE_LL_TRACE, "QWEBS", "send frame", "buflen = %lu", buflen);
    
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
    
    qwave_conctx_send (self, &s);
}

//-----------------------------------------------------------------------------

void qwave_conctx_ws__decode_payload (QWaveConctx self, CapeCursor cursor)
{
    number_t i;
    
    CapeString h = cape_cursor_scan_s (cursor, self->ws_data_size);
    
    // handle some opcodes
    switch (self->ws_opcode)
    {
        case RFC_WEBSOCKET_FRAME__TEXT:   // text frame
        {
            if (self->ws_masking_key)
            {
                for (i = 0; i < self->ws_data_size; i++)
                {
                    h[i] = h[i] ^ self->ws_masking_key[i % 4];
                }
            }
            
            // TODO: run this in a queue
            if (self->ws_on_message)
            {
                // call the user defined on message method
                self->ws_on_message (self->ws_user_ptr, h, self->ws_data_size);
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
            //cape_log_msg (CAPE_LL_TRACE, "QWEBS", "payload", "retrieved PING request");
            
            qwave_conctx_ws__send_frame (self, RFC_WEBSOCKET_FRAME__PONG, h, self->ws_data_size);
            
            break;
        }
    }
    
    cape_str_del (&h);
}

//-----------------------------------------------------------------------------

void qwave_conctx_ws__adjust_buffer (QWaveConctx self, CapeCursor cursor)
{
    // local objects
    CapeStream h = NULL;
    
    {
        // returns the bytes which had not been used for parsing
        number_t bytes_left_to_scan = cape_cursor_tail (cursor);
        
        //cape_log_fmt (CAPE_LL_TRACE, "QWEBS", "on recv", "adjust buffer = %lu", bytes_left_to_scan);
        
        if (bytes_left_to_scan > 0)
        {
            h = cape_stream_new ();
            
            // shift the buffer
            // travers the cursor (to the end)
            cape_stream_append_buf (h, cape_cursor_tpos (cursor, bytes_left_to_scan), bytes_left_to_scan);

            // replace the buffer
            cape_stream_replace_mv (&(self->buffer), &h);
        }
        else
        {
            cape_stream_clr (self->buffer);
        }
    }    
}

//-----------------------------------------------------------------------------

void qwave_conctx_ws__handle_protocol (QWaveConctx self, CapeCursor cursor)
{
    int has_enogh_bytes_for_parsing = TRUE;
    
    while (has_enogh_bytes_for_parsing)
    {
        switch (self->ws_state)
        {
            case QWAVE_PROT_WEBSOCKET_RECV__NONE:
            {
                if (cape_cursor__has_data (cursor, 2))
                {
                    qwave_conctx_ws__decode_header1 (self, cursor);
                    
                    self->ws_state = QWAVE_PROT_WEBSOCKET_RECV__HEADER1;
                    
                    //cape_log_fmt (CAPE_LL_TRACE, "QWEBS", "on recv", "payload length from header = %lu", self->ws_data_size);
                }
                else
                {
                    has_enogh_bytes_for_parsing = FALSE;
                }
                
                break;
            }
            case QWAVE_PROT_WEBSOCKET_RECV__HEADER1:
            {
                if (self->ws_data_size == 126)
                {
                    if (cape_cursor__has_data (cursor, 2))
                    {
                        self->ws_data_size = cape_cursor_scan_16 (cursor, TRUE);
                        
                        self->ws_state = QWAVE_PROT_WEBSOCKET_RECV__LENGTH;
                    }
                    else
                    {
                        has_enogh_bytes_for_parsing = FALSE;
                    }
                }
                else if (self->ws_data_size == 127)
                {
                    if (cape_cursor__has_data (cursor, 8))
                    {
                        self->ws_data_size = cape_cursor_scan_64 (cursor, TRUE);
                        
                        self->ws_state = QWAVE_PROT_WEBSOCKET_RECV__LENGTH;
                    }
                    else
                    {
                        has_enogh_bytes_for_parsing = FALSE;
                    }
                }
                else
                {
                    self->ws_state = QWAVE_PROT_WEBSOCKET_RECV__LENGTH;
                }
                
                break;
            }
            case QWAVE_PROT_WEBSOCKET_RECV__LENGTH:
            {
                if (self->ws_mask)
                {
                    if (cape_cursor__has_data (cursor, 4))
                    {
                        cape_str_del (&(self->ws_masking_key));
                        self->ws_masking_key = cape_cursor_scan_s (cursor, 4);
                        
                        self->ws_state = QWAVE_PROT_WEBSOCKET_RECV__PAYLOAD;
                    }
                    else
                    {
                        has_enogh_bytes_for_parsing = FALSE;
                    }
                }
                else
                {
                    self->ws_state = QWAVE_PROT_WEBSOCKET_RECV__PAYLOAD;
                }
                
                break;
            }
            case QWAVE_PROT_WEBSOCKET_RECV__PAYLOAD:
            {        
                if (cape_cursor__has_data (cursor, self->ws_data_size))
                {
                    //cape_log_fmt (CAPE_LL_TRACE, "QWEBS", "on recv", "payload length = %lu -> decode payload", self->ws_data_size);
                    
                    // travers the cursor by self->data_size
                    qwave_conctx_ws__decode_payload (self, cursor);
                    
                    self->ws_state = QWAVE_PROT_WEBSOCKET_RECV__NONE;
                }
                else
                {
                    //cape_log_fmt (CAPE_LL_TRACE, "QWEBS", "on recv", "payload length = %lu -> continue", self->ws_data_size);
                    
                    has_enogh_bytes_for_parsing = FALSE;
                }
                
                break;
            }
        }
    }

    qwave_conctx_ws__adjust_buffer (self, cursor);
}

//-----------------------------------------------------------------------------

void qwave_conctx_ws_cb (QWaveConctx self, void* user_ptr, fct_qwave__on_ws_upgrade on_upgrade, fct_qwave__on_ws_message on_message, const CapeString remote_address)
{
    if (on_upgrade)
    {
        // the method will return the connection user pointer
        self->ws_user_ptr = on_upgrade (user_ptr, self, remote_address);
    }

    self->ws_on_message = on_message;
}

//-----------------------------------------------------------------------------

void qwave_conctx_ws_read (QWaveConctx self)
{
    int read = TRUE;
    
    // local objects
    CapeErr err = cape_err_new ();
    CapeCursor cursor = cape_cursor_new ();
    
    while (read)
    {
        switch (cape_sock__recv (qwave_aioctx_event_get (self->connection_handle), self->buffer, 1024, err))
        {
            case CAPE_ERR_NONE:
            {
                // use the current buffer for the cursor
                cape_cursor_set (cursor, cape_stream_data (self->buffer), cape_stream_size (self->buffer));
                
                // use the cursor to handle the protocol
                qwave_conctx_ws__handle_protocol (self, cursor);
                
                break;
            }
            case CAPE_ERR_EOF:
            {
                cape_log_fmt (CAPE_LL_TRACE, "QWAVE", "read", "connection shutdown detected [%li]", qwave_aioctx_event_get (self->connection_handle));
                
                read = FALSE;
                break;
            }            
            case CAPE_ERR_CONTINUE:
            {
                read = FALSE;
                break;
            }
            default:
            {
                read = FALSE;
                break;
            }
        }
    }

    cape_cursor_del (&cursor);
    cape_err_del (&err);
}

//-----------------------------------------------------------------------------

void qwave_conctx_ws_send (QWaveConctx self, const char* bufdat, number_t buflen)
{
    qwave_conctx_ws__send_frame (self, RFC_WEBSOCKET_FRAME__TEXT, bufdat, buflen);
}

//-----------------------------------------------------------------------------
