#include "qtee_part.h"
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

struct QTeePart_s
{
    int type;
    
    CapeString text;
    
    CapeString eval;
    
    CapeString modn;
    
    CapeList parts;

    QTeePart parent;

    QTeeFormat format;
};

//-----------------------------------------------------------------------------

void cape_template_part_checkForEval (QTeePart self, const CapeString text)
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

          self->format = qtee_format_gen (self->text);
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

QTeePart qtee_part_new (int type, const CapeString raw_text, QTeePart parent)
{
  QTeePart self = CAPE_NEW (struct QTeePart_s);
  
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

void qtee_part_del (QTeePart* p_self)
{
    QTeePart self = *p_self;
  
  qtee_format_del (&(self->format));
  
  cape_str_del (&(self->text));
  cape_str_del (&(self->eval));
  cape_str_del (&(self->modn));

  if (self->parts)
  {
    cape_list_del (&(self->parts));
  }
  
  CAPE_DEL (p_self, struct QTeePart_s);
}

//-----------------------------------------------------------------------------

void qtee_part_set (QTeePart self, CapeString* p_text, CapeString* p_modn)
{
    cape_str_replace_mv (&(self->text), p_text);
    cape_str_replace_mv (&(self->modn), p_modn);
}

//-----------------------------------------------------------------------------

static void __STDCALL cape_template_create_parts_onDestroy (void* ptr)
{
  QTeePart h = ptr; qtee_part_del (&h);
}

//-----------------------------------------------------------------------------

int qtee_part_equal (QTeePart self, const CapeString text)
{
    if (self->format)
    {
        return qtee_format_has_name (self->format, text);
    }
    else
    {
        return cape_str_equal (self->text, text);
    }
}

//-----------------------------------------------------------------------------

QTeePart qtee_part_parent (QTeePart self)
{
  return self->parent;
}

//-----------------------------------------------------------------------------

void qtee_part_add (QTeePart self, QTeePart part)
{
    if (self->parts == NULL)
    {
        self->parts = cape_list_new (cape_template_create_parts_onDestroy);
    }
    
    // add a new part to the part list
    cape_list_push_back (self->parts, part);
    
    cape_log_fmt (CAPE_LL_TRACE, "QTEE", "compiler", "add part [%i]@[%p] text = '%s'", part->type, self->parts, part->text);
}

//-----------------------------------------------------------------------------

void qtee_part_clear (QTeePart self)
{
    if (self->parts == NULL)
    {
        self->parts = cape_list_new (cape_template_create_parts_onDestroy);
    }
    else
    {
        cape_list_clr (self->parts);
    }
}

//-----------------------------------------------------------------------------

int qtee_part_apply (QTeePart self, CapeList node_stack, QTeeTemplateCB cb, number_t pos, CapeErr err);

//-----------------------------------------------------------------------------

int cape_template_file_apply (QTeePart self, QTeePart part, CapeList node_stack, QTeeTemplateCB cb, CapeErr err)
{
  int res;
  const CapeString name = part->text;

    
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
  
  return ret;
}

//-----------------------------------------------------------------------------

int cape_template_mod_apply__math (QTeePart self, CapeList node_stack, QTeeTemplateCB cb, CapeErr err)
{
    qtee_template_cb__f (cb, self->format, cape_template_math (self->text, node_stack));
    
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

int cape_template_mod_apply__date (QTeePart self, CapeList node_stack, QTeeTemplateCB cb, CapeErr err)
{
    CapeDatetime* value = cape_template_date (self->text, node_stack);
    
    if (value)
    {
        qtee_template_cb__d (cb, self->format, value);
        
        cape_datetime_del (&value);
    }
    
    return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

int qtee_template_tag_apply (QTeePart self, CapeList node_stack, QTeeTemplateCB cb, number_t pos, CapeErr err)
{
    CapeUdc found_item = NULL;
  
    if (self->format)
    {
        // try to find the node_name UDC object in the node stack
        found_item = qtee_format_item (self->format, node_stack);
    }
    else
    {
        cape_log_msg (CAPE_LL_WARN, "QTEE", "template", "no format defined");
    }

    //cape_log_fmt (CAPE_LL_TRACE, "QTEE", "tag apply", "use format = '%s', found item = %p", self->text, found_item);

    // call the tag callback
    qtee_template_cb__tag (cb, self->text);
  
  if (found_item)
  {
    switch (cape_udc_type (found_item))
    {
      case CAPE_UDC_NODE:
      {
        // add a next level
        cape_list_push_back (node_stack, found_item);
        
        int res = qtee_part_apply (self, node_stack, cb, pos, err);
        
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
          
          int res = qtee_part_apply (self, node_stack, cb, cursor_item->position, err);
          
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
              qtee_part_apply (self, node_stack, cb, 0, err);
          }
        }
        else
        {
            qtee_template_cb__s (cb, self->format, cape_udc_s (found_item, NULL));
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
              qtee_part_apply (self, node_stack, cb, 0, err);
          }
        }
        else
        {
            qtee_template_cb__n (cb, self->format, cape_udc_n (found_item, 0));            
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
              qtee_part_apply (self, node_stack, cb, 0, err);
          }
        }
        else
        {
            qtee_template_cb__f (cb, self->format, cape_udc_f (found_item, .0));
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
              qtee_part_apply (self, node_stack, cb, 0, err);
          }
          else if ((h == FALSE) && cape_str_equal (self->eval, "FALSE"))
          {
              qtee_part_apply (self, node_stack, cb, 0, err);
          }
        }
        else
        {
            qtee_template_cb__b (cb, self->format, cape_udc_b (found_item, FALSE));
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
            qtee_template_cb__d (cb, self->format, cape_udc_d (found_item, NULL));
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
            qtee_template_cb__m (cb, self->format, cape_udc_m (found_item));
        }
      }
    }
  }
  
  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

int qtee_part_apply (QTeePart self, CapeList node_stack, QTeeTemplateCB cb, number_t pos, CapeErr err)
{
  if (self->parts)
  {
    CapeListCursor* cursor = cape_list_cursor_new (self->parts, CAPE_DIRECTION_FORW);
    
    while (cape_list_cursor_next (cursor))
    {
      QTeePart part = cape_list_node_data (cursor->node);
      
      switch (part->type)
      {
        case PART_TYPE_TEXT:
        {
            qtee_template_cb__s (cb, self->format, part->text);
                        
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
            // apply special values
            if (cape_str_equal (part->text, "INDEX_1"))
            {
                CapeString h = cape_str_n (pos + 1);
              
                qtee_template_cb__value (cb, h);
                
                cape_str_del (&h);
            }
            else
            {
                qtee_template_tag_apply (part, node_stack, cb, cursor->position, err);
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
