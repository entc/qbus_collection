#include "cape_template.h"

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
#define PART_TYPE_CR     5
#define PART_TYPE_MOD    6

//-----------------------------------------------------------------------------

#define FORMAT_TYPE_NONE        0
#define FORMAT_TYPE_DATE_LCL    1
#define FORMAT_TYPE_ENCRYPTED   2
#define FORMAT_TYPE_DECIMAL     3
#define FORMAT_TYPE_PIPE        4
#define FORMAT_TYPE_SUBSTR      5
#define FORMAT_TYPE_DATE_UTC    6

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
  
  int format_type;
};

//-----------------------------------------------------------------------------

void cape_template_part_checkForFormat (CapeTemplatePart self, const CapeString format)
{
  CapeString f1 = NULL;
  CapeString f2 = NULL;
  
  if (cape_tokenizer_split (format, ':', &f1, &f2))
  {
    CapeString h = cape_str_trim_utf8 (f1);
    
    if (cape_str_equal (h, "date"))
    {
      self->format_type = FORMAT_TYPE_DATE_LCL;
      self->eval = cape_str_trim_utf8 (f2);
    }
    else if (cape_str_equal (h, "date_utc"))
    {
      self->format_type = FORMAT_TYPE_DATE_UTC;
      self->eval = cape_str_trim_utf8 (f2);
    }
    else if (cape_str_equal (h, "substr"))
    {
      self->format_type = FORMAT_TYPE_SUBSTR;
      self->eval = cape_str_trim_utf8 (f2);
    }
    else if (cape_str_equal (h, "decimal"))
    {
      self->format_type = FORMAT_TYPE_DECIMAL;
      self->eval = cape_str_trim_utf8 (f2);
    }
    else if (cape_str_begins (h, "pipe_"))
    {
      self->format_type = FORMAT_TYPE_PIPE;
      self->eval = cape_str_trim_utf8 (f2);
      self->modn = cape_str_mv (&h);
    }
    
    cape_str_del (&h);
  }
  
  cape_str_del (&f1);
  cape_str_del (&f2);
}

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
      else if (cape_tokenizer_split (text, '|',  &s1, &s2))
      {
        cape_template_part_checkForFormat (self, s2);
        self->text = cape_str_trim_utf8 (s1);
      }
      else
      {
        self->text = cape_str_trim_utf8 (text);
      }

      cape_str_del (&s1);
      cape_str_del (&s2);
      
      break;
    }
    case PART_TYPE_FILE:
    {
      CapeString s1 = NULL;
      CapeString s2 = NULL;

      if (cape_tokenizer_split (text, '|',  &s1, &s2))
      {
        self->text = cape_str_trim_utf8 (s1);
        
        if (cape_str_equal (s2, "encrypted"))
        {
          self->format_type = FORMAT_TYPE_ENCRYPTED;
        }
      }
      else
      {
        self->text = cape_str_trim_utf8 (text);
      }
      
      cape_str_del (&s1);
      cape_str_del (&s2);

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
  
  // set no format
  self->format_type = FORMAT_TYPE_NONE;
  
  self->parts = NULL;
  self->parent = parent;
  
  // analyse the text value for later evaluation
  cape_template_part_checkForEval (self, raw_text);
  
  return self;
}

//-----------------------------------------------------------------------------

void cape_template_part_del (CapeTemplatePart* p_self)
{
  CapeTemplatePart self = *p_self;
  
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

int cape__evaluate_expression__compare (const CapeString left, const CapeString right)
{
  int ret;
  
  CapeString l_trimmed = cape_str_trim_utf8 (left);
  CapeString r_trimmed = cape_str_trim_utf8 (right);

  if (cape_str_equal (l_trimmed, "EMPTY"))
  {
    ret = cape_str_empty (r_trimmed);
  }
  else if (cape_str_equal (r_trimmed, "EMPTY"))
  {
    ret = cape_str_empty (l_trimmed);
  }
  else if (cape_str_equal (l_trimmed, "VALID"))
  {
    ret = cape_str_not_empty (r_trimmed);
  }
  else if (cape_str_equal (r_trimmed, "VALID"))
  {
    ret = cape_str_not_empty (l_trimmed);
  }
  else
  {
    ret = cape_str_equal (l_trimmed, r_trimmed);
  }
  
  cape_log_fmt (CAPE_LL_TRACE, "CAPE", "template cmp", "expression: '%s' = '%s' -> %i", l_trimmed, r_trimmed, ret);
    
  cape_str_del (&l_trimmed);
  cape_str_del (&r_trimmed);
  
  return ret;
}

//-----------------------------------------------------------------------------

int cape__evaluate_expression__greater_than (const CapeString left, const CapeString right)
{
  int ret;
  
  CapeString l_trimmed = cape_str_trim_utf8 (left);
  CapeString r_trimmed = cape_str_trim_utf8 (right);

  char* l_res = NULL;
  char* r_res = NULL;

  // try to convert into double
  double l_d = strtod (l_trimmed, &l_res);
  double r_d = strtod (r_trimmed, &r_res);

  if (l_res && r_res)
  {
    // do a mathematical comparison
    ret = l_d > r_d;
    
    cape_log_fmt (CAPE_LL_TRACE, "CAPE", "template cmp", "expression [double]: %f > %f -> %i,   [orignal]: '%s' > '%s'", l_d, r_d, ret, l_trimmed, r_trimmed);
  }
  else
  {
    // do a textual comparision
    ret = cape_str_compare_c (l_trimmed, r_trimmed) > 0;
  
    cape_log_fmt (CAPE_LL_TRACE, "CAPE", "template cmp", "expression [text]: '%s' > '%s' -> %i", l_trimmed, r_trimmed, ret);
  }
      
  cape_str_del (&l_trimmed);
  cape_str_del (&r_trimmed);

  return ret;
}

//-----------------------------------------------------------------------------

int cape__evaluate_expression__smaller_than (const CapeString left, const CapeString right)
{
  int ret;
  
  CapeString l_trimmed = cape_str_trim_utf8 (left);
  CapeString r_trimmed = cape_str_trim_utf8 (right);

  char* l_res = NULL;
  char* r_res = NULL;

  // try to convert into double
  double l_d = strtod (l_trimmed, &l_res);
  double r_d = strtod (r_trimmed, &r_res);

  if (l_res && r_res)
  {
    // do a mathematical comparison
    ret = l_d < r_d;
    
    cape_log_fmt (CAPE_LL_TRACE, "CAPE", "template cmp", "expression [double]: %f < %f -> %i,   [orignal]: '%s' < '%s'", l_d, r_d, ret, l_trimmed, r_trimmed);
  }
  else
  {
    // do a textual comparision
    ret = cape_str_compare_c (l_trimmed, r_trimmed) < 0;
  
    cape_log_fmt (CAPE_LL_TRACE, "CAPE", "template cmp", "expression [text]: '%s' < '%s' -> %i", l_trimmed, r_trimmed, ret);
  }
      
  cape_str_del (&l_trimmed);
  cape_str_del (&r_trimmed);

  return ret;
}

//-----------------------------------------------------------------------------

CapeList cape__evaluate_expression__list (const CapeString source)
{
  CapeList ret = NULL;
  
  number_t pos;
  number_t len;

  if (cape_str_next (source, '[', &pos) && cape_str_next (source, ']', &len))
  {
    ret = cape_tokenizer_buf (source + pos + 1, len - pos - 1, ',');
  }
  
  return ret;
}

//-----------------------------------------------------------------------------

int cape__evaluate_expression__in (const CapeString left, const CapeString right)
{
  int ret = FALSE;
  
  CapeString l_trimmed = cape_str_trim_utf8 (left);
  CapeString r_trimmed = cape_str_trim_utf8 (right);

  CapeList right_list = cape__evaluate_expression__list (r_trimmed);
  
  if (right_list)
  {
    CapeListCursor* cursor = cape_list_cursor_create (right_list, CAPE_DIRECTION_FORW);
    
    while (cape_list_cursor_next (cursor))
    {
      CapeString h = cape_str_trim_utf8 (cape_list_node_data (cursor->node));
      
      ret = ret || cape_str_equal (l_trimmed, h);

      cape_log_fmt (CAPE_LL_TRACE, "CAPE", "template cmp", "expression [left in right]: %s in %s -> %i", l_trimmed, h, ret);

      cape_str_del (&h);
    }
    
    cape_list_cursor_destroy (&cursor);
  }
  
  cape_str_del (&l_trimmed);
  cape_str_del (&r_trimmed);

  cape_list_del (&right_list);
  return ret;
}

//-----------------------------------------------------------------------------

void cape_template_part_eval_datetime__lcl (CapeTemplatePart self, CapeDatetime* dt, CapeTemplateCB cb)
{
  // set the original as UTC
  dt->is_utc = TRUE;
  
  // convert into local time
  cape_datetime_to_local (dt);
  
  // apply format
  {
    CapeString h2 = cape_datetime_s__fmt_lcl (dt, self->eval);
    
    if (cb->on_text)
    {
      cb->on_text (cb->ptr, h2);
    }
    
    cape_str_del (&h2);
  }
}

//-----------------------------------------------------------------------------

void cape_template_part_eval_datetime__utc (CapeTemplatePart self, CapeDatetime* dt, CapeTemplateCB cb)
{
  CapeString h = cape_datetime_s__fmt_utc (dt, self->eval);
  
  if (cb->on_text)
  {
    cb->on_text (cb->ptr, h);
  }
  
  cape_str_del (&h);
}

//-----------------------------------------------------------------------------

int cape_template_part_eval_datetime_item (CapeTemplatePart self, CapeUdc item, CapeTemplateCB cb, CapeErr err)
{
  const CapeDatetime* dt = cape_udc_d (item, NULL);
  if (dt)
  {
    //cape_log_fmt (CAPE_LL_TRACE, "CAPE", "eval datetime", "evaluate %s = []", cape_udc_name (item));
    
    switch (self->format_type)
    {
      case FORMAT_TYPE_NONE:
      {
        CapeString val = cape_datetime_s__std (dt);
        
        if (cb->on_text)
        {
          cb->on_text (cb->ptr, val);
        }

        cape_str_del (&val);
        
        break;
      }
      case FORMAT_TYPE_DATE_LCL:
      {
        CapeDatetime* h1 = cape_datetime_cp (dt);

        cape_template_part_eval_datetime__lcl (self, h1, cb);
        
        cape_datetime_del (&h1);
        
        break;
      }
      case FORMAT_TYPE_DATE_UTC:
      {
        CapeDatetime* h1 = cape_datetime_cp (dt);
        
        cape_template_part_eval_datetime__utc (self, h1, cb);
        
        cape_datetime_del (&h1);

        break;
      }
    }
  }
  
  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

int cape_template_part_eval_substr (CapeTemplatePart self, const CapeString value, CapeTemplateCB cb, CapeErr err)
{
  CapeList tokens = cape_tokenizer_buf (self->eval, cape_str_size (self->eval), '%');
  
  if (cape_list_size (tokens) == 2)
  {
    number_t pos = 0;
    number_t len = 0;
    
    CapeListCursor* cursor = cape_list_cursor_create (tokens, CAPE_DIRECTION_FORW);

    cape_list_cursor_next (cursor);
    pos = cape_str_to_n (cape_list_node_data (cursor->node));

    cape_list_cursor_next (cursor);
    len = cape_str_to_n (cape_list_node_data (cursor->node));

    CapeString val = cape_str_sub (value + pos, len);
    
    if (cb->on_text)
    {
      cb->on_text (cb->ptr, val);
    }

    cape_str_del (&val);
    
    cape_list_cursor_destroy (&cursor);
  }
  
  cape_list_del (&tokens);
  
  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

int cape_template_part_eval_decimal (CapeTemplatePart self, double value, CapeTemplateCB cb, CapeErr err)
{
  double fraction = 1.0;
  CapeString devider = NULL;
  number_t decimal = 3;
  
  CapeUdc options = cape_tokenizer_options (self->eval);
  
  if (options)
  {
    fraction = cape_udc_get_f (options, "fraction", fraction);
    devider = cape_str_cp (cape_udc_get_s (options, "delimiter", "."));
    decimal = cape_udc_get_n (options, "digits", decimal);
    
    cape_udc_del (&options);
  }
  else
  {
    CapeList tokens = cape_tokenizer_buf (self->eval, cape_str_size (self->eval), '%');
    
    if (cape_list_size (tokens) == 3)
    {
      CapeListCursor* cursor = cape_list_cursor_create (tokens, CAPE_DIRECTION_FORW);
      
      cape_list_cursor_next (cursor);
      
      fraction = cape_str_to_f (cape_list_node_data (cursor->node));
      
      cape_list_cursor_next (cursor);
      
      devider = cape_str_cp (cape_list_node_data (cursor->node));
      
      cape_list_cursor_next (cursor);
      
      decimal = strtol (cape_list_node_data (cursor->node), NULL, 10);
      
      cape_list_cursor_destroy (&cursor);
    }
    
    cape_list_del (&tokens);
  }
  
  // calculate value
  {
    double v = (double)value / fraction;
    
    number_t dh = pow (10, decimal);
    
    // fix an issue with rounding
    double v2 = v * dh + 0.5000000001;
    
    number_t dj = floor(v2);

    double dk = (double)dj / dh;
    
    CapeString fmt = cape_str_fmt ("%%.%if", decimal);
    CapeString val = cape_str_fmt (fmt, dk);
    
    cape_str_replace (&val, ".", devider);
    
    if (cb->on_text)
    {
      cb->on_text (cb->ptr, val);
    }
    
    cape_str_del (&val);
    cape_str_del (&fmt);
  }

  cape_str_del (&devider);
  
  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

int cape_template_part_eval_str (CapeTemplatePart self, CapeList node_stack, CapeUdc item, CapeTemplateCB cb, CapeErr err)
{
  const CapeString text = cape_udc_s (item, NULL);
  if (text)
  {
    //cape_log_fmt (CAPE_LL_TRACE, "CAPE", "eval str", "evaluate %s = '%s'", cape_udc_name (item), text);
    
    switch (self->format_type)
    {
      case FORMAT_TYPE_NONE:
      {
        if (self->eval)
        {
          if (cape__evaluate_expression__compare (self->eval, text))
          {
            return cape_template_part_apply (self, node_stack, cb, 0, err);
          }
        }
        else
        {
          if (cb->on_text)
          {
            cb->on_text (cb->ptr, text);
          }
          
          return cape_template_part_apply (self, node_stack, cb, 0, err);
        }
        
        break;
      }
      case FORMAT_TYPE_SUBSTR:
      {
        return cape_template_part_eval_substr (self, text, cb, err);
      }
      case FORMAT_TYPE_DATE_LCL:
      {
        CapeDatetime dt;
        
        // convert text into dateformat
        if (cape_datetime__str_msec (&dt, text) || cape_datetime__str (&dt, text) || cape_datetime__std_msec (&dt, text) || cape_datetime__date_de (&dt, text))
        {
          // set the original as UTC
          dt.is_utc = TRUE;
          
          // convert into local time
          cape_datetime_to_local (&dt);
          
          // apply format
          {
            CapeString h = cape_datetime_s__fmt_lcl (&dt, self->eval);
            
            {
              CapeString h2 = cape_datetime_s__std (&dt);
              
              cape_log_fmt (CAPE_LL_TRACE, "CAPE", "template eval", "applied date format = '%s' >> '%s' -> '%s'", self->eval, h2, h);

              cape_str_del (&h2);
            }
            
            if (cb->on_text)
            {
              cb->on_text (cb->ptr, h);
            }
            
            cape_str_del (&h);
          }
        }
        else
        {
          cape_log_fmt (CAPE_LL_ERROR, "CAPE", "template eval", "can't evaluate '%s' as date", text);
        }
        
        return cape_template_part_apply (self, node_stack, cb, 0, err);
      }
      case FORMAT_TYPE_PIPE:
      {
        if (cb->on_pipe)
        {
          CapeString h = cb->on_pipe (self->modn, self->eval, text);
          if (h)
          {
            if (cb->on_text)
            {
              cb->on_text (cb->ptr, h);
            }
            
            cape_str_del (&h);
          }
        }
        else
        {
          if (cb->on_text)
          {
            cb->on_text (cb->ptr, text);
          }
        }
        
        return cape_template_part_apply (self, node_stack, cb, 0, err);
      }
      case FORMAT_TYPE_DECIMAL:
      {
        // try to convert text into a number
        double value = cape_str_to_f (text);

        return cape_template_part_eval_decimal (self, value, cb, err);
      }
    }
  }
  
  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

int cape_template_part_eval_number (CapeTemplatePart self, CapeList node_stack, CapeUdc item, CapeTemplateCB cb, CapeErr err)
{
  //cape_log_fmt (CAPE_LL_TRACE, "CAPE", "eval number", "evaluate %s = [%i]", cape_udc_name (item), cape_udc_n (item, 0));

  switch (self->format_type)
  {
    case FORMAT_TYPE_NONE:
    {
      if (self->eval)
      {
        number_t h = strtol (self->eval, NULL, 10);
        
        if (h == cape_udc_n (item, 0))
        {
          return cape_template_part_apply (self, node_stack, cb, 0, err);
        }
      }
      else
      {
        if (cb->on_text)
        {
          CapeString h = cape_str_fmt ("%li", cape_udc_n (item, 0));
          
          cb->on_text (cb->ptr, h);
          
          cape_str_del (&h);
          
          return cape_template_part_apply (self, node_stack, cb, 0, err);
        }
      }
      
      break;
    }
    case FORMAT_TYPE_DECIMAL:
    {
      return cape_template_part_eval_decimal (self, cape_udc_n (item, 0), cb, err);
    }
  }
  
  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

int cape_template_part_eval_double (CapeTemplatePart self, CapeList node_stack, CapeUdc item, CapeTemplateCB cb, CapeErr err)
{
  //cape_log_fmt (CAPE_LL_TRACE, "CAPE", "eval double", "evaluate %s = []", cape_udc_name (item));

  if (self->eval)
  {
    double h = cape_str_to_f (self->eval);
    
    if (h == cape_udc_f (item, .0))
    {
      return cape_template_part_apply (self, node_stack, cb, 0, err);
    }
  }
  else
  {
    if (cb->on_text)
    {
      CapeString h = cape_str_f (cape_udc_f (item, .0));
      
      cb->on_text (cb->ptr, h);
      
      cape_str_del (&h);
      
      return cape_template_part_apply (self, node_stack, cb, 0, err);
    }
  }
  
  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

int cape_template_part_eval_bool (CapeTemplatePart self, CapeList node_stack, CapeUdc item, CapeTemplateCB cb, CapeErr err)
{
  //cape_log_fmt (CAPE_LL_TRACE, "CAPE", "eval bool", "evaluate %s = []", cape_udc_name (item));

  if (self->eval)
  {
    int h = cape_udc_b (item, FALSE);
    
    if ((h == TRUE) && cape_str_equal (self->eval, "TRUE"))
    {
      return cape_template_part_apply (self, node_stack, cb, 0, err);
    }
    else if ((h == FALSE) && cape_str_equal (self->eval, "FALSE"))
    {
      return cape_template_part_apply (self, node_stack, cb, 0, err);
    }
  }
  else
  {
    if (cb->on_text)
    {
      const CapeString h = cape_udc_b (item, FALSE) ? "TRUE" : "FALSE";
      
      cb->on_text (cb->ptr, h);

      return cape_template_part_apply (self, node_stack, cb, 0, err);
    }
  }
  
  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

CapeUdc cape_template__seek_item__from_stack (CapeList node_stack, const CapeString name)
{
  CapeUdc ret = NULL;

  // local objects
  CapeListCursor* cursor = cape_list_cursor_create (node_stack, CAPE_DIRECTION_PREV);
  
  while (cape_list_cursor_prev (cursor))
  {
    // get the node from the stack
    CapeUdc node = cape_list_node_data (cursor->node);
    
    // try to find the item in the node with the 'name'
    CapeUdc item = cape_udc_get (node, name);
    
    if (item)
    {
      ret = item;
      goto exit_and_cleanup;
    }
  }
  
exit_and_cleanup:
  
  cape_list_cursor_destroy (&cursor);
  return ret;
}

//-----------------------------------------------------------------------------

CapeUdc cape_template__seek_item (CapeList node_stack, const CapeString name)
{
  CapeUdc ret = NULL;
  CapeUdc sub = NULL;
  
  // split the name into its parts
  CapeList name_parts = cape_tokenizer_buf (name, cape_str_size (name), '.');
  CapeListCursor* name_parts_cursor = cape_list_cursor_create (name_parts, CAPE_DIRECTION_FORW);

  if (cape_list_cursor_next (name_parts_cursor))
  {
    sub = cape_template__seek_item__from_stack (node_stack, cape_list_node_data (name_parts_cursor->node));
  }

  while (sub && cape_list_cursor_next (name_parts_cursor))
  {
    sub = cape_udc_get (sub, cape_list_node_data (name_parts_cursor->node));
  }
  
  ret = sub;
  
  cape_list_cursor_destroy (&name_parts_cursor);
  cape_list_del (&name_parts);

  return ret;
}

//-----------------------------------------------------------------------------

int cape_template_file_apply (CapeTemplatePart self, CapeTemplatePart part, CapeList node_stack, CapeTemplateCB cb, CapeErr err)
{
  int res;
  const CapeString name = part->text;

  if (cb->on_file == NULL)
  {
    res = CAPE_ERR_NONE;
    goto exit_and_cleanup;
  }
  
  CapeUdc item = cape_template__seek_item (node_stack, name);
  if (item)
  {
    switch (cape_udc_type (item))
    {
      case CAPE_UDC_STRING:
      {
        const CapeString text = cape_udc_s (item, NULL);
        if (text)
        {
          number_t flags = CAPE_TEMPLATE_FLAG__NONE;
          
          if (part->format_type == FORMAT_TYPE_ENCRYPTED)
          {
            flags |= CAPE_TEMPLATE_FLAG__ENCRYPTED;
          }
          
          res = cb->on_file (cb->ptr, text, flags, err);
          goto exit_and_cleanup;
        }
        
        break;
      }
    }
  }

  res = cb->on_file (cb->ptr, part->text, CAPE_TEMPLATE_FLAG__NONE, err);

exit_and_cleanup:

  return res;
}

//-----------------------------------------------------------------------------

double cape_template_math (const CapeString formular, CapeList node_stack)
{
  double ret = .0;
  
  CapeString le = NULL;
  CapeString re = NULL;
  
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
  else
  {
    CapeUdc item = cape_template__seek_item (node_stack, formular);
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
          }

          break;
        }
        case CAPE_UDC_NUMBER:
        {
          ret = cape_udc_n (item, 0);
          break;
        }
        case CAPE_UDC_FLOAT:
        {
          ret = cape_udc_f (item, .0);
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

int cape_template_mod_apply__math (CapeTemplatePart self, CapeList node_stack, CapeTemplateCB cb, CapeErr err)
{
  double value = cape_template_math (self->text, node_stack);
  
  switch (self->format_type)
  {
    case FORMAT_TYPE_NONE:
    {

      break;
    }
    case FORMAT_TYPE_DECIMAL:
    {
      return cape_template_part_eval_decimal (self, value, cb, err);
    }
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

    CapeUdc item = cape_template__seek_item (node_stack, lh);
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
    switch (self->format_type)
    {
      case FORMAT_TYPE_NONE:
      {
        
        break;
      }
      case FORMAT_TYPE_DATE_LCL:
      {
        cape_template_part_eval_datetime__lcl (self, value, cb);
        break;
      }
      case FORMAT_TYPE_DATE_UTC:
      {
        cape_template_part_eval_datetime__utc (self, value, cb);
        break;
      }
    }

    cape_datetime_del (&value);
  }
  
  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

int cape_template_part_apply (CapeTemplatePart self, CapeList node_stack, CapeTemplateCB cb, number_t pos, CapeErr err)
{
  if (self->parts)
  {
    CapeListCursor* cursor = cape_list_cursor_create (self->parts, CAPE_DIRECTION_FORW);
    
    while (cape_list_cursor_next (cursor))
    {
      CapeTemplatePart part = cape_list_node_data (cursor->node);
      
      switch (part->type)
      {
        case PART_TYPE_TEXT:
        case PART_TYPE_CR:
        {
          if (cb->on_text)
          {
            cb->on_text (cb->ptr, part->text);
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
          const CapeString name = part->text;
          
          if (cape_str_equal (name, "INDEX_1"))
          {
            CapeString h = cape_str_n (pos + 1);
            
            if (cb->on_text)
            {
              cb->on_text (cb->ptr, h);
            }
            
            cape_str_del (&h);
          }
          else
          {
            CapeUdc item = cape_template__seek_item (node_stack, name);
            if (item)
            {
              if (cb->on_tag)
              {
                cb->on_tag (cb->ptr, name);
              }
              
              switch (cape_udc_type (item))
              {
                case CAPE_UDC_LIST:
                {
                  CapeUdcCursor* cursor_item = cape_udc_cursor_new (item, CAPE_DIRECTION_FORW);
                  
                  while (cape_udc_cursor_next (cursor_item))
                  {
                    // add a next level
                    cape_list_push_back (node_stack, cursor_item->item);
                    
                    int res = cape_template_part_apply (part, node_stack, cb, cursor_item->position, err);
                    
                    cape_list_pop_back (node_stack);
                    
                    if (res)
                    {
                      return res;
                    }
                  }
                  
                  cape_udc_cursor_del (&cursor_item);
                  
                  break;
                }
                case CAPE_UDC_NODE:
                {
                  // add a next level
                  cape_list_push_back (node_stack, item);

                  int res = cape_template_part_apply (part, node_stack, cb, cursor->position, err);

                  cape_list_pop_back (node_stack);

                  if (res)
                  {
                    return res;
                  }
                  
                  break;
                }
                case CAPE_UDC_STRING:
                {
                  int res = cape_template_part_eval_str (part, node_stack, item, cb, err);
                  if (res)
                  {
                    return res;
                  }
                  
                  break;
                }
                case CAPE_UDC_NUMBER:
                {
                  int res = cape_template_part_eval_number (part, node_stack, item, cb, err);
                  if (res)
                  {
                    return res;
                  }
                  
                  break;
                }
                case CAPE_UDC_FLOAT:
                {
                  int res = cape_template_part_eval_double (part, node_stack, item, cb, err);
                  if (res)
                  {
                    return res;
                  }
                  
                  break;
                }
                case CAPE_UDC_BOOL:
                {
                  int res = cape_template_part_eval_bool (part, node_stack, item, cb, err);
                  if (res)
                  {
                    return res;
                  }

                  break;
                }
                case CAPE_UDC_DATETIME:
                {
                  int res = cape_template_part_eval_datetime_item (part, item, cb, err);
                  if (res)
                  {
                    return res;
                  }
                  
                  break;
                }
                default:
                {
                  int res = cape_template_part_eval_str (part, node_stack, item, cb, err);
                  if (res)
                  {
                    return res;
                  }
                  
                  break;
                }
              }
            }
            else
            {
              cape_log_fmt (CAPE_LL_WARN, "CAPE", "template", "can't find node '%s'", name);
            }
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
      const CapeString text = cape_stream_get (self->sb);
      
      cape_template_part_add (self->part, cape_template_part_new (type, text, NULL));
      cape_stream_clr (self->sb);
      
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
    case PART_TYPE_CR:
    {
      // always clear
      cape_stream_clr (self->sb);
      
      break;
    }
  }
  
  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

int cape_template_compiler_parse (EcTemplateCompiler self, const char* buffer, int size, CapeErr err)
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
          cape_stream_append_c (self->sb, *c);
          
          res = cape_template_compiler_part (self, PART_TYPE_CR, err);
          if (res)
          {
            return res;
          }
          
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

int cape_eval_f (const CapeString s, CapeUdc node, double* p_ret, fct_cape_template__on_pipe on_pipe, CapeErr err)
{
  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

int __STDCALL cape_eval__on_text (void* ptr, const char* text)
{
  cape_stream_append_str (ptr, text);
  
  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

int cape__evaluate_expression__single (const CapeString expression)
{
  int ret = FALSE;
  
  // local objects
  CapeString left = NULL;
  CapeString right = NULL;

  //printf ("EXPR SINGLE: '%s'\n", expression);

  // find the '=' in the expression
  if (cape_tokenizer_split (expression, '=', &left, &right))
  {
    ret = cape__evaluate_expression__compare (left, right);
  }
  else if (cape_tokenizer_split (expression, '>', &left, &right))
  {
    ret = cape__evaluate_expression__greater_than (left, right);
  }
  else if (cape_tokenizer_split (expression, '<', &left, &right))
  {
    ret = cape__evaluate_expression__smaller_than (left, right);
  }
  else if (cape_tokenizer_split (expression, 'I', &left, &right))
  {
    ret = cape__evaluate_expression__in (left, right);
  }
  else
  {
    ret = cape_str_not_empty (expression);
  }
  
  cape_str_del (&left);
  cape_str_del (&right);
  
  return ret;
}

//-----------------------------------------------------------------------------

int cape__evaluate_expression_not (const CapeString expression)
{
  if (cape_str_begins (expression, "NOT "))
  {
    return !cape__evaluate_expression__single (expression + 4);
  }
  else
  {
    return cape__evaluate_expression__single (expression);
  }
}

//-----------------------------------------------------------------------------

int cape__evaluate_expression_or (const CapeString expression)
{
  int ret = FALSE;
  
  // local objects
  CapeListCursor* cursor = NULL;
  CapeList logical_parts = cape_tokenizer_str (expression, " OR ");
  
  //printf ("EXPR OR: '%s'\n", expression);
  
  if (cape_list_size (logical_parts))
  {
    cursor = cape_list_cursor_create (logical_parts, CAPE_DIRECTION_FORW);
    while (cape_list_cursor_next (cursor))
    {
      ret = cape__evaluate_expression_not (cape_list_node_data (cursor->node));
      if (ret == TRUE)
      {
        goto exit_and_cleanup;
      }
    }
  }
  else
  {
    ret = cape__evaluate_expression_not (expression);
  }
  
exit_and_cleanup:
  
  cape_list_cursor_destroy (&cursor);
  cape_list_del (&logical_parts);
  
  return ret;
}

//-----------------------------------------------------------------------------

int cape__evaluate_expression_and (const CapeString expression)
{
  int ret = TRUE;
  
  // local objects
  CapeListCursor* cursor = NULL;
  CapeList logical_parts = cape_tokenizer_str (expression, " AND ");

  //printf ("EXPR AND: '%s'\n", expression);

  if (cape_list_size (logical_parts))
  {
    cursor = cape_list_cursor_create (logical_parts, CAPE_DIRECTION_FORW);
    while (cape_list_cursor_next (cursor))
    {
      ret = cape__evaluate_expression_or (cape_list_node_data (cursor->node));
      if (ret == FALSE)
      {
        goto exit_and_cleanup;
      }
    }
  }
  else
  {
    ret = cape__evaluate_expression_or (expression);
  }
  
exit_and_cleanup:
  
  cape_list_cursor_destroy (&cursor);
  cape_list_del (&logical_parts);
  
  return ret;
}

//-----------------------------------------------------------------------------

int cape_eval_b (const CapeString s, CapeUdc node, int* p_ret, fct_cape_template__on_pipe on_pipe, CapeErr err)
{
  int res;
  
  // local objects
  CapeTemplate tmpl = cape_template_new ();
  CapeStream stream = cape_stream_new ();
  
  res = cape_template_compile_str (tmpl, s, err);
  if (res)
  {
    goto exit_and_cleanup;
  }

  res = cape_template_apply (tmpl, node, stream, cape_eval__on_text, NULL, on_pipe, NULL, err);
  if (res)
  {
    goto exit_and_cleanup;
  }

  *p_ret = cape__evaluate_expression_and (cape_stream_get (stream));

  res = CAPE_ERR_NONE;
  
exit_and_cleanup:

  cape_template_del (&tmpl);
  cape_stream_del (&stream);

  return res;
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
