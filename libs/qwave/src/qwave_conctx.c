#include "qwave_conctx.h"
#include "qwave.h"
#include "http_parser.h"

// cape includes
#include <sys/cape_log.h>
#include <stc/cape_stream.h>
#include <stc/cape_map.h>
#include <fmt/cape_tokenizer.h>

//-----------------------------------------------------------------------------

struct QWaveConctx_s
{
    QWaveConfig config;                       // reference
    
    CapeString remote_address;
    CapeStream buffer;

    // http parser
    http_parser parser;
    http_parser_settings settings;
    
    // for parsing: temporary string to remember the last header field
    CapeString last_header_field;
    
    CapeString site;
    CapeString url;
    CapeString api;
    
    CapeMap header_values;
    CapeMap query_values;
    
    CapeList url_values;
    
};

//-----------------------------------------------------------------------------

static void __STDCALL qwave_conctx__intern__on_headers_del (void* key, void* val)
{
    {
        CapeString h = key; cape_str_del (&h);
    }
    {
        CapeString h = val; cape_str_del (&h);
    }
}

//-----------------------------------------------------------------------------

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

//-----------------------------------------------------------------------------

static int qwave_conctx__internal__on_message_begin (http_parser* parser)
{
    QWaveConctx self = parser->data;
    
    cape_str_del (&(self->last_header_field));

    cape_str_del (&(self->site));
    cape_str_del (&(self->url));
    cape_str_del (&(self->api));
    
    cape_map_clr (self->header_values);
    cape_map_del (&(self->query_values));
    
    cape_list_del (&(self->url_values));
    
    return 0;
}

//-----------------------------------------------------------------------------

static int qwave_conctx__internal__on_url (http_parser* parser, const char *at, size_t length)
{
    QWaveConctx self = parser->data;
    
    // this will re-write the url
    // -> checks all sites for re-writing
    self->site = qwave_config_site (self->config, at, length, &(self->url));
    
    cape_log_fmt (CAPE_LL_TRACE, "QWAVE", "parse", "access: %s", self->url);
    
    if ('/' == *(self->url))
    {
        CapeString url = NULL;
        CapeString query = NULL;
        
        if (cape_tokenizer_split (self->url, '?', &url, &query))
        {
            cape_str_replace_mv (&(self->url), &url);
            
            cape_map_del (&(self->query_values));
            
            // parse the options into a map
            self->query_values = qwave_conctx__internal__convert_query (query);
            
            cape_str_del (&query);
        }
        
        // split the url into its parts
        self->url_values = cape_tokenizer_buf__noempty (self->url + 1, cape_str_size (self->url) - 1, '/');
        
        if (cape_list_size (self->url_values) >= 1)
        {
            CapeListNode n = cape_list_node_front (self->url_values);
            
            // get the first part
            const CapeString first_part = cape_list_node_data (n);
            
            if (qwave_config_route (self->config, first_part))
            {
                cape_str_replace_cp (&(self->url), "/index.html");
            }
            else if (cape_list_size (self->url_values) >= 2)
            {
                // anaylse the URL if we have an API or not
       //         self->api = qwebs_get_api (self->webs, first_part);
                
                if (self->api)
                {
                    // reduce the url values by one
                    {
                        CapeListNode n = cape_list_node_front (self->url_values);
                        
                        cape_list_node_erase (self->url_values, n);
                    }
                    
                   // self->body_value = cape_stream_new ();
                }
            }
            else
            {
   //             self->api = qwebs_get_page (self->webs, self->url + 1);
            }
        }
        else
        {
            if (cape_str_equal ("/", self->url))
            {
                cape_str_replace_cp (&(self->url), "/index.html");
            }
            
   //         self->api = qwebs_get_page (self->webs, self->url);
        }
    }
    
    return 0;
}

//-----------------------------------------------------------------------------

static int qwave_conctx__internal__on_header_field (http_parser* parser, const char *at, size_t length)
{
    QWaveConctx self = parser->data;
    
    if (self->header_values)
    {
        CapeString h = cape_str_sub (at, length);
        
        cape_str_replace_mv (&(self->last_header_field), &h);
    }
    
    return 0;
}

//-----------------------------------------------------------------------------

static int qwave_conctx__internal__on_header_value (http_parser* parser, const char *at, size_t length)
{
    QWaveConctx self = parser->data;
    
    if (self->header_values)
    {
        if (self->last_header_field)
        {
            CapeString h = cape_str_sub (at, length);
            // printf ("HEADER VALUE: %s = %s\n", self->last_header_field, h);
            
            // transfer ownership to the map
            cape_map_insert (self->header_values, self->last_header_field, h);
            self->last_header_field = NULL;
        }
    }
    
    return 0;
}

//-----------------------------------------------------------------------------

static int qwave_conctx__internal__on_body (http_parser* parser, const char* at, size_t length)
{
    QWaveConctx self = parser->data;
    
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
    QWaveConctx self = parser->data;
    
    
    return 0;
}

//-----------------------------------------------------------------------------

QWaveConctx qwave_conctx_new (QWaveConfig config, const CapeString remote_address)
{
    QWaveConctx self = CAPE_NEW (struct QWaveConctx_s);
    
    self->config = config;
    
    self->remote_address = cape_str_cp (remote_address);
    self->buffer = cape_stream_new ();
    
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
    
    self->parser.data = self;
    
    self->site = NULL;
    self->url = NULL;
    
    self->header_values = cape_map_new (NULL, qwave_conctx__intern__on_headers_del, NULL);
    self->query_values = NULL;
    
    return self;
}

//-----------------------------------------------------------------------------

void qwave_conctx_del (QWaveConctx* p_self)
{
    if (*p_self)
    {
        QWaveConctx self = *p_self;

        cape_str_del (&(self->site));
        cape_str_del (&(self->url));

        cape_map_del (&(self->query_values));
        cape_map_del (&(self->header_values));
        
        cape_stream_del (&(self->buffer));
        cape_str_del (&(self->remote_address));
        
        CAPE_DEL (p_self, struct QWaveConctx_s);
    }
}

//-----------------------------------------------------------------------------

#ifdef __WINDOWS_OS

#elif defined __LINUX_OS

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

//-----------------------------------------------------------------------------

void qwave_conctx_read (QWaveConctx self, void* handle)
{
    cape_log_fmt (CAPE_LL_DEBUG, "QWAVE", "request", "new request on fd [%lu]", handle);
    
    number_t bytes_read;

    while (TRUE)
    {
        // reserve 1024 bytes
        cape_stream_cap (self->buffer, 1024);
        
        // try to read bytes from FD
        bytes_read = read((number_t)handle, cape_stream_pos (self->buffer), 1024);
        if(bytes_read == -1)
        {
            if (errno != EAGAIN)
            {
                
                
            }
            
            return;
        }
        else if (bytes_read == 0)
        {
            
            break;
        }
        
        cape_stream_set (self->buffer, bytes_read);
        
        //int bytes_processed = http_parser_execute (&(self->parser), &(self->settings), bufdat, buflen);
        http_parser_execute (&(self->parser), &(self->settings), cape_stream_data (self->buffer), cape_stream_size (self->buffer));

        if (self->parser.http_errno > 0)
        {
            CapeString h = cape_str_catenate_3 (http_errno_name (self->parser.http_errno), " : ", http_errno_description ((enum http_errno)self->parser.http_errno));
            
            cape_log_fmt (CAPE_LL_ERROR, "QWAVE", "read", "parser returned an error [%i]: %s", self->parser.http_errno, h);
            
            cape_str_del (&h);
            
            // close it
            return;
        }
        
        if (http_body_is_final (&(self->parser)))
        {
            return;
        }
    }
        
}

//-----------------------------------------------------------------------------

#endif

//-----------------------------------------------------------------------------
