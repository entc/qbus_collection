#include "qwebs_files.h"
#include "qwebs_response.h"

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

CapeStream qwebs_files_get (QWebsFiles self, const CapeString site, const CapeString path)
{
  // local objects
  CapeStream s = cape_stream_new ();

  CapeUdc file_node = cape_udc_new (CAPE_UDC_NODE, NULL);

  CapeString file = cape_fs_path_merge (site, path);

  cape_udc_add_s_mv (file_node, "file", &file);
  cape_udc_add_s_cp (file_node, "mime", qwebs_files_mime (self, cape_fs_extension (path)));
  
  qwebs_response_file (s, self->webs, file_node);
  
  return s;
}

//-----------------------------------------------------------------------------
