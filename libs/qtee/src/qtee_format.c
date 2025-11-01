#include "qtee_format.h"
#include "qtee_gen.h"

// cape inlcudes
#include <fmt/cape_tokenizer.h>
#include <sys/cape_log.h>

//-----------------------------------------------------------------------------

#define FORMAT_TYPE_NONE        0
#define FORMAT_TYPE_DATE_LCL    1
#define FORMAT_TYPE_ENCRYPTED   2
#define FORMAT_TYPE_DECIMAL     3
#define FORMAT_TYPE_PIPE        4
#define FORMAT_TYPE_SUBSTR      5
#define FORMAT_TYPE_DATE_UTC    6
#define FORMAT_TYPE_LPAD        7

//-----------------------------------------------------------------------------

struct QTeeFormatItem_s
{
    number_t format_type;
    CapeString format_text;
    CapeString pipe;
    
}; typedef struct QTeeFormatItem_s* QTeeFormatItem;

//-----------------------------------------------------------------------------

QTeeFormatItem qtee_format_item_new (const CapeString format_part)
{
    QTeeFormatItem self = CAPE_NEW (struct QTeeFormatItem_s);
    
    // local objects
    CapeString f1 = NULL;
    CapeString f2 = NULL;
    
    self->pipe = NULL;
    
    if (cape_tokenizer_split (format_part, ':', &f1, &f2))
    {
        CapeString h = cape_str_trim_utf8 (f1);
        
        if (cape_str_equal (h, "date"))
        {
            self->format_type = FORMAT_TYPE_DATE_LCL;
            self->format_text = cape_str_trim_utf8 (f2);
        }
        else if (cape_str_equal (h, "date_utc"))
        {
            self->format_type = FORMAT_TYPE_DATE_UTC;
            self->format_text = cape_str_trim_utf8 (f2);
        }
        else if (cape_str_equal (h, "substr"))
        {
            self->format_type = FORMAT_TYPE_SUBSTR;
            self->format_text = cape_str_trim_utf8 (f2);
        }
        else if (cape_str_equal (h, "decimal"))
        {
            self->format_type = FORMAT_TYPE_DECIMAL;
            self->format_text = cape_str_trim_utf8 (f2);
        }
        else if (cape_str_equal (h, "lpad"))
        {
            self->format_type = FORMAT_TYPE_LPAD;
            self->format_text = cape_str_trim_utf8 (f2);
        }
        else if (cape_str_begins (h, "pipe_"))
        {
            self->format_type = FORMAT_TYPE_PIPE;
            self->format_text = cape_str_trim_utf8 (f2);
            self->pipe = cape_str_mv (&h);
        }
        
        cape_str_del (&h);
    }
    else
    {
        self->format_type = FORMAT_TYPE_NONE;
        self->format_text = NULL;
    }
    
    // cleanup
    cape_str_del (&f1);
    cape_str_del (&f2);
    
    return self;
}

//-----------------------------------------------------------------------------

void qtee_format_item_del (QTeeFormatItem* p_self)
{
    if (*p_self)
    {
        QTeeFormatItem self = *p_self;
        
        cape_str_del (&(self->pipe));
        cape_str_del (&(self->format_text));
        
        CAPE_DEL (p_self, struct QTeeFormatItem_s);
    }
}

//-----------------------------------------------------------------------------

int qtee_format_item__encrypted (QTeeFormatItem self)
{
    return (self->format_type == FORMAT_TYPE_ENCRYPTED); 
}

//-----------------------------------------------------------------------------

CapeString qtee_format_item__string (QTeeFormatItem self, const CapeString original_value, fct_cape_template__on_pipe on_pipe)
{
    CapeString ret = NULL;
    
    switch (self->format_type)
    {
        case FORMAT_TYPE_DECIMAL:
        {
            ret = qtee_gen_decimal (cape_str_to_f (original_value), self->format_text);
            break;
        }
        case FORMAT_TYPE_LPAD:
        {
            ret = qtee_gen_lpadding (original_value, self->format_text);
            break;
        }
        case FORMAT_TYPE_SUBSTR:
        {
            ret = qtee_gen_substr (original_value, self->format_text);
            break;
        }
        case FORMAT_TYPE_DATE_UTC:
        {
            CapeDatetime dt;
            
            // convert text into dateformat
            if (cape_datetime__str_msec (&dt, original_value) || cape_datetime__str (&dt, original_value) || cape_datetime__std_usec (&dt, original_value) || cape_datetime__std_msec (&dt, original_value) || cape_datetime__date_de (&dt, original_value))
            {
                ret = cape_datetime_s__fmt_utc (&dt, self->format_text);
            }
            
            break;
        }
        case FORMAT_TYPE_DATE_LCL:
        {
            CapeDatetime dt;
            
            // convert text into dateformat
            if (cape_datetime__str_msec (&dt, original_value) || cape_datetime__str (&dt, original_value) || cape_datetime__std_usec (&dt, original_value) || cape_datetime__std_msec (&dt, original_value) || cape_datetime__date_de (&dt, original_value))
            {
                // set the original as UTC
                dt.is_utc = TRUE;
                
                // convert into local time
                cape_datetime_to_local (&dt);
                
                ret = cape_datetime_s__fmt_lcl (&dt, self->format_text);                
            }

            break;
        }
        case FORMAT_TYPE_PIPE:
        {
            if (on_pipe)
            {
                ret = on_pipe (self->pipe, self->format_text, original_value);
            }
            else
            {
                ret = cape_str_cp (self->format_text);
            }
            
            break;
        }
    }
    
    return ret;
}

//-----------------------------------------------------------------------------

CapeString qtee_format_item__number (QTeeFormatItem self, number_t original_value)
{
    CapeString ret = NULL;
    
    switch (self->format_type)
    {
        case FORMAT_TYPE_DECIMAL:
        {
            ret = qtee_gen_decimal ((double)original_value, self->format_text);
            break;
        }
        case FORMAT_TYPE_LPAD:
        {
            CapeString h = cape_str_n (original_value);
            
            ret = qtee_gen_lpadding (h, self->format_text);
            
            cape_str_del (&h);
            break;            
        }
        case FORMAT_TYPE_SUBSTR:
        {
            CapeString h = cape_str_n (original_value);

            ret = qtee_gen_substr (h, self->format_text);

            cape_str_del (&h);
            break;            
        }
        
    }
    
    return ret;
}

//-----------------------------------------------------------------------------

CapeString qtee_format_item__float (QTeeFormatItem self, double original_value)
{
    CapeString ret = NULL;
    
    switch (self->format_type)
    {
        case FORMAT_TYPE_DECIMAL:
        {
            ret = qtee_gen_decimal (original_value, self->format_text);
            break;
        }
        case FORMAT_TYPE_LPAD:
        {
            CapeString h = cape_str_f (original_value);
            
            ret = qtee_gen_lpadding (h, self->format_text);
            
            cape_str_del (&h);
            break;            
        }
        case FORMAT_TYPE_SUBSTR:
        {
            CapeString h = cape_str_f (original_value);
            
            ret = qtee_gen_substr (h, self->format_text);
            
            cape_str_del (&h);
            break;            
        }
        
    }
    
    return ret;
}

//-----------------------------------------------------------------------------

CapeString qtee_format_item__datetime (QTeeFormatItem self, const CapeDatetime* original_value)
{
    CapeString ret = NULL;
    
    switch (self->format_type)
    {
        case FORMAT_TYPE_DATE_UTC:
        {
            ret = cape_datetime_s__fmt_utc (original_value, self->format_text);
            break;
        }
        case FORMAT_TYPE_DATE_LCL:
        {
            CapeDatetime* dt = cape_datetime_cp (original_value);
            
            // set the original as UTC
            dt->is_utc = TRUE;
                
            // convert into local time
            cape_datetime_to_local (dt);
                
            ret = cape_datetime_s__fmt_lcl (dt, self->format_text);                
            
            cape_datetime_del (&dt);
            break;
        }
    }
    
    return ret;
}

//-----------------------------------------------------------------------------

CapeString qtee_format_item__boolean (QTeeFormatItem self, int original_value)
{
    CapeString ret = NULL;
    
        
    return ret;
}

//-----------------------------------------------------------------------------

CapeString qtee_format_item__udc (QTeeFormatItem self, CapeUdc found_item, fct_cape_template__on_pipe on_pipe)
{
    CapeString ret = NULL;
    
    switch (cape_udc_type (found_item))
    {
        case CAPE_UDC_STRING:
        {
            ret = qtee_format_item__string (self, cape_udc_s (found_item, NULL), on_pipe);
            break;
        }
        case CAPE_UDC_NUMBER:
        {
            ret = qtee_format_item__number (self, cape_udc_n (found_item, 0));
            break;
        }
        case CAPE_UDC_FLOAT:
        {
            ret = qtee_format_item__float (self, cape_udc_f (found_item, .0));
            break;
        }
        case CAPE_UDC_DATETIME:
        {
            ret = qtee_format_item__datetime (self, cape_udc_d (found_item, NULL));
            break;
        }
        case CAPE_UDC_BOOL:
        {
            ret = qtee_format_item__boolean (self, cape_udc_b (found_item, FALSE));
            break;
        }
    }
    
    return ret;
}


//-----------------------------------------------------------------------------

struct QTeeFormat_s
{
    CapeList formats;
    
    CapeString node_name;
    int encrypted;
};

//-----------------------------------------------------------------------------

QTeeFormat qtee_format_new (void)
{
    QTeeFormat self = CAPE_NEW (struct QTeeFormat_s);
    
    self->formats = NULL;
    self->encrypted = FALSE;
    
    return self;
}

//-----------------------------------------------------------------------------

void qtee_format_del (QTeeFormat* p_self)
{
    if (*p_self)
    {
        QTeeFormat self = *p_self;
        
        cape_list_del (&(self->formats));
        
        CAPE_DEL (p_self, struct QTeeFormat_s);
    }
}

//-----------------------------------------------------------------------------

QTeeFormat qtee_format_gen (const CapeString possible_format)
{
    QTeeFormat self = qtee_format_new ();
    
    qtee_format_parse (self, possible_format);
    
    return self;
}

//-----------------------------------------------------------------------------

void __STDCALL qtee_format__formats__on_del (void* ptr)
{
    QTeeFormatItem item = ptr;
    
    qtee_format_item_del (&item);   
}

//-----------------------------------------------------------------------------

void qtee_format_parse__items (QTeeFormat self, const CapeString possible_items)
{
    // local objects
    CapeList parts = cape_tokenizer_buf__noempty (possible_items, cape_str_size (possible_items), '|');
    CapeListCursor* cursor = NULL;
    
    if (0 == cape_list_size (parts))
    {
        goto cleanup_and_exit;
    }
        
    // create anew list to hold all formats
    self->formats = cape_list_new (qtee_format__formats__on_del);
    
    cursor = cape_list_cursor_new (parts, CAPE_DIRECTION_FORW);
    
    while (cape_list_cursor_next (cursor))
    {
        cape_list_push_back (self->formats, (void*)qtee_format_item_new (cape_list_node_data (cursor->node)));
    }
    
cleanup_and_exit:
    
    cape_list_cursor_del (&cursor);
    cape_list_del (&parts);
}

//-----------------------------------------------------------------------------

void qtee_format_parse (QTeeFormat self, const CapeString possible_format)
{
    // local objects
    CapeString s1 = NULL;
    CapeString s2 = NULL;
    
    // cleanup old formats
    cape_list_del (&(self->formats));
    
    if (cape_tokenizer_split (possible_format, '|',  &s1, &s2))
    {
        // store the name of the node
        self->node_name = cape_str_trim_utf8 (s1);

        // parse for special case encrypted
        if (cape_str_begins (s2, "encrypted"))
        {
            self->encrypted = TRUE;
        }
        else
        {
            qtee_format_parse__items (self, s2);
        }        
    }
    else
    {
        self->node_name = cape_str_trim_utf8 (possible_format);
    }
    
    cape_str_del (&s2);
    cape_str_del (&s1);
}

//-----------------------------------------------------------------------------

int qtee_format_has_encrypted (QTeeFormat self)
{
    return self->encrypted;    
}

//-----------------------------------------------------------------------------

CapeUdc qtee_format__seek_item__from_stack (CapeList node_stack, const CapeString name)
{
    CapeUdc ret = NULL;
    
    // local objects
    CapeListCursor* cursor = cape_list_cursor_new (node_stack, CAPE_DIRECTION_PREV);
    
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
    
    cape_list_cursor_del (&cursor);
    return ret;
}

//-----------------------------------------------------------------------------

CapeUdc qtee_format_seek_item (CapeList node_stack, const CapeString name)
{
    CapeUdc ret = NULL;
    CapeUdc sub = NULL;
    
    // split the name into its parts
    CapeList name_parts = cape_tokenizer_buf (name, cape_str_size (name), '.');
    CapeListCursor* name_parts_cursor = cape_list_cursor_new (name_parts, CAPE_DIRECTION_FORW);
    
    if (cape_list_cursor_next (name_parts_cursor))
    {
        sub = qtee_format__seek_item__from_stack (node_stack, cape_list_node_data (name_parts_cursor->node));
    }
    
    while (sub && cape_list_cursor_next (name_parts_cursor))
    {
        sub = cape_udc_get (sub, cape_list_node_data (name_parts_cursor->node));
    }
    
    ret = sub;
    
    cape_list_cursor_del (&name_parts_cursor);
    cape_list_del (&name_parts);
    
    return ret;
}

//-----------------------------------------------------------------------------

CapeUdc qtee_format_item (QTeeFormat self, CapeList node_stack)
{
    CapeUdc ret = qtee_format_seek_item (node_stack, self->node_name);    
    
    if (NULL == ret)
    {
        cape_log_fmt (CAPE_LL_WARN, "QTEE", "template", "can't find node '%s'", self->node_name);
    }

    return ret;
}

//-----------------------------------------------------------------------------

CapeString qtee_format_apply_node (QTeeFormat self, CapeUdc found_item, fct_cape_template__on_pipe on_pipe)
{
    CapeString ret = NULL;
    
    if (self->formats)
    {
        // local objects
        CapeListCursor* cursor = cape_list_cursor_new (self->formats, CAPE_DIRECTION_FORW);
        
        if (cape_list_cursor_next (cursor))
        {
            ret = qtee_format_item__udc (cape_list_node_data (cursor->node), found_item, on_pipe);
            
            while (cape_list_cursor_next (cursor))
            {
                CapeString h = qtee_format_item__string (cape_list_node_data (cursor->node), ret, on_pipe);
                
                cape_str_replace_mv (&ret, &h);
            }
        }

        cape_list_cursor_del (&cursor);
    }
    else
    {
        switch (cape_udc_type (found_item))
        {
            case CAPE_UDC_STRING:
            {
                ret = cape_str_cp (cape_udc_s (found_item, NULL));
                break;
            }
            case CAPE_UDC_NUMBER:
            {
                ret = cape_str_n (cape_udc_n (found_item, 0));
                break;
            }
            case CAPE_UDC_FLOAT:
            {
                ret = cape_str_f (cape_udc_f (found_item, .0));
                break;
            }
            case CAPE_UDC_DATETIME:
            {
                ret = cape_datetime_s__std (cape_udc_d (found_item, NULL));
                break;
            }
            case CAPE_UDC_BOOL:
            {
                ret = cape_str_cp (cape_udc_b (found_item, FALSE) ? "TRUE" : "FALSE");
                break;
            }
        }
    }
    
    return ret;
}

//-----------------------------------------------------------------------------

CapeString qtee_format_apply_s (QTeeFormat self, const CapeString value, fct_cape_template__on_pipe on_pipe)
{
    CapeString ret = NULL;
    
    if (self->formats)
    {
        // local objects
        CapeListCursor* cursor = cape_list_cursor_new (self->formats, CAPE_DIRECTION_FORW);
        
        if (cape_list_cursor_next (cursor))
        {
            ret = qtee_format_item__string (cape_list_node_data (cursor->node), value, on_pipe);
            
            while (cape_list_cursor_next (cursor))
            {
                CapeString h = qtee_format_item__string (cape_list_node_data (cursor->node), ret, on_pipe);
                
                cape_str_replace_mv (&ret, &h);
            }
        }
        
        cape_list_cursor_del (&cursor);
    }
    else
    {
        ret = cape_str_cp (value);
    }
    
    return ret;
}

//-----------------------------------------------------------------------------

CapeString qtee_format_apply_n (QTeeFormat self, number_t value, fct_cape_template__on_pipe on_pipe)
{
    CapeString ret = NULL;
    
    if (self->formats)
    {
        // local objects
        CapeListCursor* cursor = cape_list_cursor_new (self->formats, CAPE_DIRECTION_FORW);
        
        if (cape_list_cursor_next (cursor))
        {
            ret = qtee_format_item__number (cape_list_node_data (cursor->node), value);
            
            while (cape_list_cursor_next (cursor))
            {
                CapeString h = qtee_format_item__string (cape_list_node_data (cursor->node), ret, on_pipe);
                
                cape_str_replace_mv (&ret, &h);
            }
        }
        
        cape_list_cursor_del (&cursor);
    }
    else
    {
        ret = cape_str_n (value);
    }
    
    return ret;
}

//-----------------------------------------------------------------------------

CapeString qtee_format_apply_f (QTeeFormat self, double value, fct_cape_template__on_pipe on_pipe)
{
    CapeString ret = NULL;

    if (self->formats)
    {
        // local objects
        CapeListCursor* cursor = cape_list_cursor_new (self->formats, CAPE_DIRECTION_FORW);
        
        if (cape_list_cursor_next (cursor))
        {
            ret = qtee_format_item__float (cape_list_node_data (cursor->node), value);
            
            while (cape_list_cursor_next (cursor))
            {
                CapeString h = qtee_format_item__string (cape_list_node_data (cursor->node), ret, on_pipe);
                
                cape_str_replace_mv (&ret, &h);
            }
        }
        
        cape_list_cursor_del (&cursor);
    }
    else
    {
        ret = cape_str_f (value);
    }

    return ret;
}

//-----------------------------------------------------------------------------

CapeString qtee_format_apply_b (QTeeFormat self, int value, fct_cape_template__on_pipe on_pipe)
{
    CapeString ret = NULL;

    ret = cape_str_cp (value ? "TRUE" : "FALSE");
    
    return ret;
}

//-----------------------------------------------------------------------------

CapeString qtee_format_apply_d (QTeeFormat self, const CapeDatetime* value, fct_cape_template__on_pipe on_pipe)
{
    CapeString ret = NULL;
    
    if (self->formats)
    {
        // local objects
        CapeListCursor* cursor = cape_list_cursor_new (self->formats, CAPE_DIRECTION_FORW);
        
        if (cape_list_cursor_next (cursor))
        {
            ret = qtee_format_item__datetime (cape_list_node_data (cursor->node), value);
            
            while (cape_list_cursor_next (cursor))
            {
                CapeString h = qtee_format_item__string (cape_list_node_data (cursor->node), ret, on_pipe);
                
                cape_str_replace_mv (&ret, &h);
            }
        }
        
        cape_list_cursor_del (&cursor);
    }
    else
    {
        ret = cape_datetime_s__std (value);
    }
    
    return ret;
}

//-----------------------------------------------------------------------------
