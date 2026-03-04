#include "qwave_response.h"

struct QWaveResponse_s
{
    
    
};

//-----------------------------------------------------------------------------

QWaveResponse qwave_response_new ()
{
    QWaveResponse self = CAPE_NEW (struct QWaveResponse_s);
    
    
    return self;
}

//-----------------------------------------------------------------------------

void qwave_response_del (QWaveResponse* p_self)
{
    if (*p_self)
    {
        QWaveResponse self = *p_self;
        
        
        
        
        CAPE_DEL (p_self, struct QWaveResponse_s);
    }
}

//-----------------------------------------------------------------------------

void qwave_response_file (QWaveResponse self, CapeStream s, const CapeString path)
{
    // BEGIN
    cape_stream_clr (s);
    
    cape_stream_append_str (s, "HTTP/1.1 200 OK\r\n");
    
    // mime type
    {
        /*
        cape_stream_append_str (s, "Content-Type: ");
        cape_stream_append_str (s, cape_udc_get_s (file_node, "mime", "application/json"));
        cape_stream_append_str (s, "\r\n");
        */
    }
    
    // name (this is important to open the file directly in the browser)
    {
        /*
        cape_stream_append_str (s, "Content-Disposition: inline; filename=\"");
        cape_stream_append_str (s, cape_udc_get_s (file_node, "name", "document"));
        cape_stream_append_str (s, "\"; name=\"");
        cape_stream_append_str (s, cape_udc_get_s (file_node, "name", "document"));
        cape_stream_append_str (s, "\"\r\n");
        */
    }
    
    
    
}

//-----------------------------------------------------------------------------
