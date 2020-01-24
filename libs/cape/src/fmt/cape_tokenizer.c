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

CapeList cape_tokenizer_str (const CapeString haystack, const CapeString needle)
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

CapeList cape_tokenizer_str_utf8 (const CapeString haystack, const CapeString needle)
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
