#include "qwebs_response.h"
#include "qwebs.h"

// cape includes
#include <sys/cape_file.h>
#include <sys/cape_log.h>
#include <fmt/cape_json.h>
#include <fmt/cape_tokenizer.h>

// qcrypt includes
#include <qcrypt.h>
#include <qcrypt_encrypt.h>
#include <qcrypt_file.h>

//-----------------------------------------------------------------------------

void qwebs_response__internal__identification (CapeStream s, const CapeString server_identifier, const CapeString provider)
{
  cape_stream_append_str (s, "Server: ");
  cape_stream_append_str (s, server_identifier);
  cape_stream_append_str (s, "\r\nx-powered-by: ");
  cape_stream_append_str (s, provider);
  cape_stream_append_str (s, "\r\n");
  
  // some extra fields
  {
    cape_stream_append_str (s, "tk: N\r\n");
  }

  // deactive caching
//  cape_stream_append_str (s, "cache-control: no-cache, no-store, max-age=0, no-transform, private\r\n");

  // tell the browser not to sniff for the correct mime type
  cape_stream_append_str (s, "x-content-type-options: nosniff\r\n");
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

int qwebs_response_file__data_uri (CapeStream s, CapeUdc file_node, int is_encrypted, const CapeString file, const CapeString vsec, CapeErr err)
{
  int res;

  // local objects
  CapeStream c = cape_stream_new ();
  CapeString h = NULL;
  CapeString lh = NULL;
  CapeString rh = NULL;
  CapeString lu = NULL;
  CapeString ru = NULL;

  // load the file
  {
    QWebsFileContext fctx;
    
    fctx.content = c;
    fctx.enbase64 = FALSE;
    
    if (is_encrypted)
    {
      res = qcrypt_decrypt_file (vsec, file, &fctx, qwebs_response__file__on_load, err);
    }
    else
    {
      res = cape_fs_file_load (NULL, file, &fctx, qwebs_response__file__on_load, err);
    }
  }

  if (res)
  {
    goto exit_and_cleanup;
  }

  // convert into string
  h = cape_stream_to_str (&c);
  
  // analyse the beginning
  if (cape_str_begins (h, "data:") == FALSE)
  {
    cape_err_set (err, CAPE_ERR_WRONG_VALUE, "file is not in data uri format");
    goto exit_and_cleanup;
  }
  
  if (cape_tokenizer_split (h + 5, ',', &lh, &rh) == FALSE)
  {
    cape_err_set (err, CAPE_ERR_WRONG_VALUE, "file is not in data uri format");
    goto exit_and_cleanup;
  }
  
  if (cape_tokenizer_split (lh, ';', &lu, &ru))
  {
    if (cape_str_equal (ru, "base64"))
    {
      c = qcrypt__decode_base64_s (rh);
      
      // mime type
      {
        cape_stream_append_str (s, "Content-Type: ");
        cape_stream_append_str (s, lu);
        cape_stream_append_str (s, "\r\n");
      }
    }
  }
  else
  {
    // mime type
    {
      cape_stream_append_str (s, "Content-Type: ");
      cape_stream_append_str (s, lh);
      cape_stream_append_str (s, "\r\n");
    }
  }
  
  // name (this is important to open the file directly in the browser)
  {
    cape_stream_append_str (s, "Content-Disposition: inline; filename=\"");
    cape_stream_append_str (s, cape_udc_get_s (file_node, "name", "document"));
    cape_stream_append_str (s, "\"; name=\"");
    cape_stream_append_str (s, cape_udc_get_s (file_node, "name", "document"));
    cape_stream_append_str (s, "\"\r\n");
  }

  if (c)
  {
    // content
    qwebs_response__internal__content_length (s, cape_stream_size (c));
    cape_stream_append_stream (s, c);
  }

exit_and_cleanup:

  cape_str_del (&lu);
  cape_str_del (&ru);
  cape_str_del (&lh);
  cape_str_del (&rh);
  cape_stream_del (&c);
  cape_str_del (&h);
  return res;
}

//-----------------------------------------------------------------------------

int qwebs_response_file__content (CapeStream s, CapeUdc file_node, int is_encrypted, const CapeString file, const CapeString vsec, CapeErr err)
{
  int res;
  
  // local objects
  CapeStream c = NULL;

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

  // create a stream for the content
  c = cape_stream_new ();
  
  // content
  {
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
      goto exit_and_cleanup;
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
      goto exit_and_cleanup;
    }
    
    // content
    qwebs_response__internal__content_length (s, cape_stream_size (c));
    cape_stream_append_stream (s, c);
  }

  res = CAPE_ERR_NONE;
  
exit_and_cleanup:

  cape_stream_del (&c);
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
  CapeString file_absolute = NULL;
  
  // do some checks
  file = cape_udc_get_s (file_node, "file", NULL);
  if (NULL == file)
  {
    cape_err_set (err, CAPE_ERR_WRONG_VALUE, "file is not set");
    goto exit_and_cleanup;
  }

  is_encrypted = cape_udc_get_b (file_node, "encrypted", FALSE);
  if (is_encrypted)
  {
    vsec = cape_udc_get_s (file_node, "vsec", NULL);
    if (NULL == vsec)
    {
      cape_err_set (err, CAPE_ERR_WRONG_VALUE, "vsec is not set");
      goto exit_and_cleanup;
    }
  }
  else
  {
    vsec = NULL;
  }
  
  // BEGIN
  cape_stream_clr (s);
  
  cape_stream_append_str (s, "HTTP/1.1 200 OK\r\n");
  
  qwebs_response__internal__identification (s, qwebs_identifier (webs), qwebs_provider (webs));
  
  if (cape_udc_get_b (file_node, "data_uri", FALSE))
  {
    qwebs_response_file__data_uri (s, file_node, is_encrypted, file, vsec, err);
  }
  else
  {
    qwebs_response_file__content (s, file_node, is_encrypted, file, vsec, err);
  }
  
exit_and_cleanup:
  
  if (cape_err_code (err))
  {
    //cape_log_fmt (CAPE_LL_ERROR, "QWEBS", "send file", "got error: %s", cape_err_text (err));
    qwebs_response_err (s, webs, NULL, cape_udc_get_s (file_node, "mime", "application/json"), err);
  }
  
  cape_err_del (&err);
}

//-----------------------------------------------------------------------------

void qwebs_response_expires (CapeStream s, number_t ttl)
{
  if (ttl > 0)
  {
    CapeDatetime dt_current;
    CapeDatetime dt_expire;
    
    // get the current time in UTC
    cape_datetime_utc (&dt_current);
    
    // add ttl as seconds
    cape_datetime__add_n (&dt_current, ttl, &dt_expire);
    
    {
      CapeString h = cape_datetime_s__gmt (&dt_expire);
      
      cape_stream_append_str (s, "Expires: ");
      cape_stream_append_str (s, h);
      cape_stream_append_str (s, "\r\n");
      
      cape_str_del (&h);
    }
  }
}

//-----------------------------------------------------------------------------

void qwebs_response_json (CapeStream s, QWebs webs, CapeUdc content, number_t ttl)
{
  // BEGIN
  cape_stream_clr (s);
  
  cape_stream_append_str (s, "HTTP/1.1 200 OK\r\n");
  
  qwebs_response__internal__identification (s, qwebs_identifier (webs), qwebs_provider (webs));
  
  // mime type for JSON
  {
    cape_stream_append_str (s, "Content-Type: ");
    cape_stream_append_str (s, "application/json;charset=UTF-8");
    cape_stream_append_str (s, "\r\n");
  }
  
  // add expire date
  qwebs_response_expires (s, ttl);

  qwebs_response__internal__content (s, webs, content);
}

//-----------------------------------------------------------------------------

void qwebs_response_image (CapeStream s, QWebs webs, const CapeString buf)
{
  // BEGIN
  cape_stream_clr (s);
  
  cape_stream_append_str (s, "HTTP/1.1 200 OK\r\n");

  qwebs_response__internal__identification (s, qwebs_identifier (webs), qwebs_provider (webs));
  
  // mime type for JSON
  {
    cape_stream_append_str (s, "Content-Type: ");
    cape_stream_append_str (s, "text/plain");
    cape_stream_append_str (s, "\r\n");
  }
  
  CapeString data_url = "data:image/jpeg;base64,";
  
  qwebs_response__internal__content_length (s, cape_str_size (data_url) + cape_str_size (buf));
  cape_stream_append_str (s, data_url);
  cape_stream_append_str (s, buf);
}

//-----------------------------------------------------------------------------

void qwebs_response_buf (CapeStream s, QWebs webs, const CapeString buf, const CapeString mime_type, number_t ttl)
{
  cape_stream_clr (s);
  
  // start with the header
  cape_stream_append_str (s, "HTTP/1.1 200 OK\r\n");
  qwebs_response__internal__identification (s, qwebs_identifier (webs), qwebs_provider (webs));

  // mime type
  {
    cape_stream_append_str (s, "Content-Type: ");
    cape_stream_append_str (s, mime_type);
    cape_stream_append_str (s, "\r\n");
  }
  
  // add expire date
  qwebs_response_expires (s, ttl);

  qwebs_response__internal__content_length (s, cape_str_size (buf));
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
  
  qwebs_response__internal__identification (s, qwebs_identifier (webs), qwebs_provider (webs));
  
  if (cape_err_code (err))
  {
    cape_stream_append_str (s, "Warning: ");
    cape_stream_append_n (s, cape_err_code (err));
    cape_stream_append_str (s, ", ");
    cape_stream_append_str (s, cape_err_text (err));
    cape_stream_append_str (s, "\r\n");
  }
  
  if (content)
  {
    // mime type for JSON
    cape_stream_append_str (s, "Content-Type: ");
    cape_stream_append_str (s, "application/json; charset=UTF-8");
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

void qwebs_response_redirect (CapeStream s, QWebs webs, const CapeString url)
{
  // BEGIN
  cape_stream_clr (s);
  
  cape_stream_append_str (s, "HTTP/1.1 301 Moved Permanently\r\n");

  qwebs_response__internal__identification (s, qwebs_identifier (webs), qwebs_provider (webs));

  cape_stream_append_str (s, "Location: ");
  
  {
    //CapeString encoded_url = qwebs_url_encode (webs, url);
    
    cape_log_fmt (CAPE_LL_TRACE, "QWEBS", "redirect", "redirect to location = '%s'", url);
    
    cape_stream_append_str (s, url);
    
    //cape_str_del (&encoded_url);
  }
  
  cape_stream_append_str (s, "\r\n");

  // DONE
  qwebs_response__internal__content_length (s, 0);
}

//-----------------------------------------------------------------------------

void qwebs_response_mp_init (CapeStream s, QWebs webs, const CapeString boundary, const CapeString mime)
{
  // BEGIN
  cape_stream_clr (s);
  
  // start with the header
  cape_stream_append_str (s, "HTTP/1.1 200 OK\r\n");
  qwebs_response__internal__identification (s, qwebs_identifier (webs), qwebs_provider (webs));

  cape_stream_append_str (s, "Max-Age: 0\r\n");
  cape_stream_append_str (s, "Expires: 0\r\n");
  cape_stream_append_str (s, "Cache-Control: no-cache, private\r\n");
  cape_stream_append_str (s, "Pragma: no-cache\r\n");
  
  cape_stream_append_str (s, "Content-Type: ");
  cape_stream_append_str (s, mime);
  cape_stream_append_str (s, "; boundary=--");
  cape_stream_append_str (s, boundary);
  cape_stream_append_str (s, "\r\n");
  cape_stream_append_str (s, "\r\n");  
}

//-----------------------------------------------------------------------------

void qwebs_response_mp_part (CapeStream s, QWebs webs, const CapeString boundary, const CapeString mime, const char* bufdat, number_t buflen)
{
  // BEGIN
  cape_stream_clr (s);
  
  cape_stream_append_str (s, "--");
  cape_stream_append_str (s, boundary);
  cape_stream_append_str (s, "\r\n");

  cape_stream_append_str (s, "Content-Type: ");
  cape_stream_append_str (s, mime);
  cape_stream_append_str (s, "\r\n");

  qwebs_response__internal__content_length (s, buflen);

  cape_stream_append_buf (s, bufdat, buflen);

  cape_stream_append_str (s, "\r\n");
}

//-----------------------------------------------------------------------------

void qwebs_response_sp (CapeStream s, QWebs webs, const CapeString name, CapeMap return_headers)
{
  // local objects
  CapeMapCursor* cursor = cape_map_cursor_create (return_headers, CAPE_DIRECTION_FORW);
  
  // BEGIN
  cape_stream_clr (s);

  // start with the header
  cape_stream_append_str (s, "HTTP/1.1 101 Switching Protocols\r\n");
  qwebs_response__internal__identification (s, qwebs_identifier (webs), qwebs_provider (webs));

  cape_stream_append_str (s, "Upgrade: ");
  cape_stream_append_str (s, name);
  cape_stream_append_str (s, "\r\n");

  cape_stream_append_str (s, "Connection: Upgrade\r\n");
  
  while (cape_map_cursor_next (cursor))
  {
    cape_stream_append_str (s, cape_map_node_key (cursor->node));
    cape_stream_append_str (s, ": ");
    cape_stream_append_str (s, cape_map_node_value (cursor->node));
    cape_stream_append_str (s, "\r\n");
  }

  cape_stream_append_str (s, "\r\n");

  cape_map_cursor_destroy (&cursor);
}

//-----------------------------------------------------------------------------
