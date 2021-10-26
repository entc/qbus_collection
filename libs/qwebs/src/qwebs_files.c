#include "qwebs_files.h"
#include "qwebs_response.h"
#include "qwebs.h"

// cape includes
#include <sys/cape_types.h>
#include <sys/cape_file.h>
#include <sys/cape_log.h>
#include <stc/cape_map.h>

//-----------------------------------------------------------------------------

struct QWebsFiles_s
{
  CapeMap mime_types;
  QWebs webs;
};

//-----------------------------------------------------------------------------

static void __STDCALL qwebs_files__intern__on_mime_types_del (void* key, void* val)
{
}

//-----------------------------------------------------------------------------

QWebsFiles qwebs_files_new (QWebs webs)
{
  QWebsFiles self = CAPE_NEW (struct QWebsFiles_s);
  
  self->webs = webs;
  self->mime_types = cape_map_new (NULL, qwebs_files__intern__on_mime_types_del, NULL);
  
  cape_map_insert (self->mime_types, "html",  "text/html; charset=utf-8");
  cape_map_insert (self->mime_types, "htm",   "text/html; charset=utf-8");
  cape_map_insert (self->mime_types, "css",   "text/css");
  cape_map_insert (self->mime_types, "js",    "text/javascript; charset=utf-8");
  cape_map_insert (self->mime_types, "ico",   "image/vnd.microsoft.icon");
  cape_map_insert (self->mime_types, "png",   "image/png");
  cape_map_insert (self->mime_types, "jpeg",  "image/jpeg");
  cape_map_insert (self->mime_types, "jpg",   "image/jpeg");
  cape_map_insert (self->mime_types, "jpe",   "image/jpeg");
  
  return self;
}

//-----------------------------------------------------------------------------

void qwebs_files_del (QWebsFiles* p_self)
{
  if (*p_self)
  {
    QWebsFiles self = *p_self;
    
    cape_map_del (&(self->mime_types));
    
    CAPE_DEL (p_self, struct QWebsFiles_s);
  }
}

//-----------------------------------------------------------------------------

const CapeString qwebs_files_mime (QWebsFiles self, const CapeString extension)
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

CapeStream qwebs_files_get (QWebsFiles self, const CapeString site, const CapeString path, const CapeString remote)
{
  CapeStream ret = NULL;
  
  // local objects
  CapeErr err = cape_err_new ();
  CapeUdc file_node = cape_udc_new (CAPE_UDC_NODE, NULL);
  CapeString file_relative = cape_fs_path_merge (site, path);
  CapeString file_rebuild = NULL;
  
  {
    cape_log_fmt (CAPE_LL_TRACE, "QWEBS", "files get", "path: %s", file_relative);

    // retrieve an absolute path
    file_rebuild = cape_fs_path_rebuild (file_relative, err);
    if (file_rebuild == NULL)
    {
      cape_log_msg (CAPE_LL_WARN, "QWEBS", "files get", "file path invalid");
      
      cape_err_set (err, CAPE_ERR_NOT_FOUND, "not found");

      // report an incident
      if (qwebs_raise_file (self->webs, file_rebuild, remote))
      {
        // do something special here
        
      }

      // it seems to be no valid file
      goto exit_and_cleanup;
    }
    
    if (!cape_str_begins (file_rebuild, site))
    {
      cape_log_msg (CAPE_LL_WARN, "QWEBS", "files get", "file outside site path");
      
      cape_err_set (err, CAPE_ERR_NOT_FOUND, "not found");
      
      // report an incident
      if (qwebs_raise_file (self->webs, file_rebuild, remote))
      {
        // do something special here
        
      }
      
      // it seems to be no valid file
      goto exit_and_cleanup;
    }

    ret = cape_stream_new ();
    
    cape_udc_add_s_mv (file_node, "file", &file_rebuild);
    cape_udc_add_s_cp (file_node, "mime", qwebs_files_mime (self, cape_fs_extension (path)));
  }
  
  qwebs_response_file (ret, self->webs, file_node);

exit_and_cleanup:
  
  if (ret == NULL)
  {
    ret = cape_stream_new ();
    
    //cape_log_fmt (CAPE_LL_ERROR, "QWEBS", "send file", "got error: %s", cape_err_text (err));
    qwebs_response_err (ret, self->webs, NULL, cape_udc_get_s (file_node, "mime", "application/json"), err);
  }

  cape_str_del (&file_rebuild);
  cape_str_del (&file_relative);
  cape_udc_del (&file_node);
  cape_err_del (&err);
  
  return ret;
}

//-----------------------------------------------------------------------------
