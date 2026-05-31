#include "qtee_template.h"
#include "qtee_template_part.h"
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
  
  QTeeTemplatePart root_part;
  
};

//-----------------------------------------------------------------------------

struct QTeeTemplateCompiler_s
{
  
    int state;
    
    CapeStream sb;
    
    QTeeTemplatePart part;   // reference

}; typedef struct QTeeTemplateCompiler_s* QTeeTemplateCompiler;

//-----------------------------------------------------------------------------

QTeeTemplateCompiler qtee_template_compiler_new (QTeeTemplatePart part)
{
    QTeeTemplateCompiler self = CAPE_NEW (struct QTeeTemplateCompiler_s);
  
    self->state = 0;
    self->sb = cape_stream_new ();
  
    self->part = part;
  
    return self;
}

//-----------------------------------------------------------------------------

void qtee_template_compiler_del (QTeeTemplateCompiler* p_self)
{
    if (*p_self)
    {
        QTeeTemplateCompiler self = *p_self;
      
        cape_stream_del (&(self->sb));
      
        CAPE_DEL (p_self, struct QTeeTemplateCompiler_s);
    }
}

//-----------------------------------------------------------------------------

void qtee_template_compiler_module__parse (const CapeString buf, CapeString* p_name, CapeString* p_cont, CapeString *p_fomt)
{
  number_t size = cape_str_size (buf);
  
  const char* c = buf;
  int i;
  number_t state = 0;

  // local objects
  CapeStream s = cape_stream_new ();

  for (i = 0; i < size; i++, c++)
  {
    switch (state)
    {
      case 0:
      {
        if (*c == '{')
        {
          *p_name = cape_str_sub (buf, i);
          state = 1;
        }

        break;
      }
      case 1:
      {
        if (*c == '}')
        {
          goto exit_and_cleanup;
        }
        else
        {
          cape_stream_append_c (s, *c);
        }

        break;
      }
    }
  }
  
exit_and_cleanup:
  
  *p_fomt = cape_str_sub (c, size - i);
  *p_cont = cape_stream_to_str (&s);
}

//-----------------------------------------------------------------------------

void qtee_template_compiler_module (QTeeTemplateCompiler self, const CapeString raw_name)
{
    // local objects
    CapeString name = NULL;
    CapeString cont = NULL;
    CapeString fomt = NULL;
    
    // use a small parser to retrieve the 3 string needed for a part module
    qtee_template_compiler_module__parse (raw_name, &name, &cont, &fomt);

    {
        QTeeTemplatePart tmplpart = qtee_template_part_new (PART_TYPE_MOD, fomt, self->part);

        qtee_template_part_set (tmplpart, &cont, &name);
        
        // add tmplpart as child
        qtee_template_part_add (self->part, tmplpart);
    }

    cape_str_del (&name);
    cape_str_del (&cont);
    cape_str_del (&fomt);
}

//-----------------------------------------------------------------------------

void qtee_template_compiler_part (QTeeTemplateCompiler self, int type)
{
    switch (type)
    {
        case PART_TYPE_TEXT:
        case PART_TYPE_FILE:
        {
            if (cape_stream_size (self->sb) > 0)
            {
                const CapeString text = cape_stream_get (self->sb);
                
                qtee_template_part_add (self->part, qtee_template_part_new (type, text, NULL));
                cape_stream_clr (self->sb);
            }
            
            break;
        }
        case PART_TYPE_TAG:
        {
            const CapeString name = cape_stream_get (self->sb);
            switch (name[0])
            {
              case '#':
              {
                  QTeeTemplatePart new_part = qtee_template_part_new (type, name + 1, self->part);
                  
                  // add the new part to the current part
                  qtee_template_part_add (self->part, new_part);
                  
                  // now change the current part to the new part, that we go one level up
                  self->part = new_part;
                  
                  break;
              }
              case '/':
              {
                  // is the current part the ending tag
                  if (qtee_template_part_equal (self->part, name + 1))
                  {
                      // has the current part a parent
                      QTeeTemplatePart parent_part = qtee_template_part_parent (self->part);
                      if (parent_part)
                      {
                          // change back the current part to the parent, that we go one level down
                          self->part = parent_part;
                      }
                  }

                  break;
              }
              case '$':   // extra modules
              {
                  qtee_template_compiler_module (self, name);
                  
                  break;
              }
              default:
              {
                  qtee_template_part_add (self->part, qtee_template_part_new (type, name, self->part));
                  
                  break;
              }
            }
            
            // always clear
            cape_stream_clr (self->sb);
            
            break;
        }
    }
}

//-----------------------------------------------------------------------------

#define QTEE_TEMPLATE_PARSE_TYPE__NONE            0
#define QTEE_TEMPLATE_PARSE_TYPE__ITEM_PRE        1
#define QTEE_TEMPLATE_PARSE_TYPE__ITEM            2
#define QTEE_TEMPLATE_PARSE_TYPE__ITEM_RET        3
#define QTEE_TEMPLATE_PARSE_TYPE__FILE_PRE        4
#define QTEE_TEMPLATE_PARSE_TYPE__FILE            5
#define QTEE_TEMPLATE_PARSE_TYPE__FILE_RET        6
#define QTEE_TEMPLATE_PARSE_TYPE__ESCS_PRE        7
#define QTEE_TEMPLATE_PARSE_TYPE__ESCS            8
#define QTEE_TEMPLATE_PARSE_TYPE__ESCS_RET        9
#define QTEE_TEMPLATE_PARSE_TYPE__SPECIAL        10

#define QTEE_TEMPLATE_SYNTAX_ITEM_S              '{'
#define QTEE_TEMPLATE_SYNTAX_ITEM_E              '}'
#define QTEE_TEMPLATE_SYNTAX_FILE_S              '['
#define QTEE_TEMPLATE_SYNTAX_FILE_E              ']'
#define QTEE_TEMPLATE_SYNTAX_ESCS_S              '('
#define QTEE_TEMPLATE_SYNTAX_ESCS_E              ')'

//-----------------------------------------------------------------------------

int qtee_template_compiler_parse (QTeeTemplateCompiler self, const char* buffer, number_t size, CapeErr err)
{
    int res;
    
    const char* c = buffer;
    int i;
    
    for (i = 0; i < size; i++, c++)
    {
        switch (self->state)
        {
            case QTEE_TEMPLATE_PARSE_TYPE__NONE:
            {
                // start with a simple devider
                if (QTEE_TEMPLATE_SYNTAX_ITEM_S == *c)
                {
                    self->state = QTEE_TEMPLATE_PARSE_TYPE__ITEM_PRE;
                }
                else if (QTEE_TEMPLATE_SYNTAX_FILE_S == *c)
                {
                    self->state = QTEE_TEMPLATE_PARSE_TYPE__FILE_PRE;
                }
                else if (QTEE_TEMPLATE_SYNTAX_ESCS_S == *c)
                {
                    self->state = QTEE_TEMPLATE_PARSE_TYPE__ESCS_PRE;
                }
                else
                {
                    cape_stream_append_c (self->sb, *c);
                }

                break;
            }
            case QTEE_TEMPLATE_PARSE_TYPE__ITEM_PRE:
            {
                if (QTEE_TEMPLATE_SYNTAX_ITEM_S == *c)
                {
                    qtee_template_compiler_part (self, PART_TYPE_TEXT);

                    self->state = QTEE_TEMPLATE_PARSE_TYPE__ITEM;
                }
                else
                {
                    self->state = QTEE_TEMPLATE_PARSE_TYPE__NONE;
                    
                    // add missing
                    cape_stream_append_c (self->sb, QTEE_TEMPLATE_SYNTAX_ITEM_S);
                    cape_stream_append_c (self->sb, *c);
                }

                break;
            }
            case QTEE_TEMPLATE_PARSE_TYPE__ITEM:
            {
                if (QTEE_TEMPLATE_SYNTAX_ITEM_E == *c)
                {
                    self->state = QTEE_TEMPLATE_PARSE_TYPE__ITEM_RET;
                }
                else
                {
                    cape_stream_append_c (self->sb, *c);
                }

                break;
            }
            case QTEE_TEMPLATE_PARSE_TYPE__ITEM_RET:
            {
                if (QTEE_TEMPLATE_SYNTAX_ITEM_E == *c)
                {
                    qtee_template_compiler_part (self, PART_TYPE_TAG);
                    
                    self->state = QTEE_TEMPLATE_PARSE_TYPE__NONE;
                }
                else
                {
                    self->state = QTEE_TEMPLATE_PARSE_TYPE__ITEM;
                    
                    // add missing
                    cape_stream_append_c (self->sb, QTEE_TEMPLATE_SYNTAX_ITEM_E);
                    cape_stream_append_c (self->sb, *c);
                }

                break;
            }
            case QTEE_TEMPLATE_PARSE_TYPE__FILE_PRE:
            {
                if (QTEE_TEMPLATE_SYNTAX_FILE_S == *c)
                {
                    qtee_template_compiler_part (self, PART_TYPE_TEXT);
                    
                    self->state = QTEE_TEMPLATE_PARSE_TYPE__FILE;
                }
                else
                {
                    self->state = QTEE_TEMPLATE_PARSE_TYPE__NONE;
                    
                    // add missing
                    cape_stream_append_c (self->sb, QTEE_TEMPLATE_SYNTAX_FILE_S);
                    cape_stream_append_c (self->sb, *c);
                }

                break;
            }
            case QTEE_TEMPLATE_PARSE_TYPE__FILE:
            {
                if (QTEE_TEMPLATE_SYNTAX_FILE_E == *c)
                {
                    self->state = QTEE_TEMPLATE_PARSE_TYPE__FILE_RET;
                }
                else
                {
                    cape_stream_append_c (self->sb, *c);
                }

                break;
            }
            case QTEE_TEMPLATE_PARSE_TYPE__FILE_RET:
            {
                if (QTEE_TEMPLATE_SYNTAX_FILE_E == *c)
                {
                    qtee_template_compiler_part (self, PART_TYPE_FILE);

                    self->state = QTEE_TEMPLATE_PARSE_TYPE__NONE;
                }
                else
                {
                    // add missing
                    cape_stream_append_c (self->sb, QTEE_TEMPLATE_SYNTAX_FILE_E);
                    cape_stream_append_c (self->sb, *c);

                    self->state = QTEE_TEMPLATE_PARSE_TYPE__FILE;
                }

                break;
            }
            case QTEE_TEMPLATE_PARSE_TYPE__ESCS_PRE:
            {
                if (QTEE_TEMPLATE_SYNTAX_ESCS_S == *c)
                {
                    self->state = QTEE_TEMPLATE_PARSE_TYPE__ESCS;
                }
                else
                {
                    self->state = QTEE_TEMPLATE_PARSE_TYPE__NONE;

                    // add missing
                    cape_stream_append_c (self->sb, QTEE_TEMPLATE_SYNTAX_ESCS_S);
                    cape_stream_append_c (self->sb, *c);
                }

                break;
            }
            case QTEE_TEMPLATE_PARSE_TYPE__ESCS:
            {
                if (QTEE_TEMPLATE_SYNTAX_ESCS_E == *c)
                {
                    self->state = QTEE_TEMPLATE_PARSE_TYPE__ESCS_RET;
                }
                else
                {
                    cape_stream_append_c (self->sb, *c);
                }

                break;
            }
            case QTEE_TEMPLATE_PARSE_TYPE__ESCS_RET:
            {
                if (QTEE_TEMPLATE_SYNTAX_ESCS_E == *c)
                {
                    self->state = QTEE_TEMPLATE_PARSE_TYPE__NONE;
                }
                else
                {
                    self->state = QTEE_TEMPLATE_PARSE_TYPE__ESCS;
                    
                    // add missing
                    cape_stream_append_c (self->sb, QTEE_TEMPLATE_SYNTAX_ESCS_E);
                    cape_stream_append_c (self->sb, *c);
                }

                break;
            }
        }
    }
    
    // add last part as text
    qtee_template_compiler_part (self, PART_TYPE_TEXT);
    
    return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

static int __STDCALL qtee_template_compile__on_buf (void* ptr, const char* bufdat, number_t buflen, CapeErr err)
{
    return qtee_template_compiler_parse (ptr, bufdat, buflen, err);
}

//-----------------------------------------------------------------------------

int cape_template_compile (CapeTemplate self, const char* path, CapeErr err)
{
    int res;
  
    // local objects
    QTeeTemplateCompiler tcl = qtee_template_compiler_new (self->root_part);

    // open the file and parse the content
    res = cape_fs_file_load (path, self->fileName, tcl, qtee_template_compile__on_buf, err);

    qtee_template_compiler_del (&tcl);
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
  
  self->root_part = qtee_template_part_new (PART_TYPE_TAG, NULL, NULL);
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
    qtee_template_part_del (&(self->root_part));
    
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

int cape_template_compile_str (CapeTemplate self, const char* content, CapeErr err)
{
    int res;
  
    // local object
    QTeeTemplateCompiler tcl = qtee_template_compiler_new (self->root_part);
    
    // parse the input string into qtee parts, results into root_part
    res = qtee_template_compiler_parse (tcl, content, cape_str_size (content), err);
    
    qtee_template_compiler_del (&tcl);
    return res;
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
  
    res = qtee_template_part_apply (self->root_part, node_stack, cb, 0, err);

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
  
  res = cape_template_compile_str (tmpl, s, err);
  if (res)
  {
    goto exit_and_cleanup;
  }

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
