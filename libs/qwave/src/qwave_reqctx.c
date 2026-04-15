#include "qwave_reqctx.h"
#include "qwave_response.h"

// cape includes
#include <sys/cape_log.h>
#include <fmt/cape_tokenizer.h>

//-----------------------------------------------------------------------------

struct QWaveReqctx_s
{
    QWaveConctx conctx;                      // reference
    QWaveConfig config;                      // reference
  
    int upgrade;
    int keep_alive;
  
    CapeMap header_values;
    CapeMap query_values;
    
    CapeString url;
    CapeString method;
    
    // used for the parsing process
    int complete;
    CapeString last_header_field;
};

//-----------------------------------------------------------------------------

QWaveReqctx qwave_reqctx_new (QWaveConctx conctx, QWaveConfig config)
{
    QWaveReqctx self = CAPE_NEW (struct QWaveReqctx_s);
    
    self->conctx = conctx;
    self->config = config;
    
    self->complete = FALSE;
    self->upgrade = FALSE;
    self->keep_alive = FALSE;
        
    self->header_values = NULL;
    self->query_values = NULL;
    
    self->url = NULL;
    self->method = NULL;
    
    return self;
}

//-----------------------------------------------------------------------------

void qwave_reqctx_dec (QWaveReqctx* p_self)
{
    if (*p_self)
    {
        QWaveReqctx self = *p_self;
        
        cape_str_del (&(self->url));
        cape_str_del (&(self->method));
        
        cape_map_del (&(self->query_values));
        cape_map_del (&(self->header_values));
        
        CAPE_DEL (p_self, struct QWaveReqctx_s);
    }    
}

//-----------------------------------------------------------------------------

static void __STDCALL qwave_reqctx__intern__on_headers_del (void* key, void* val)
{
    {
        CapeString h = key; cape_str_del (&h);
    }
    {
        CapeString h = val; cape_str_del (&h);
    }
}

//-----------------------------------------------------------------------------

void qwave_reqctx_clr (QWaveReqctx self)
{
    // initialize the complete status
    self->complete = FALSE;
    
    cape_str_del (&(self->last_header_field));
    cape_str_del (&(self->url));
    
    // remove old values
    cape_map_del (&(self->header_values));
    cape_map_del (&(self->query_values));
    
    // create a new map
    self->header_values = cape_map_new (NULL, qwave_reqctx__intern__on_headers_del, NULL);
}

//-----------------------------------------------------------------------------

void qwave_reqctx_set_url (QWaveReqctx self, CapeString* p_url)
{
    cape_str_replace_mv (&(self->url), p_url);
}

//-----------------------------------------------------------------------------

void qwave_reqctx_set_ohf (QWaveReqctx self, CapeString* p_field)
{
    // replace the last header field
    cape_str_replace_mv (&(self->last_header_field), p_field);
}

//-----------------------------------------------------------------------------

void qwave_reqctx_set_ohv (QWaveReqctx self, CapeString* p_value)
{
    if (self->last_header_field)
    {
        // transfer ownership to the map
        cape_map_insert (self->header_values, cape_str_mv (&(self->last_header_field)), cape_str_mv (p_value));
    }
}

//-----------------------------------------------------------------------------

void qwave_reqctx_set_complete (QWaveReqctx self)
{
    self->complete = TRUE;
}

//-----------------------------------------------------------------------------

void qwave_reqctx_set (QWaveReqctx self, int upgrade, int keep_alive, const char* method)
{
    self->upgrade = upgrade;
    self->keep_alive = keep_alive;
    self->method = cape_str_cp (method);
}

//-----------------------------------------------------------------------------

int qwave_reqctx_is_complete (QWaveReqctx self)
{
    return self->complete;
}

//-----------------------------------------------------------------------------

void qwave_reqctx__parse_query (QWaveReqctx self)
{
    // local objects
    CapeString url = NULL;
    CapeString query = NULL;
    
    if (cape_tokenizer_split (self->url, '?', &url, &query))
    {
        cape_str_replace_mv (&(self->url), &url);
        
        
        cape_str_del (&query);
    }
}

//-----------------------------------------------------------------------------

void qwave_reqctx__parse_parts (QWaveReqctx self)
{
    // split the url into its parts
    CapeList url_values = cape_tokenizer_buf__noempty (self->url + 1, cape_str_size (self->url) - 1, '/');

    if (cape_list_size (url_values) >= 1)
    {
        CapeListNode n = cape_list_node_front (url_values);
        
        // get the first part
        const CapeString first_part = cape_list_node_data (n);
        
        if (qwave_config_route (self->config, first_part))
        {
            cape_str_replace_cp (&(self->url), "/index.html");
        }
        else if (cape_list_size (url_values) >= 2)
        {
            /*
            // anaylse the URL if we have an API or not
            self->api = qwebs_get_api (self->webs, first_part);
            
            if (self->api)
            {
                // reduce the url values by one
                {
                    CapeListNode n = cape_list_node_front (self->url_values);
                    
                    cape_list_node_erase (self->url_values, n);
                }
                
                self->body_value = cape_stream_new ();
            }
            */
        }
    }
    else
    {
        if (cape_str_equal ("/", self->url))
        {
            cape_str_replace_cp (&(self->url), "index.html");
        }
        else
        {
//            self->api = qwebs_get_page (self->webs, self->url);
        }
    }
    
    cape_list_del (&url_values);
}
    
//-----------------------------------------------------------------------------

void qwave_reqctx_exec (QWaveReqctx self)
{
    if (self->upgrade)
    {
        cape_log_fmt (CAPE_LL_DEBUG, "QWAVE", "reqexec", "connection upgrade");
        
        // try to find the appropriate header entry
        {
            const CapeString key;
            CapeMapNode n = cape_map_find (self->header_values, (void*)"Sec-WebSocket-Key");
            
            if (NULL == n)
            {
                cape_log_msg (CAPE_LL_WARN, "QWEBS", "on upgrade", "request has no 'Sec-WebSocket-Key'");
                
                return;
            }
            
            key = cape_map_node_value (n);
            if (NULL == key)
            {
                cape_log_msg (CAPE_LL_WARN, "QWEBS", "on upgrade", "header entry 'Sec-WebSocket-Key' is invalid");

                return;
            }
            
                        
            qwave_conctx_upgrade (self->conctx, key);                        
        }
        
        // try to close connection
        qwave_conctx_close (self->conctx, TRUE);
        
        // tell the context we don't need it anymore
        qwave_conctx_reqdec (self->conctx);
    }
    else
    {
        const CapeString site = qwave_config_site (self->config, &(self->url));

        if ('/' == *(self->url))
        {
            // extract query parameters
            qwave_reqctx__parse_query (self);
            
            // split path into parts
            qwave_reqctx__parse_parts (self);
        }

        cape_log_fmt (CAPE_LL_TRACE, "QWAVE", "reqexec", "process request url = %s, site = %s, method = %s, keep-alive = %i", self->url, site, self->method, self->keep_alive);
  
        /*
        if (self->api)
        {
            qwebs_request_api (p_self);
        }
        else
        */
        if ((FALSE == qwave_conctx_send_file (self->conctx, site, self->url, self->keep_alive)) || (FALSE == self->keep_alive))
        {
            // try to close connection
            qwave_conctx_close (self->conctx, TRUE);
        }

        // tell the context we don't need it anymore
        qwave_conctx_reqdec (self->conctx);
    }
}

//-----------------------------------------------------------------------------

