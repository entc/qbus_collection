#include "qwave_response.h"

// cape includes
#include <sys/cape_file.h>
#include <sys/cape_log.h>
#include <stc/cape_map.h>

//-----------------------------------------------------------------------------

struct QWaveResponse_s
{
    CapeMap mime_types;
    
    
};

//-----------------------------------------------------------------------------

static void __STDCALL qwave_response__mime_types__on_del (void* key, void* val)
{
}

//-----------------------------------------------------------------------------

QWaveResponse qwave_response_new ()
{
    QWaveResponse self = CAPE_NEW (struct QWaveResponse_s);
    
    self->mime_types = cape_map_new (NULL, qwave_response__mime_types__on_del, NULL);
    
    cape_map_insert (self->mime_types, "html",  "text/html; charset=utf-8");
    cape_map_insert (self->mime_types, "htm",   "text/html; charset=utf-8");
    cape_map_insert (self->mime_types, "css",   "text/css");
    cape_map_insert (self->mime_types, "js",    "text/javascript; charset=utf-8");
    cape_map_insert (self->mime_types, "ico",   "image/vnd.microsoft.icon");
    cape_map_insert (self->mime_types, "png",   "image/png");
    cape_map_insert (self->mime_types, "jpeg",  "image/jpeg");
    cape_map_insert (self->mime_types, "jpg",   "image/jpeg");
    cape_map_insert (self->mime_types, "jpe",   "image/jpeg");
    cape_map_insert (self->mime_types, "svg",   "image/svg+xml");
    cape_map_insert (self->mime_types, "json",  "application/json; charset=utf-8");
    cape_map_insert (self->mime_types, "pdf",   "application/pdf");
    
    return self;
}

//-----------------------------------------------------------------------------

void qwave_response_del (QWaveResponse* p_self)
{
    if (*p_self)
    {
        QWaveResponse self = *p_self;
                
        cape_map_del (&(self->mime_types));
        
        CAPE_DEL (p_self, struct QWaveResponse_s);
    }
}

//-----------------------------------------------------------------------------

const CapeString qwave_response__fetch_mime (QWaveResponse self, const CapeString extension)
{
    if (extension)
    {
        CapeMapNode n = cape_map_find (self->mime_types, extension);
        if (n)
        {
            return cape_map_node_value (n);
        }
    }
    
    return "text/plain; charset=utf-8";
}

//-----------------------------------------------------------------------------

int __STDCALL qwave_response__file__on_load (void* ptr, const char* bufdat, number_t buflen, CapeErr err)
{
    cape_stream_append_buf (ptr, bufdat, buflen);
    
    return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

void qwave_response_file (QWaveResponse self, CapeStream stream_message, const CapeString path)
{
    int res;
    
    // local objects
    CapeErr err = cape_err_new ();
    CapeStream stream_content = cape_stream_new ();

    // BEGIN
    cape_stream_clr (stream_message);

    res = cape_fs_file_load (NULL, path, stream_content, qwave_response__file__on_load, err);
    if (res)
    {
        cape_stream_append_str (stream_message, "HTTP/1.1 404 Not Found\r\n");
        
        // start with content
        cape_stream_append_str (stream_message, "\r\n");
    }
    else
    {        
        cape_stream_append_str (stream_message, "HTTP/1.1 200 OK\r\n");
        
        // mime type
        {
            cape_stream_append_str (stream_message, "Content-Type: ");
            cape_stream_append_str (stream_message, qwave_response__fetch_mime (self, cape_fs_extension (path)));
            cape_stream_append_str (stream_message, "\r\n");
        }
        
        // name (this is important to open the file directly in the browser)
        {
            cape_stream_append_str (stream_message, "Content-Disposition: inline; filename=\"");
            cape_stream_append_str (stream_message, path);
            cape_stream_append_str (stream_message, "\"; name=\"");
            cape_stream_append_str (stream_message, path);
            cape_stream_append_str (stream_message, "\"\r\n");
        }
        
        // content length
        {
            cape_stream_append_str (stream_message, "Content-Length: ");
            cape_stream_append_n (stream_message, cape_stream_size (stream_content));
            cape_stream_append_str (stream_message, "\r\n");
        }

        // start with content
        cape_stream_append_str (stream_message, "\r\n");
        
        cape_stream_append_stream (stream_message, stream_content);
    }
    
    cape_stream_del (&stream_content);
    cape_err_del (&err);
}

//-----------------------------------------------------------------------------
