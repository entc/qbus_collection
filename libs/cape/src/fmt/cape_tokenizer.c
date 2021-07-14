#include "cape_tokenizer.h"

// c includes
#include <string.h>

//-----------------------------------------------------------------------------

static void __STDCALL cape_tokenizer_onDestroy (void* ptr)
{
  CapeString h = ptr; cape_str_del (&h);
}

//-----------------------------------------------------------------------------------------------------------

CapeList cape_tokenizer_buf (const char* buffer, number_t len, char delimeter)
{
  CapeList tokens = cape_list_new (cape_tokenizer_onDestroy);
  
  const char* posR = buffer;
  const char* posL = buffer;
  
  number_t posI = 0;
  
  if (buffer == NULL)
  {
    return tokens;
  }
  
  for (posI = 0; posI < len; posI++)
  {
    if (*posR == delimeter)
    {
      // calculate the length of the last segment
      CapeString h = cape_str_sub (posL, posR - posL);

      cape_list_push_back (tokens, h);
      
      posL = posR + 1;
    }
    
    posR++;
  }
  
  {
    CapeString h = cape_str_sub (posL, posR - posL);
    
    cape_list_push_back (tokens, h);
  }
  
  return tokens;  
}

//-----------------------------------------------------------------------------------------------------------

CapeList cape_tokenizer_buf__noempty (const char* buffer, number_t len, char delimeter)
{
  CapeList tokens = cape_list_new (cape_tokenizer_onDestroy);
  
  const char* posR = buffer;
  const char* posL = buffer;
  
  number_t posI = 0;
  
  if (buffer == NULL)
  {
    return tokens;
  }
  
  for (posI = 0; posI < len; posI++)
  {
    if (*posR == delimeter)
    {
      // calculate the length of the last segment
      CapeString h = cape_str_sub (posL, posR - posL);
      
      if (cape_str_empty (h))
      {
        cape_str_del (&h);
      }
      else
      {
        cape_list_push_back (tokens, h);
      }
            
      posL = posR + 1;
    }
    
    posR++;
  }
  
  {
    CapeString h = cape_str_sub (posL, posR - posL);
    
    if (cape_str_empty (h))
    {
      cape_str_del (&h);
    }
    else
    {
      cape_list_push_back (tokens, h);
    }
  }
  
  return tokens; 
}

//-----------------------------------------------------------------------------------------------------------

int cape_tokenizer_split (const CapeString source, char token, CapeString* p_left, CapeString* p_right)
{
  if (source == NULL)
  {
    return FALSE;
  }
  {
    const char * pos = strchr (source, token);
    if (pos != NULL)
    {
      CapeString h = cape_str_sub (source, pos - source);
      
      cape_str_replace_mv (p_left, &h);
      cape_str_replace_cp (p_right, pos + 1);
      
      return TRUE;
    }
    
    return FALSE;
  }
}

//-----------------------------------------------------------------------------------------------------------

int cape_tokenizer_split_last (const CapeString source, char token, CapeString* p_left, CapeString* p_right)
{
  if (source == NULL)
  {
    return FALSE;
  }
  {
    const char * pos = strrchr (source, token);
    if (pos != NULL)
    {
      CapeString h = cape_str_sub (source, pos - source);
      
      cape_str_replace_mv (p_left, &h);
      cape_str_replace_cp (p_right, pos + 1);
      
      return TRUE;
    }
    
    return FALSE;
  }
}

//-----------------------------------------------------------------------------------------------------------

CapeList cape_tokenizer_str (const CapeString haystack, const CapeString needle)
{
  CapeList ret = cape_list_new (cape_tokenizer_onDestroy);

  number_t pos = 0;
  number_t plh = 0;
  number_t len = cape_str_size (needle);

  while (cape_str_find (haystack + plh, needle, &pos))
  {
    CapeString h = cape_str_sub (haystack + plh, pos);
    
    cape_list_push_back (ret, h);
    
    // calculate position to continue
    plh += pos + len;
  }

  {
    CapeString h = cape_str_cp (haystack + plh);

    cape_list_push_back (ret, h);
  }
  
  return ret;
}

//-----------------------------------------------------------------------------------------------------------

CapeList cape_tokenizer_str_pos (const CapeString haystack, const CapeString needle)
{
  CapeList ret = cape_list_new (NULL);

  number_t pos = 0;
  number_t plh = 0;
  number_t len = cape_str_size (needle);
  
  while (cape_str_find (haystack + plh, needle, &pos))
  {
    // calculate the absolute position
    plh += pos;
    
    cape_list_push_back (ret, (void*)plh);
    
    // calculate position to continue
    plh += len;
  }
  
  return ret;
}

//-----------------------------------------------------------------------------------------------------------

CapeList cape_tokenizer_str_utf8_pos (const CapeString haystack, const CapeString needle)
{
  CapeList ret = cape_list_new (NULL);
  
  number_t pos_len = 0;
  number_t pos_size = 0;

  number_t plh_len = 0;
  number_t plh_size = 0;

  number_t len = cape_str_size (needle);
  
  while (cape_str_find_utf8 (haystack + plh_size, needle, &pos_len, &pos_size))
  {
    // calculate the absolute position
    plh_len += pos_len;
    plh_size += pos_size;
    
    cape_list_push_back (ret, (void*)plh_len);
    
    // calculate position to continue
    plh_size += len;
  }
  
  return ret;
}

//-----------------------------------------------------------------------------------------------------------

#define CAPE_TOPTIONS_STATE__TEXT    0
#define CAPE_TOPTIONS_STATE__OPTION  2
#define CAPE_TOPTIONS_STATE__IN      3
#define CAPE_TOPTIONS_STATE__OUT     4

//-----------------------------------------------------------------------------------------------------------

void cape_tokenizer_options__append (const char* pos_key, const char* pos_val, const char* pos, CapeUdc* p_ret)
{
  CapeUdc ret = *p_ret;
  
  if (pos_key && pos_val)
  {
    CapeString key1 = cape_str_sub (pos_key, pos_val - pos_key - 1);
    CapeString val1 = cape_str_sub (pos_val, pos - pos_val);
    
    CapeString key2 = cape_str_trim_utf8 (key1);
    CapeString val2 = cape_str_trim_utf8 (val1);
    
    if (cape_str_not_empty (key2))
    {
      if (ret == NULL)
      {
        ret = cape_udc_new (CAPE_UDC_NODE, NULL);
      }

      {
        char* pos_end = NULL;
        
        // test if we can convert it into a number
        number_t h = strtol (val2, &pos_end, 10);
        
        if ((pos_end == NULL) || (pos_end == val2))
        {
          cape_udc_add_s_mv (ret, key2, &val2);
        }
        else
        {
          cape_udc_add_n (ret, key2, h);
        }
      }
    }
    
    cape_str_del (&key2);
    cape_str_del (&val2);
    cape_str_del (&key1);
    cape_str_del (&val1);
  }
  
  *p_ret = ret;
}

//-----------------------------------------------------------------------------------------------------------

CapeUdc cape_tokenizer_options (const CapeString source)
{
  CapeUdc ret = NULL;

  const char* pos;
  int state = CAPE_TOPTIONS_STATE__TEXT;
  
  const char* pos_text = NULL;
  const char* pos_option_key = NULL;
  const char* pos_option_val = NULL;

  // iterate through
  for (pos = source; *pos; pos++)
  {
    // checks for the state engine
    switch (*pos)
    {
      case '(':
      {
        switch (state)
        {
          case CAPE_TOPTIONS_STATE__TEXT:
          {
            state = CAPE_TOPTIONS_STATE__IN;
            break;
          }
          case CAPE_TOPTIONS_STATE__OPTION:
          {
            break;
          }
          case CAPE_TOPTIONS_STATE__IN:
          {
            state = CAPE_TOPTIONS_STATE__OPTION;

            pos_text = pos - 1;
            pos_option_key = pos + 1;

            break;
          }
          case CAPE_TOPTIONS_STATE__OUT:
          {
            state = CAPE_TOPTIONS_STATE__OPTION;
            break;
          }
        }
        
        break;
      }
      case ')':
      {
        switch (state)
        {
          case CAPE_TOPTIONS_STATE__TEXT:
          {
            break;
          }
          case CAPE_TOPTIONS_STATE__OPTION:
          {
            state = CAPE_TOPTIONS_STATE__OUT;
            break;
          }
          case CAPE_TOPTIONS_STATE__IN:
          {
            state = CAPE_TOPTIONS_STATE__TEXT;
            break;
          }
          case CAPE_TOPTIONS_STATE__OUT:
          {
            state = CAPE_TOPTIONS_STATE__TEXT;

            cape_tokenizer_options__append (pos_option_key, pos_option_val, pos - 1, &ret);
            break;
          }
        }

        break;
      }
      case ':':
      {
        switch (state)
        {
          case CAPE_TOPTIONS_STATE__IN:
          {
            state = CAPE_TOPTIONS_STATE__TEXT;
            break;
          }
          case CAPE_TOPTIONS_STATE__TEXT:
          {
            break;
          }
          case CAPE_TOPTIONS_STATE__OUT:
          {
            state = CAPE_TOPTIONS_STATE__OPTION;
            pos_option_val = pos + 1;

            break;
          }
          case CAPE_TOPTIONS_STATE__OPTION:
          {
            pos_option_val = pos + 1;

            break;
          }
        }
        
        break;
      }
      case ',':
      {
        switch (state)
        {
          case CAPE_TOPTIONS_STATE__IN:
          {
            state = CAPE_TOPTIONS_STATE__TEXT;
            break;
          }
          case CAPE_TOPTIONS_STATE__TEXT:
          {
            break;
          }
          case CAPE_TOPTIONS_STATE__OUT:
          {
            break;
          }
          case CAPE_TOPTIONS_STATE__OPTION:
          {
            cape_tokenizer_options__append (pos_option_key, pos_option_val, pos, &ret);
            pos_option_key = pos + 1;
            break;
          }
        }
        
        break;
      }
      default:
      {
        switch (state)
        {
          case CAPE_TOPTIONS_STATE__TEXT:
          {
            break;
          }
          case CAPE_TOPTIONS_STATE__OPTION:
          {
            break;
          }
          case CAPE_TOPTIONS_STATE__IN:
          {
            state = CAPE_TOPTIONS_STATE__TEXT;
            break;
          }
          case CAPE_TOPTIONS_STATE__OUT:
          {
            state = CAPE_TOPTIONS_STATE__OPTION;
            break;
          }
        }

        break;
      }
    }
  }
  
  if (pos_text && ret)
  {
    CapeString h1 = cape_str_sub (source, pos_text - source);
    CapeString h2 = cape_str_trim_utf8 (h1);

    cape_udc_add_s_mv (ret, "_", &h2);
    
    cape_str_del (&h1);
  }
  
  return ret;
}

//-----------------------------------------------------------------------------------------------------------
