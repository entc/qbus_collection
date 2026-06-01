#include "qtee_template.h"
#include "qtee_compiler.h"
#include "qtee_eval.h"
#include "qtee_format.h"

#include <fcntl.h>
#include <math.h>

// cape includes
#include "stc/cape_stream.h"
#include "stc/cape_map.h"
#include "stc/cape_list.h"
#include "sys/cape_log.h"
#include "sys/cape_time.h"
#include "sys/cape_file.h"
#include "fmt/cape_tokenizer.h"
#include "fmt/cape_json.h"

//-----------------------------------------------------------------------------

struct CapeTemplate_s
{
  
  CapeString fileName;
  
  QTeePart root_part;
  
};

//-----------------------------------------------------------------------------

static int __STDCALL qtee_template_compile__on_buf (void* ptr, const char* bufdat, number_t buflen, CapeErr err)
{
    qtee_compiler_parse (ptr, bufdat, buflen);
    
    return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

int cape_template_compile (CapeTemplate self, const char* path, CapeErr err)
{
    int res;
  
    // local objects
    QTeeCompiler tcl = qtee_compiler_new (self->root_part);

    // open the file and parse the content
    res = cape_fs_file_load (path, self->fileName, tcl, qtee_template_compile__on_buf, err);

    qtee_compiler_del (&tcl);
    return res;
}

//-----------------------------------------------------------------------------

int cape_template_filename (CapeTemplate self, const char* name, const char* lang, CapeErr err)
{
  if (name == NULL)
  {
    return cape_err_set (err, CAPE_ERR_WRONG_VALUE, "name is NULL");
  }
  
  if (lang)
  {
    self->fileName = cape_str_catenate_c (lang, '_', name);
  }
  else
  {
    self->fileName = cape_str_cp (name);
  }
  
  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

CapeTemplate cape_template_new (void)
{
  CapeTemplate self = CAPE_NEW (struct CapeTemplate_s);
  
  self->root_part = qtee_part_new (PART_TYPE_TAG, NULL, NULL);
  self->fileName = NULL;
  
  return self;
}

//-----------------------------------------------------------------------------

void cape_template_del (CapeTemplate* p_self)
{
  if (*p_self)
  {
    CapeTemplate self = *p_self;
    
    cape_str_del(&(self->fileName));
    qtee_part_del (&(self->root_part));
    
    CAPE_DEL(p_self, struct CapeTemplate_s);
  }
}

//-----------------------------------------------------------------------------

int cape_template_compile_file (CapeTemplate self, const char* path, const char* name, const char* lang, CapeErr err)
{
  int res;
  
  res = cape_template_filename (self, name, lang, err);
  if (res)
  {
    return res;
  }
  
  return cape_template_compile (self, path, err);
}

//-----------------------------------------------------------------------------

void cape_template_compile_str (CapeTemplate self, const char* content)
{
    // local object
    QTeeCompiler tcl = qtee_compiler_new (self->root_part);
    
    // clear before we start
    qtee_part_clear (self->root_part);
    
    // parse the input string into qtee parts, results into root_part
    qtee_compiler_parse (tcl, content, cape_str_size (content));
    
    qtee_compiler_del (&tcl);
}

//-----------------------------------------------------------------------------

int cape_template_apply (CapeTemplate self, CapeUdc node, void* ptr, fct_cape_template__on_text on_text, fct_cape_template__on_file on_file, fct_cape_template__on_pipe on_pipe, fct_cape_template__on_tag on_tag, CapeErr err)
{
    int res;
    
    // local objects
    QTeeTemplateCB cb = qtee_template_cb_new (ptr, on_text, on_file, on_pipe, on_tag);
  
    // a list to store all nodes in top down order
    CapeList node_stack = cape_list_new (NULL);

    // first entry
    cape_list_push_back (node_stack, node);
  
    res = qtee_part_apply (self->root_part, node_stack, cb, 0, err);

    // cleanup
    cape_list_del (&node_stack);
    qtee_template_cb_del (&cb);

    return res;
}

//-----------------------------------------------------------------------------

int __STDCALL cape_eval__on_text (void* ptr, const char* bufdat, number_t buflen)
{
  cape_stream_append_buf (ptr, bufdat, buflen);

  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

CapeString cape_template_run (const CapeString s, CapeUdc node, fct_cape_template__on_pipe on_pipe, fct_cape_template__on_tag on_tag, CapeErr err)
{
  CapeString ret = NULL;
  int res;
  
  // local objects
  CapeTemplate tmpl = cape_template_new ();
  CapeStream stream = cape_stream_new ();

  // compile into parts
  cape_template_compile_str (tmpl, s);

  res = cape_template_apply (tmpl, node, stream, cape_eval__on_text, NULL, on_pipe, on_tag, err);
  if (res)
  {
    goto exit_and_cleanup;
  }

  ret = cape_stream_to_str (&stream);
  
exit_and_cleanup:

  cape_template_del (&tmpl);
  cape_stream_del (&stream);

  return ret;
}

//-----------------------------------------------------------------------------
