#include "qtee_gen.h"

// cape inlcudes
#include <fmt/cape_tokenizer.h>
#include <sys/cape_log.h>

// c includes
#include <math.h>

//-----------------------------------------------------------------------------

CapeString qtee_gen_decimal (double value, const CapeString format_text)
{
    CapeString ret = NULL;
    
    double fraction = 1.0;
    CapeString devider = NULL;
    number_t decimal = 3;
    
    // look for cape standard options
    CapeUdc options = cape_tokenizer_options (format_text);
    if (options)
    {
        fraction = cape_udc_get_f (options, "fraction", fraction);
        devider = cape_str_cp (cape_udc_get_s (options, "delimiter", "."));
        decimal = cape_udc_get_n (options, "digits", decimal);
        
        cape_udc_del (&options);
    }
    else
    {
        CapeList tokens = cape_tokenizer_buf (format_text, cape_str_size (format_text), '%');
        
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
    
    //printf ("fraction = %f, devider = '%s', decimal = %lu\n", fraction, devider, decimal);
    
    // calculate value
    {
        double v = (double)value / fraction;
        
        number_t dh = pow (10, decimal);
        
        // fix an issue with rounding
        double v2 = v * dh + 0.5000000001;
        
        number_t dj = floor(v2);
        
        double dk = (double)dj / dh;
        
        {
            CapeString fmt = cape_str_fmt ("%%.%if", decimal);
            
            ret = cape_str_fmt (fmt, dk);
            
            cape_str_replace (&ret, ".", devider);
            
            cape_str_del (&fmt);            
        }
    }
    
    cape_str_del (&devider);
    
    return ret;
}

//-----------------------------------------------------------------------------

CapeString qtee_gen_lpadding (const CapeString value, const CapeString format_text)
{
    CapeString ret = NULL;
    
    // convert eval value into number
    number_t digits = cape_str_to_n (format_text);
    
    if (digits)
    {
        ret = cape_str_lpad (value, ' ', digits);    
    }
    else
    {
        ret = cape_str_cp (value);
    }

    return ret;
}

//-----------------------------------------------------------------------------

CapeString qtee_gen_substr (const CapeString value, const CapeString format_text)
{
    CapeString ret = NULL;

    // local objects
    CapeList tokens = cape_tokenizer_buf (format_text, cape_str_size (format_text), '%');
    
    if (cape_list_size (tokens) == 2)
    {
        number_t pos = 0;
        number_t len = 0;
        
        CapeListCursor* cursor = cape_list_cursor_new (tokens, CAPE_DIRECTION_FORW);
        
        cape_list_cursor_next (cursor);
        pos = cape_str_to_n (cape_list_node_data (cursor->node));
        
        cape_list_cursor_next (cursor);
        len = cape_str_to_n (cape_list_node_data (cursor->node));
        
        cape_list_cursor_del (&cursor);

        ret = cape_str_sub (value + pos, len);                
    }
    else
    {
        ret = cape_str_cp (value);
    }
    
    cape_list_del (&tokens);
    
    return ret;
}

//-----------------------------------------------------------------------------
