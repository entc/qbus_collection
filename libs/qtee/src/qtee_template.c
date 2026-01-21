#include "qtee_template.h"
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

#define PART_TYPE_NONE   0
#define PART_TYPE_TEXT   1
#define PART_TYPE_FILE   2
#define PART_TYPE_TAG    3
#define PART_TYPE_NODE   4
#define PART_TYPE_MOD    6

//-----------------------------------------------------------------------------

struct CapeTemplateCB_s
{
  void* ptr;
  
  // references to callback functions
  fct_cape_template__on_text on_text;
  fct_cape_template__on_file on_file;
  fct_cape_template__on_pipe on_pipe;
  fct_cape_template__on_tag on_tag;
  
}; typedef struct CapeTemplateCB_s* CapeTemplateCB;

//-----------------------------------------------------------------------------

struct CapeTemplatePart_s; typedef struct CapeTemplatePart_s* CapeTemplatePart;

struct CapeTemplatePart_s
{
  
  int type;
  
  CapeString text;
  
  CapeString eval;
  
  CapeString modn;
  
  CapeList parts;
  
  CapeTemplatePart parent;

  QTeeFormat format;
};

//-----------------------------------------------------------------------------

void cape_template_part_checkForEval (CapeTemplatePart self, const CapeString text)
{
  switch (self->type)
  {
    case PART_TYPE_TAG:
    case PART_TYPE_MOD:
    {
      CapeString s1 = NULL;
      CapeString s2 = NULL;
      
      // try to split with '='
      if (cape_tokenizer_split (text, '=', &s1, &s2))
      {
        self->text = cape_str_trim_utf8 (s1);
        self->eval = cape_str_trim_utf8 (s2);
      }
      else
      {
        self->format = qtee_format_gen (text);
      }

      cape_str_del (&s1);
      cape_str_del (&s2);
      
      break;
    }
    case PART_TYPE_FILE:
    {
      self->format = qtee_format_gen (text);      
      break;
    }
    default:
    {
      self->text = cape_str_cp (text);
      break;
    }
  }
}

//-----------------------------------------------------------------------------

CapeTemplatePart cape_template_part_new (int type, const CapeString raw_text, CapeTemplatePart parent)
{
  CapeTemplatePart self = CAPE_NEW (struct CapeTemplatePart_s);
  
  self->type = type;
  
  self->text = NULL;
  self->eval = NULL;
  self->modn = NULL;
    
  self->parts = NULL;
  self->parent = parent;
  
  self->format = NULL;
  
  // analyse the text value for later evaluation
  cape_template_part_checkForEval (self, raw_text);
  
  return self;
}

//-----------------------------------------------------------------------------

void cape_template_part_del (CapeTemplatePart* p_self)
{
  CapeTemplatePart self = *p_self;
  
  qtee_format_del (&(self->format));
  
  cape_str_del (&(self->text));
  cape_str_del (&(self->eval));
  cape_str_del (&(self->modn));

  if (self->parts)
  {
    cape_list_del (&(self->parts));
  }
  
  CAPE_DEL (p_self, struct CapeTemplatePart_s);
}

//-----------------------------------------------------------------------------

static void __STDCALL cape_template_create_parts_onDestroy (void* ptr)
{
  CapeTemplatePart h = ptr; cape_template_part_del (&h);
}

//-----------------------------------------------------------------------------

int cape_template_part_hasText (CapeTemplatePart self, const CapeString text)
{
  return cape_str_equal (self->text, text);
}

//-----------------------------------------------------------------------------

CapeTemplatePart cape_template_part_parent (CapeTemplatePart self)
{
  return self->parent;
}

//-----------------------------------------------------------------------------

void cape_template_part_add (CapeTemplatePart self, CapeTemplatePart part)
{
  if (self->parts == NULL)
  {
    self->parts = cape_list_new (cape_template_create_parts_onDestroy);
  }
  
  cape_list_push_back (self->parts, part);
}

//-----------------------------------------------------------------------------

int cape_template_part_apply (CapeTemplatePart self, CapeList node_stack, CapeTemplateCB cb, number_t pos, CapeErr err);

//-----------------------------------------------------------------------------

int cape_template_file_apply (CapeTemplatePart self, CapeTemplatePart part, CapeList node_stack, CapeTemplateCB cb, CapeErr err)
{
  int res;
  const CapeString name = part->text;

  if (cb->on_file == NULL)
  {
    res = CAPE_ERR_NONE;
  }
  else
  {
    CapeUdc found_item = qtee_format_item (self->format, node_stack);
    
    if (found_item)
    {
      CapeString filepath = qtee_format_apply_node (self->format, found_item, cb->on_pipe);
      if (filepath)
      {
        number_t flags = CAPE_TEMPLATE_FLAG__NONE;
        
        if (qtee_format_has_encrypted (self->format))
        {
          flags |= CAPE_TEMPLATE_FLAG__ENCRYPTED;
        }
        
        res = cb->on_file (cb->ptr, filepath, flags, err);
        
        cape_str_del (&filepath);
      }
      else
      {
        res = CAPE_ERR_NONE;
      }
    }
  }
    
  return res;
}

//-----------------------------------------------------------------------------

double cape_template_math (const CapeString formular, CapeList node_stack)
{
  double ret = .0;
  
  CapeString le = NULL;
  CapeString re = NULL;
  
  cape_log_fmt (CAPE_LL_TRACE, "CAPE", "template math", "using the formular: '%s'", formular);

  if (cape_tokenizer_split (formular, '+', &le, &re))
  {
    CapeString lh = cape_str_trim_utf8 (le);
    CapeString rh = cape_str_trim_utf8 (re);

    double lv = cape_template_math (lh, node_stack);
    double rv = cape_template_math (rh, node_stack);

    ret = lv + rv;
    
    cape_str_del (&lh);
    cape_str_del (&rh);
  }
  else if (cape_tokenizer_split (formular, '-', &le, &re))
  {
    CapeString lh = cape_str_trim_utf8 (le);
    CapeString rh = cape_str_trim_utf8 (re);

    double lv = cape_template_math (lh, node_stack);
    double rv = cape_template_math (rh, node_stack);

    ret = lv - rv;
    
    cape_str_del (&lh);
    cape_str_del (&rh);
  }
  else if (cape_tokenizer_split (formular, '*', &le, &re))
  {
    CapeString lh = cape_str_trim_utf8 (le);
    CapeString rh = cape_str_trim_utf8 (re);

    double lv = cape_template_math (lh, node_stack);
    double rv = cape_template_math (rh, node_stack);

    ret = lv * rv;
    
    cape_str_del (&lh);
    cape_str_del (&rh);
  }
  else if (cape_tokenizer_split (formular, '/', &le, &re))
  {
    CapeString lh = cape_str_trim_utf8 (le);
    CapeString rh = cape_str_trim_utf8 (re);

    double lv = cape_template_math (lh, node_stack);
    double rv = cape_template_math (rh, node_stack);

    ret = lv / rv;
    
    cape_str_del (&lh);
    cape_str_del (&rh);
  }
  else
  {
    CapeUdc item = qtee_format_seek_item (node_stack, formular);
    if (item)
    {
      switch (cape_udc_type (item))
      {
        case CAPE_UDC_STRING:
        {
          const CapeString h = cape_udc_s (item, NULL);
          if (h)
          {
            ret = cape_str_to_f (h);
            
            cape_log_fmt (CAPE_LL_TRACE, "CAPE", "template math", "found item %s = '%f'", formular, ret);
          }
          else
          {
            cape_log_fmt (CAPE_LL_WARN, "CAPE", "template math", "found item can't be formatted into a float: %s = '%s'", formular, h);
          }

          break;
        }
        case CAPE_UDC_NUMBER:
        {
          ret = cape_udc_n (item, 0);

          cape_log_fmt (CAPE_LL_TRACE, "CAPE", "template math", "found item %s = '%f'", formular, ret);
          break;
        }
        case CAPE_UDC_FLOAT:
        {
          ret = cape_udc_f (item, .0);

          cape_log_fmt (CAPE_LL_TRACE, "CAPE", "template math", "found item %s = '%f'", formular, ret);
          break;
        }
      }
    }
    else
    {
      ret = cape_str_to_f (formular);
    }
  }
    
  cape_str_del (&le);
  cape_str_del (&re);
  
  //printf ("RET: %f\n", ret);
  
  return ret;
}

//-----------------------------------------------------------------------------

int cape_template_mod_apply__math (CapeTemplatePart self, CapeList node_stack, CapeTemplateCB cb, CapeErr err)
{
  double value = cape_template_math (self->text, node_stack);
  
  CapeString h = qtee_format_apply_f (self->format, value, cb->on_pipe);
  if (h)
  {
    if (cb->on_text)
    {
      cb->on_text (cb->ptr, h, cape_str_size (h));
    }
    
    cape_str_del (&h);
  }
    
  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

CapeDatetime* cape_template_date (const CapeString formular, CapeList node_stack)
{
  CapeDatetime* ret = NULL;
  
  CapeString le = NULL;
  CapeString re = NULL;

  if (cape_tokenizer_split (formular, '+', &le, &re))
  {
    CapeString lh = cape_str_trim_utf8 (le);
    CapeString rh = cape_str_trim_utf8 (re);

    CapeUdc item = qtee_format_seek_item (node_stack, lh);
    if (item)
    {
      switch (cape_udc_type (item))
      {
        case CAPE_UDC_DATETIME:
        {
          const CapeDatetime* dt = cape_udc_d (item, NULL);
          if (dt)
          {
            ret = cape_datetime_cp (dt);
            cape_datetime__add_s (dt, rh, ret);
          }

          break;
        }
        case CAPE_UDC_STRING:
        {
          CapeDatetime dt;
          
          const CapeString text = cape_udc_s (item, NULL);
          
          // convert text into dateformat
          if (cape_datetime__str_msec (&dt, text) || cape_datetime__str (&dt, text) || cape_datetime__std_msec (&dt, text) || cape_datetime__date_de (&dt, text))
          {
            ret = cape_datetime_cp (&dt);
            cape_datetime__add_s (&dt, rh, ret);
          }
          else
          {
            cape_log_fmt (CAPE_LL_ERROR, "CAPE", "template eval", "can't evaluate '%s' as date", text);
          }

          break;
        }
      }
    }
    
    cape_str_del (&lh);
    cape_str_del (&rh);
  }
  else if (cape_tokenizer_split (formular, '-', &le, &re))
  {
    CapeString lh = cape_str_trim_utf8 (le);
    CapeString rh = cape_str_trim_utf8 (re);

    
    cape_str_del (&lh);
    cape_str_del (&rh);
  }

  cape_str_del (&le);
  cape_str_del (&re);

  return ret;
}

//-----------------------------------------------------------------------------

int cape_template_mod_apply__date (CapeTemplatePart self, CapeList node_stack, CapeTemplateCB cb, CapeErr err)
{
  CapeDatetime* value = cape_template_date (self->text, node_stack);
  if (value)
  {
    CapeString h = qtee_format_apply_d (self->format, value, cb->on_pipe);
  
    if (cb->on_text)
    {
      cb->on_text (cb->ptr, h, cape_str_size (h));
    }
    
    cape_datetime_del (&value);
  }
  
  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

int cape_template_tag_apply (CapeTemplatePart self, CapeList node_stack, CapeTemplateCB cb, number_t pos, CapeErr err)
{
  CapeUdc found_item = NULL;
  
  if (self->format)
  {
    found_item = qtee_format_item (self->format, node_stack);
  }
  else
  {
    cape_log_msg (CAPE_LL_WARN, "QTEE", "template", "no format defined");
  }

  // call the tag callback
  if (cb->on_tag)
  {
    cb->on_tag (cb->ptr, self->text);
  }
  
  if (found_item)
  {
    switch (cape_udc_type (found_item))
    {
      case CAPE_UDC_NODE:
      {
        // add a next level
        cape_list_push_back (node_stack, found_item);
        
        int res = cape_template_part_apply (self, node_stack, cb, pos, err);
        
        cape_list_pop_back (node_stack);
        
        if (res)
        {
          return res;
        }
        
        break;
      }
      case CAPE_UDC_LIST:
      {
        CapeUdcCursor* cursor_item = cape_udc_cursor_new (found_item, CAPE_DIRECTION_FORW);
        
        while (cape_udc_cursor_next (cursor_item))
        {
          // add a next level
          cape_list_push_back (node_stack, cursor_item->item);
          
          int res = cape_template_part_apply (self, node_stack, cb, cursor_item->position, err);
          
          cape_list_pop_back (node_stack);
          
          if (res)
          {
            return res;
          }
        }
        
        cape_udc_cursor_del (&cursor_item);
        
        break;
      }
      case CAPE_UDC_STRING:
      {
        if (self->eval)
        {
          if (qtee_compare (self->eval, cape_udc_s (found_item, "")))
          {
            cape_template_part_apply (self, node_stack, cb, 0, err);
          }
        }
        else
        {
          CapeString value = qtee_format_apply_s (self->format, cape_udc_s (found_item, NULL), cb->on_pipe);
          
          if (value)
          {
            if (cb->on_text)
            {
              cb->on_text (cb->ptr, value, cape_str_size (value));
            }
            
            cape_str_del (&value);
          }
        }
        
        break;
      }
      case CAPE_UDC_NUMBER:
      {
        if (self->eval)
        {
          number_t h = strtol (self->eval, NULL, 10);
          
          if (h == cape_udc_n (found_item, 0))
          {
            cape_template_part_apply (self, node_stack, cb, 0, err);
          }
        }
        else
        {
          CapeString value = qtee_format_apply_n (self->format, cape_udc_n (found_item, 0), cb->on_pipe);
          
          if (value)
          {
            if (cb->on_text)
            {
              cb->on_text (cb->ptr, value, cape_str_size (value));
            }
            
            cape_str_del (&value);
          }
        }
        
        break;
      }
      case CAPE_UDC_FLOAT:
      {
        if (self->eval)
        {
          double h = cape_str_to_f (self->eval);
          
          if (h == cape_udc_f (found_item, .0))
          {
            cape_template_part_apply (self, node_stack, cb, 0, err);
          }
        }
        else
        {
          CapeString value = qtee_format_apply_f (self->format, cape_udc_f (found_item, .0), cb->on_pipe);
          
          if (value)
          {
            if (cb->on_text)
            {
              cb->on_text (cb->ptr, value, cape_str_size (value));
            }
            
            cape_str_del (&value);
          }
        }
        
        break;
      }
      case CAPE_UDC_BOOL:
      {
        if (self->eval)
        {
          int h = cape_udc_b (found_item, FALSE);
          
          if ((h == TRUE) && cape_str_equal (self->eval, "TRUE"))
          {
            cape_template_part_apply (self, node_stack, cb, 0, err);
          }
          else if ((h == FALSE) && cape_str_equal (self->eval, "FALSE"))
          {
            cape_template_part_apply (self, node_stack, cb, 0, err);
          }
        }
        else
        {
          CapeString value = qtee_format_apply_b (self->format, cape_udc_b (found_item, FALSE), cb->on_pipe);
          
          if (value)
          {
            if (cb->on_text)
            {
              cb->on_text (cb->ptr, value, cape_str_size (value));
            }
            
            cape_str_del (&value);
          }
        }
        
        break;
      }
      case CAPE_UDC_DATETIME:
      {
        if (self->eval)
        {
         
          
        }
        else
        {
          CapeString value = qtee_format_apply_d (self->format, cape_udc_d (found_item, NULL), cb->on_pipe);
          
          if (value)
          {
            if (cb->on_text)
            {
              cb->on_text (cb->ptr, value, cape_str_size (value));
            }
            
            cape_str_del (&value);
          }
        }
        
        break;
      }
      case CAPE_UDC_STREAM:
      {
        if (self->eval)
        {
          
          
        }
        else
        {
          // TODO: add stream also to the format
          CapeStream m = cape_udc_m (found_item);
          
          if (cb->on_text)
          {
            cb->on_text (cb->ptr, cape_stream_data (m), cape_stream_size (m));
          }
        }
      }
    }
  }
  
  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

int cape_template_part_apply (CapeTemplatePart self, CapeList node_stack, CapeTemplateCB cb, number_t pos, CapeErr err)
{
  if (self->parts)
  {
    CapeListCursor* cursor = cape_list_cursor_new (self->parts, CAPE_DIRECTION_FORW);
    
    while (cape_list_cursor_next (cursor))
    {
      CapeTemplatePart part = cape_list_node_data (cursor->node);
      
      switch (part->type)
      {
        case PART_TYPE_TEXT:
        {
          if (cb->on_text)
          {
            cb->on_text (cb->ptr, part->text, cape_str_size (part->text));
          }

          break;
        }
        case PART_TYPE_FILE:
        {
          int res = cape_template_file_apply (self, part, node_stack, cb, err);
          if (res)
          {
            return res;
          }

          break;
        }
        case PART_TYPE_MOD:
        {
          if (cape_str_not_empty (part->modn))
          {
            // check all modules by name
            if (cape_str_equal (part->modn + 1, "math"))
            {
              int res = cape_template_mod_apply__math (part, node_stack, cb, err);
              if (res)
              {
                return res;
              }
            }
            else if (cape_str_equal (part->modn + 1, "date"))
            {
              int res = cape_template_mod_apply__date (part, node_stack, cb, err);
              if (res)
              {
                return res;
              }
            }
            else
            {
              // add more here
            }
          }

          break;
        }
        case PART_TYPE_TAG:
        {
          if (cape_str_equal (part->text, "INDEX_1"))
          {
            CapeString h = cape_str_n (pos + 1);
            
            if (cb->on_text)
            {
              cb->on_text (cb->ptr, h, cape_str_size (h));
            }
            
            cape_str_del (&h);
          }
          else
          {
            cape_template_tag_apply (part, node_stack, cb, cursor->position, err);
          }
          
          break;
        }
      }
    }
    
    cape_list_cursor_destroy (&cursor);
  }
  
  return CAPE_ERR_NONE;
}


//-----------------------------------------------------------------------------

struct CapeTemplate_s
{
  
  CapeString fileName;
  
  CapeTemplatePart root_part;
  
};

//-----------------------------------------------------------------------------

struct EcTemplateCompiler_s
{
  
  int state;
  
  CapeStream sb;
  
  CapeTemplatePart part;   // reference
  
}; typedef struct EcTemplateCompiler_s* EcTemplateCompiler;

//-----------------------------------------------------------------------------

EcTemplateCompiler cape_template_compiler_new (CapeTemplatePart part)
{
  EcTemplateCompiler self = CAPE_NEW (struct EcTemplateCompiler_s);
  
  self->state = 0;
  self->sb = cape_stream_new ();
  
  self->part = part;
  
  return self;
}

//-----------------------------------------------------------------------------

void cape_template_compiler_del (EcTemplateCompiler* p_self)
{
  EcTemplateCompiler self = *p_self;
  
  cape_stream_del (&(self->sb));
  
  CAPE_DEL (p_self, struct EcTemplateCompiler_s);
}

//-----------------------------------------------------------------------------

void cape_template_compiler_module__parse (const CapeString buf, CapeString* p_name, CapeString* p_cont, CapeString *p_fomt)
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

void cape_template_compiler_module (EcTemplateCompiler self, const CapeString raw_name)
{
  // local objects
  CapeString name = NULL;
  CapeString cont = NULL;
  CapeString fomt = NULL;
  
  cape_template_compiler_module__parse (raw_name, &name, &cont, &fomt);

  {
    CapeTemplatePart tmplpart = cape_template_part_new (PART_TYPE_MOD, fomt, self->part);

    cape_str_replace_mv (&(tmplpart->text), &cont);
    cape_str_replace_mv (&(tmplpart->modn), &name);

    cape_template_part_add (self->part, tmplpart);
  }

  cape_str_del (&name);
  cape_str_del (&cont);
  cape_str_del (&fomt);
}

//-----------------------------------------------------------------------------

int cape_template_compiler_part (EcTemplateCompiler self, int type, CapeErr err)
{
  switch (type)
  {
    case PART_TYPE_TEXT:
    case PART_TYPE_FILE:
    {
      if (cape_stream_size (self->sb) > 0)
      {
        const CapeString text = cape_stream_get (self->sb);
        
        cape_template_part_add (self->part, cape_template_part_new (type, text, NULL));
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
          CapeTemplatePart new_part = cape_template_part_new (type, name + 1, self->part);
          
          // add the new part to the current part
          cape_template_part_add (self->part, new_part);
          
          // now change the current part to the new part, that we go one level up
          self->part = new_part;
          
          break;
        }
        case '/':
        {
          // is the current part the ending tag
          if (cape_template_part_hasText (self->part, name + 1))
          {
            // has the current part a parent
            CapeTemplatePart parent_part = cape_template_part_parent (self->part);
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
          cape_template_compiler_module (self, name);
          
          break;
        }
        default:
        {
          cape_template_part_add (self->part, cape_template_part_new (type, name, self->part));
          
          break;
        }
      }
      
      // always clear
      cape_stream_clr (self->sb);
      
      break;
    }
  }
  
  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

int cape_template_compiler_parse (EcTemplateCompiler self, const char* buffer, number_t size, CapeErr err)
{
  int res;
  
  const char* c = buffer;
  int i;
  
  for (i = 0; i < size; i++, c++)
  {
    switch (self->state)
    {
      case 0:
      {
        if (*c == '{')
        {
          self->state = 1;
        }
        else if (*c == '[')
        {
          self->state = 4;
        }
        else
        {
          cape_stream_append_c (self->sb, *c);
        }

        break;
      }
      case 1:
      {
        if (*c == '{')
        {
          res = cape_template_compiler_part (self, PART_TYPE_TEXT, err);
          if (res)
          {
            return res;
          }
          
          self->state = 2;
        }
        else
        {
          self->state = 0;
          cape_stream_append_c (self->sb, '{');
          cape_stream_append_c (self->sb, *c);
        }

        break;
      }
      case 2:
      {
        if (*c == '}')
        {
          self->state = 3;
        }
        else
        {
          cape_stream_append_c (self->sb, *c);
        }

        break;
      }
      case 3:
      {
        if (*c == '}')
        {
          res = cape_template_compiler_part (self, PART_TYPE_TAG, err);
          if (res)
          {
            return res;
          }
          
          self->state = 7;
        }
        else
        {
          self->state = 2;
          cape_stream_append_c (self->sb, '}');
          cape_stream_append_c (self->sb, *c);
        }

        break;
      }
      case 4:
      {
        if (*c == '[')
        {
          res = cape_template_compiler_part (self, PART_TYPE_TEXT, err);
          if (res)
          {
            return res;
          }
          
          self->state = 5;
        }
        else
        {
          self->state = 0;
          cape_stream_append_c (self->sb, '[');
          cape_stream_append_c (self->sb, *c);
        }

        break;
      }
      case 5:
      {
        if (*c == ']')
        {
          self->state = 6;
        }
        else
        {
          cape_stream_append_c (self->sb, *c);
        }

        break;
      }
      case 6:
      {
        if (*c == ']')
        {
          res = cape_template_compiler_part (self, PART_TYPE_FILE, err);
          if (res)
          {
            return res;
          }
          
          self->state = 7;
        }
        else
        {
          self->state = 5;
        }

        break;
      }
      case 7:   // special state
      {
        if ((*c == '\n')||(*c == '\r'))
        {
         // cape_stream_append_c (self->sb, *c);
          
          /*
          res = cape_template_compiler_part (self, PART_TYPE_CR, err);
          if (res)
          {
            return res;
          }
          */
          
          self->state = 0;
        }
        else if (*c == '{')
        {
          self->state = 1;
        }
        else if (*c == '[')
        {
          self->state = 4;
        }
        else
        {
          cape_stream_append_c (self->sb, *c);
          self->state = 0;
        }

        break;
      }
    }
  }
  
  // add last part as text
  return cape_template_compiler_part (self, PART_TYPE_TEXT, err);
}

//-----------------------------------------------------------------------------

static int __STDCALL cape_template_compile__on_buf (void* ptr, const char* bufdat, number_t buflen, CapeErr err)
{
  return cape_template_compiler_parse (ptr, bufdat, buflen, err);
}

//-----------------------------------------------------------------------------

int cape_template_compile (CapeTemplate self, const char* path, CapeErr err)
{
  int res;
  
  EcTemplateCompiler tcl = cape_template_compiler_new (self->root_part);

  // open the file and parse the content
  res = cape_fs_file_load (path, self->fileName, tcl, cape_template_compile__on_buf, err);

  cape_template_compiler_del (&tcl);

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
  
  self->root_part = cape_template_part_new (PART_TYPE_TAG, NULL, NULL);
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
    cape_template_part_del (&(self->root_part));
    
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
  
  EcTemplateCompiler tcl = cape_template_compiler_new (self->root_part);
  
  res = cape_template_compiler_parse (tcl, content, cape_str_size (content), err);
  
  cape_template_compiler_del (&tcl);
  
  return res;
}

//-----------------------------------------------------------------------------

int cape_template_apply (CapeTemplate self, CapeUdc node, void* ptr, fct_cape_template__on_text on_text, fct_cape_template__on_file on_file, fct_cape_template__on_pipe on_pipe, fct_cape_template__on_tag on_tag, CapeErr err)
{
  int res;
  struct CapeTemplateCB_s cb;
  
  // a list to store all nodes in top down order
  CapeList node_stack = cape_list_new (NULL);

  // first entry
  cape_list_push_back (node_stack, node);
  
  // set the callbacks
  cb.ptr = ptr;
  cb.on_file = on_file;
  cb.on_pipe = on_pipe;
  cb.on_text = on_text;
  cb.on_tag = on_tag;
  
  res = cape_template_part_apply (self->root_part, node_stack, &cb, 0, err);

  // cleanup
  cape_list_del (&node_stack);
  
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
