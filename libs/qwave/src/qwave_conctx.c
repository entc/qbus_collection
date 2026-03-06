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

//-----------------------------------------------------------------------------

struct QWaveConctx_s
{
    QWaveConfig config;                       // reference
    QWaveResponse response;                   // reference
    CapeQueue queue;                          // reference
    
    number_t reference_counter;               // the reference counter for requests
    int close_connection;                     // tell the context to close the connection
    
    CapeString remote_address;                // remote address as string
    CapeStream buffer;                        // receive buffer
    
    QWaveAioctxEvent connection_handle;       // AIO event handle

    // http parser
    http_parser parser;                       // external parser instance
    http_parser_settings settings;            // external parser settings
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

QWaveConctx qwave_conctx_new (QWaveConfig config, QWaveResponse response, CapeQueue queue, QWaveAioctxEvent event, const CapeString remote_address)
{
    QWaveConctx self = CAPE_NEW (struct QWaveConctx_s);
    
    self->config = config;
    self->response =response;
    self->queue = queue;
    
    self->reference_counter = 0;
    self->close_connection = FALSE;
    
    self->remote_address = cape_str_cp (remote_address);
    self->buffer = cape_stream_new ();
    
    self->connection_handle = event;
    
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
        
    return self;
}

//-----------------------------------------------------------------------------

void qwave_conctx_del (QWaveConctx* p_self)
{
    if (*p_self)
    {
        QWaveConctx self = *p_self;
        
        cape_stream_del (&(self->buffer));
        cape_str_del (&(self->remote_address));
        
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
        QWaveConctx h = self;
        
        cape_sock__close (qwave_aioctx_event_get (self->connection_handle));

   //     qwave_aioctx_rm (QWaveAioctx, self->connection_handle);
                
        qwave_conctx_del (&h);
    }
}

//-----------------------------------------------------------------------------

void qwave_conctx_reqdec (QWaveConctx self)
{
    self->reference_counter--;

    qwave_conctx_shutdown (self);
}

//-----------------------------------------------------------------------------

void qwave_conctx_close (QWaveConctx self)
{
    self->close_connection = TRUE;

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
                    qwave_reqctx_set (self->parser.data, self->parser.upgrade, http_should_keep_alive (&(self->parser)), http_method_str (self->parser.method));
                    
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
                cape_log_fmt (CAPE_LL_TRACE, "QWAVE", "read", "connection shutdown detected");
                
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

void qwave_conctx_send (QWaveConctx self, CapeStream* p_output, int keep_alive)
{
    int res;
    
    // local objects
    CapeErr err = cape_err_new ();
    CapeStream s = cape_stream_mv (p_output);
    
    printf ("send file #1\n");
    
    res = cape_sock__send (qwave_aioctx_event_get (self->connection_handle), s, err);
    if (res)
    {
        
        
    }

    printf ("send file #2, %i\n", res);
    
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
        qwave_conctx_send (self, &s, keep_alive);
    }
    
    cape_str_del (&file_relative);
    cape_str_del (&file_absolute);
    cape_err_del (&err);    
}

//-----------------------------------------------------------------------------
