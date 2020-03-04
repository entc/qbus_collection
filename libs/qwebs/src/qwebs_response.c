#include "qwebs_response.h"
#include "qwebs.h"

// cape includes
#include <sys/cape_file.h>
#include <sys/cape_log.h>
#include <fmt/cape_json.h>

// qcrypt includes
#include <qcrypt.h>
#include <qcrypt_encrypt.h>
#include <qcrypt_file.h>

//-----------------------------------------------------------------------------

void qwebs_response__internal__identification (CapeStream s)
{
  cape_stream_append_str (s, "Server: QWEBS\r\n");
  cape_stream_append_str (s, "X-Powered-By: QWEBS/1.0.0\r\n");
}

//-----------------------------------------------------------------------------

void qwebs_response__internal__content_length (CapeStream s, number_t buflen)
{
  cape_stream_append_str (s, "Content-Length: ");
  cape_stream_append_n (s, buflen);
  cape_stream_append_str (s, "\r\n");
  cape_stream_append_str (s, "\r\n");
}

//-----------------------------------------------------------------------------

void qwebs_response__internal__content (CapeStream s, QWebs webs, CapeUdc content)
{
  // serialize UDC container to add as HTTP content
  if (content)
  {
    CapeString h = cape_json_to_s (content);

    qwebs_response__internal__content_length (s, cape_str_size (h));
    cape_stream_append_str (s, h);
    
    cape_str_del (&h);
  }
  else
  {
    qwebs_response__internal__content_length (s, 0);
  }
}

//-----------------------------------------------------------------------------

typedef struct
{
  CapeStream content;
  QEncryptBase64 enbase64;
  
} QWebsFileContext;

//-----------------------------------------------------------------------------

int __STDCALL qwebs_response__file__on_load (void* ptr, const char* bufdat, number_t buflen, CapeErr err)
{
  int res = CAPE_ERR_NONE;
  QWebsFileContext* fctx = ptr;
  
  if (fctx->enbase64)
  {
    res = qencrypt_base64_process (fctx->enbase64, bufdat, buflen, err);
  }
  else
  {
    cape_stream_append_buf (fctx->content, bufdat, buflen);
  }
  
  return res;
}

//-----------------------------------------------------------------------------

void qwebs_response_file (CapeStream s, QWebs webs, CapeUdc file_node)
{
  int is_encrypted;
  
  const CapeString vsec;
  const CapeString file;
  
  // local objects
  CapeErr err = cape_err_new ();
  CapeStream c = NULL;
  
  // do some checks
  file = cape_udc_get_s (file_node, "file", NULL);
  if (NULL == file)
  {
    cape_err_set (err, CAPE_ERR_WRONG_VALUE, "file is not set");
    goto on_error;
  }
  
  is_encrypted = cape_udc_get_b (file_node, "encrypted", FALSE);
  if (is_encrypted)
  {
    vsec = cape_udc_get_s (file_node, "vsec", NULL);
    if (NULL == vsec)
    {
      cape_err_set (err, CAPE_ERR_WRONG_VALUE, "vsec is not set");
      goto on_error;
    }
  }
  
  // BEGIN
  cape_stream_clr (s);
  
  cape_stream_append_str (s, "HTTP/1.1 200 OK\r\n");
  
  qwebs_response__internal__identification (s);
  
  // mime type
  {
    cape_stream_append_str (s, "Content-Type: ");
    cape_stream_append_str (s, cape_udc_get_s (file_node, "mime", "application/json"));
    cape_stream_append_str (s, "\r\n");
  }
  
  // name (this is important to open the file directly in the browser)
  {
    cape_stream_append_str (s, "Content-Disposition: inline; filename=\"");
    cape_stream_append_str (s, cape_udc_get_s (file_node, "name", "document"));
    cape_stream_append_str (s, "\"; name=\"");
    cape_stream_append_str (s, cape_udc_get_s (file_node, "name", "document"));
    cape_stream_append_str (s, "\"\r\n");
  }
  
  // location #1
  {
    cape_stream_append_str (s, "Content-Location: /");
    cape_stream_append_str (s, cape_udc_get_s (file_node, "name", "document"));
    cape_stream_append_str (s, "\r\n");
  }
  
  // location #2
  {
    cape_stream_append_str (s, "Location: /");
    cape_stream_append_str (s, cape_udc_get_s (file_node, "name", "document"));
    cape_stream_append_str (s, "\r\n");
  }
  
  // create a stream for the content
  c = cape_stream_new ();
  
  // content
  {
    int res;
    QWebsFileContext fctx;
    
    fctx.content = c;
    
    if (cape_udc_get_b (file_node, "base64", FALSE))
    {
      fctx.enbase64 = qencrypt_base64_new (c);
      
      cape_stream_append_str (c, "data:");
      cape_stream_append_str (c, cape_udc_get_s (file_node, "mime", "application/octet-stream"));
      cape_stream_append_str (c, ";base64,");
    }
    else
    {
      fctx.enbase64 = NULL;
    }
    
    // cape_log_fmt (CAPE_LL_DEBUG, "QWEBS", "send file", "use file = %s", file);
    
    if (is_encrypted)
    {
      res = qcrypt_decrypt_file (vsec, file, &fctx, qwebs_response__file__on_load, err);
    }
    else
    {
      res = cape_fs_file_load (NULL, file, &fctx, qwebs_response__file__on_load, err);
    }
    
    if (res)
    {
      cape_err_set (err, CAPE_ERR_NOT_FOUND, "can't open file");
      goto on_error;
    }

    if (fctx.enbase64)
    {
      // we need to finalize the base64 stream
      res = qencrypt_base64_finalize (fctx.enbase64, err);
    }

    // cleanup
    qencrypt_base64_del (&(fctx.enbase64));

    if (res)
    {
      cape_err_set (err, CAPE_ERR_NOT_FOUND, "can't open file");
      goto on_error;
    }

    // content
    qwebs_response__internal__content_length (s, cape_stream_size (c));    
    cape_stream_append_stream (s, c);
  }
  
  goto exit_and_cleanup;
  
on_error:

  //cape_log_fmt (CAPE_LL_ERROR, "QWEBS", "send file", "got error: %s", cape_err_text (err));
  qwebs_response_err (s, webs, NULL, cape_udc_get_s (file_node, "mime", "application/json"), err);
  
exit_and_cleanup:
  
  cape_stream_del (&c);
  cape_err_del (&err);
}

//-----------------------------------------------------------------------------

void qwebs_response_json (CapeStream s, QWebs webs, CapeUdc content)
{
  // BEGIN
  cape_stream_clr (s);
  
  cape_stream_append_str (s, "HTTP/1.1 200 OK\r\n");
  
  qwebs_response__internal__identification (s);
  
  // mime type for JSON
  {
    cape_stream_append_str (s, "Content-Type: ");
    cape_stream_append_str (s, "application/json");
    cape_stream_append_str (s, "\r\n");
  }

  qwebs_response__internal__content (s, webs, content);
}

//-----------------------------------------------------------------------------

void qwebs_response_buf (CapeStream s, QWebs webs, const CapeString buf)
{
  // BEGIN
  cape_stream_clr (s);
  
  cape_stream_append_str (s, "HTTP/1.1 200 OK\r\n");

  qwebs_response__internal__identification (s);
  
  // mime type for JSON
  {
    cape_stream_append_str (s, "Content-Type: ");
    cape_stream_append_str (s, "image/jpeg");
    cape_stream_append_str (s, "\r\n");
  }
  
  qwebs_response__internal__content_length (s, cape_str_size (buf));
  cape_stream_append_str (s, "data:image/jpeg;base64,");
  cape_stream_append_str (s, buf);
}

//-----------------------------------------------------------------------------

void qwebs_response_err (CapeStream s, QWebs webs, CapeUdc content, const CapeString mime, CapeErr err)
{
  const CapeString http_code;
  
  // BEGIN
  cape_stream_clr (s);
  
  // convert cape errors into HTTP error-codes
  switch (cape_err_code (err))
  {
    case CAPE_ERR_NONE:
    {
      cape_stream_append_str (s, "HTTP/1.1 200 OK\r\n");
      http_code = "200"; 
      
      break;
    }
    case CAPE_ERR_NO_CONTENT:
    {
      cape_stream_append_str (s, "HTTP/1.1 204 No Content\r\n");
      http_code = "204"; 
      
      break;
    }
    case CAPE_ERR_NO_AUTH:     
    {
      cape_stream_append_str (s, "HTTP/1.1 401 Unauthorized\r\n"); 
      http_code = "401"; 
      
      break;
    }
    case CAPE_ERR_NO_ROLE:
    {
      cape_stream_append_str (s, "HTTP/1.1 403 Forbidden\r\n");
      http_code = "403"; 
      
      break;
    }
    case CAPE_ERR_NOT_FOUND:   
    {
      cape_stream_append_str (s, "HTTP/1.1 404 Not Found\r\n");
      http_code = "404"; 
      
      break;
    }
    case CAPE_ERR_MISSING_PARAM:
    {
      cape_stream_append_str (s, "HTTP/1.1 428 Precondition Required\r\n"); 
      http_code = "428"; 
      
      break;
    }
    default:                   
    {
      cape_stream_append_str (s, "HTTP/1.1 500 Internal Server Error\r\n");
      http_code = "500"; 
      
      break;
    }
  }
  
  qwebs_response__internal__identification (s);
  
  if (content)
  {
    // mime type for JSON
    cape_stream_append_str (s, "Content-Type: ");
    cape_stream_append_str (s, "application/json");
    cape_stream_append_str (s, "\r\n");
    
    qwebs_response__internal__content (s, webs, content);
  }
  else
  {
    if (cape_err_code (err))
    {
      // check mime
      if (cape_str_begins (mime, "text/html"))
      {
        int res;
        QWebsFileContext fctx;
        
        CapeString file = cape_str_catenate_3 ("page_", http_code, ".htm");
        
        fctx.content = cape_stream_new ();
        fctx.enbase64 = NULL;
        
        cape_stream_append_str (s, "Warning: ");
        cape_stream_append_str (s, cape_err_text (err));
        cape_stream_append_str (s, "\r\n");
        
        res = cape_fs_file_load (qwebs_pages (webs), file, &fctx, qwebs_response__file__on_load, err);
        if (res)
        {
          cape_log_fmt (CAPE_LL_WARN, "QWEBS", "send json", "can't open pages file = %s", file);
          
          cape_stream_append_str (s, "Content-Type: ");
          cape_stream_append_str (s, "text/html; charset=utf-8");
          cape_stream_append_str (s, "\r\n");
          
          qwebs_response__internal__content_length (s, 0);
        }
        else
        {
          cape_stream_append_str (s, "Content-Type: ");
          cape_stream_append_str (s, "text/html; charset=utf-8");   // the loaded file should be a HTM file
          cape_stream_append_str (s, "\r\n");
          
          // content
          qwebs_response__internal__content_length (s, cape_stream_size (fctx.content));
          cape_stream_append_stream (s, fctx.content);
        }
        
        cape_str_del (&file);
        cape_stream_del (&(fctx.content));
        
        return;
      }
    }

    cape_stream_append_str (s, "Content-Type: ");
    cape_stream_append_str (s, mime ? mime : "text/html; charset=utf-8");
    cape_stream_append_str (s, "\r\n");
    
    qwebs_response__internal__content_length (s, 0);
  }
}

//-----------------------------------------------------------------------------
