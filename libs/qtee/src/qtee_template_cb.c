#include "qtee_template_cb.h"
#include "qtee_template.h"

//-----------------------------------------------------------------------------

struct QTeeTemplateCB_s
{
  void* ptr;
  
  // references to callback functions
  fct_cape_template__on_text on_text;
  fct_cape_template__on_file on_file;
  fct_cape_template__on_pipe on_pipe;
  fct_cape_template__on_tag on_tag;
  
};

//-----------------------------------------------------------------------------

QTeeTemplateCB qtee_template_cb_new (void* user_ptr, fct_cape_template__on_text on_text, fct_cape_template__on_file on_file, fct_cape_template__on_pipe on_pipe, fct_cape_template__on_tag on_tag)
{
    QTeeTemplateCB self = CAPE_NEW (struct QTeeTemplateCB_s);
  
    self->ptr = user_ptr;
    
    self->on_text = on_text;
    self->on_file = on_file;
    self->on_tag = on_tag;
    self->on_pipe = on_pipe;
  
    return self;
}

//-----------------------------------------------------------------------------

void qtee_template_cb_del (QTeeTemplateCB* p_self)
{
    if (*p_self)
    {
//        QTeeTemplateCB self = *p_self;
      
        CAPE_DEL (p_self, struct QTeeTemplateCB_s);
    }
}

//-----------------------------------------------------------------------------

void qtee_template_cb__value (QTeeTemplateCB self, const CapeString value)
{
    if (self->on_text)
    {
      self->on_text (self->ptr, value, cape_str_size (value));
    }
}

//-----------------------------------------------------------------------------

void qtee_template_cb__tag (QTeeTemplateCB self, const CapeString tag)
{
    // call the tag callback
    if (self->on_tag)
    {
      self->on_tag (self->ptr, tag);
    }
}

//-----------------------------------------------------------------------------

void qtee_template_cb__s (QTeeTemplateCB self, QTeeFormat format, const CapeString value)
{
    CapeString h = qtee_format_apply_s (format, value, self->on_pipe);
    if (h)
    {
        qtee_template_cb__value (self, h);
        
        cape_str_del (&h);
    }
}

//-----------------------------------------------------------------------------

void qtee_template_cb__n (QTeeTemplateCB self, QTeeFormat format, number_t value)
{
    CapeString h = qtee_format_apply_n (format, value, self->on_pipe);
    if (h)
    {
        qtee_template_cb__value (self, h);

        cape_str_del (&h);
    }
}

//-----------------------------------------------------------------------------

void qtee_template_cb__f (QTeeTemplateCB self, QTeeFormat format, double value)
{
    CapeString h = qtee_format_apply_f (format, value, self->on_pipe);
    if (h)
    {
        qtee_template_cb__value (self, h);

        cape_str_del (&h);
    }
}

//-----------------------------------------------------------------------------

void qtee_template_cb__d (QTeeTemplateCB self, QTeeFormat format, const CapeDatetime* value)
{
    CapeString h = qtee_format_apply_d (format, value, self->on_pipe);
    if (h)
    {
        qtee_template_cb__value (self, h);

        cape_str_del (&h);
    }
}

//-----------------------------------------------------------------------------

void qtee_template_cb__b (QTeeTemplateCB self, QTeeFormat format, int value)
{
    CapeString h = qtee_format_apply_b (format, value, self->on_pipe);
    if (h)
    {
        qtee_template_cb__value (self, h);

        cape_str_del (&h);
    }
}

//-----------------------------------------------------------------------------

void qtee_template_cb__m (QTeeTemplateCB self, QTeeFormat format, CapeStream m)
{
    // TODO: add stream also to the format

    if (self->on_text)
    {
        self->on_text (self->ptr, cape_stream_data (m), cape_stream_size (m));
    }
}

//-----------------------------------------------------------------------------

int qtee_template_cb__file (QTeeTemplateCB self, QTeeFormat format, CapeList node_stack, CapeErr err)
{
    int res;
    
    if (self->on_file == NULL)
    {
      res = CAPE_ERR_NONE;
    }
    else
    {
      CapeUdc found_item = qtee_format_item (format, node_stack);
      
      if (found_item)
      {
        CapeString filepath = qtee_format_apply_node (format, found_item, self->on_pipe);
        if (filepath)
        {
          number_t flags = CAPE_TEMPLATE_FLAG__NONE;
          
          if (qtee_format_has_encrypted (format))
          {
            flags |= CAPE_TEMPLATE_FLAG__ENCRYPTED;
          }
          
          res = self->on_file (self->ptr, filepath, flags, err);
          
          cape_str_del (&filepath);
        }
        else
        {
          res = CAPE_ERR_NONE;
        }
      }
    }
}
