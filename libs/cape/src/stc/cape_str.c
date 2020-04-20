// c includes
#ifdef __GNUC__
#define _GNU_SOURCE 1
#endif

#include "cape_str.h"
#include "cape_stream.h"

// cape includes
#include "sys/cape_err.h"
#include "sys/cape_log.h"
#include "fmt/cape_dragon4.h"

#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>

#include <stdio.h>

//-----------------------------------------------------------------------------

CapeString cape_str_cp (const CapeString source)
{
  if (source == NULL)
  {
    return NULL;
  }  

#ifdef _WIN32
  return _strdup (source);  
#else
  return strdup (source);
#endif    
}

//-----------------------------------------------------------------------------

CapeString cape_str_mv (CapeString* p_source)
{
  CapeString ret = *p_source;
  
  *p_source = NULL;
  
  return ret;
}

//-----------------------------------------------------------------------------

CapeString cape_str_sub (const CapeString source, number_t len)
{
  char* ret;
  
  // check if we have at least value 
  if (source == NULL)
  {
    return NULL;
  }  
  
  ret = (char*)CAPE_ALLOC( (2 + len) * sizeof(char) );
  
  /* copy the part */
  memcpy(ret, source, len * sizeof(char));
  
  /* set the termination */
  ret[len] = 0;
  
  return ret; 
}

//-----------------------------------------------------------------------------

void cape_str_del (CapeString* p_self)
{
  CapeString self = *p_self;
    
  if(self)
  {
    memset (self, 0x00, strlen(self));
    free (self);
  }
  
  *p_self = NULL;
}

//-----------------------------------------------------------------------------

number_t cape_str_utf8__len (unsigned char c)
{
  if (0x20 <= c && c <= 0x7E)
  {
    // ascii
    return 1;
  }
  else if ((c & 0xE0) == 0xC0)
  {
    // +1
    return 2;
  }
  else if ((c & 0xF0) == 0xE0)
  {
    // +2
    return 3;
  }
  else if ((c & 0xF8) == 0xF0)
  {
    // +3
    return 4;
  }
  else if ((c & 0xFC) == 0xF8)
  {
    // +4
    return 5;
  }
  else if ((c & 0xFE) == 0xFC)
  {
    // +5
    return 6;
  }
  else
  {
    // not supported character
    return 0;
  }
}

//-----------------------------------------------------------------------------

number_t cape_str_char__len (unsigned char c)
{
  if (0x20 <= c && c <= 0x7E)
  {
    // ascii
    return 1;
  }
  else if ((c & 0xE0) == 0xC0)
  {
    // +1
    return 2;
  }
  else if ((c & 0xF0) == 0xE0)
  {
    // +2
    return 3;
  }
  else if ((c & 0xF8) == 0xF0)
  {
    // +3
    return 4;
  }
  else if ((c & 0xFC) == 0xF8)
  {
    // +4
    return 5;
  }
  else if ((c & 0xFE) == 0xFC)
  {
    // +5
    return 6;
  }
  else
  {
    // not supported character
    return 1;
  }
}

//-----------------------------------------------------------------------------

number_t cape_str_len (const CapeString s)
{
  number_t len = 0;
  const char* s_pos = s;

  while (*s_pos)
  {
    len++;
    s_pos += cape_str_char__len (*s_pos);
  }
  
  return len;
}

//-----------------------------------------------------------------------------

number_t cape_str_size (const CapeString s)
{
  return (number_t)strlen (s);
}

//-----------------------------------------------------------------------------

CapeString cape_str_f (double value)
{
  int res;

  CapeString ret = CAPE_ALLOC (1025);
  
  CapeErr err = cape_err_new ();
  
  CapeDragon4 dragon4 = cape_dragon4_new ();
  
  cape_dragon4_positional (dragon4, CAPE_DRAGON4__DMODE_UNIQUE, CAPE_DRAGON4__CMODE_TOTAL, -1, FALSE, CAPE_DRAGON4__TMODE_ONE_ZERO, 0, 0);
  

  res = cape_dragon4_run (dragon4, ret, 1024, value, err);
  if (res)
  {
    
  }
  else
  {
  }
  
  ret[cape_dragon4_len (dragon4)] = '\0';
  
  cape_dragon4_del (&dragon4);
  
  cape_err_del (&err);

  return ret;
}

//-----------------------------------------------------------------------------

CapeString cape_str_n (number_t value)
{
  CapeString ret = CAPE_ALLOC (26);  // for very long intergers
  
#ifdef _MSC_VER
    
  _snprintf_s (ret, 24, _TRUNCATE, "%li", value);
    
#else
    
  // TODO: use a different %i / %lli if number_t is 64bit etc
  snprintf (ret, 24, "%li", value);
    
#endif

  return ret;
}

//-----------------------------------------------------------------------------

int cape_str_empty (const CapeString s)
{
  if (s)
  {
    return (*s == '\0');
  }
  
  return TRUE;
}

//-----------------------------------------------------------------------------

int cape_str_not_empty (const CapeString s)
{
  if (s)
  {
    return (*s != '\0');
  }
  
  return FALSE;
}

//-----------------------------------------------------------------------------

int cape_str_equal (const CapeString s1, const CapeString s2)
{
  if ((NULL == s1) || (NULL == s2))
  {
    return FALSE;
  }

  return strcmp (s1, s2) == 0;
}

//-----------------------------------------------------------------------------

int cape_str_compare_c (const CapeString s1, const CapeString s2)
{
  if (NULL == s1)
  {
    return -1;
  }

  if (NULL == s2)
  {
    return 1;
  }
  
#ifdef _MSC_VER
  
  return _stricmp (s1, s2);
  
#elif __GNUC__
  
  return strcasecmp (s1, s2);
  
#endif
}

//-----------------------------------------------------------------------------

int cape_str_compare (const CapeString s1, const CapeString s2)
{
  if ((NULL == s1) || (NULL == s2))
  {
    return FALSE;
  }
  
#ifdef _MSC_VER
  
  return _stricmp (s1, s2) == 0;
  
#elif __GNUC__  

  return strcasecmp (s1, s2) == 0;

#endif
}

//-----------------------------------------------------------------------------

int cape_str_begins (const CapeString s, const CapeString begins_with)
{
  if (s == NULL)
  {
    return FALSE;
  }
  
  if (begins_with == NULL)
  {
    return FALSE;
  }
  
  {
    size_t len = strlen(begins_with);
    
    return strncmp (s, begins_with, len) == 0;
  }
}

//-----------------------------------------------------------------------------

int cape_str_begins_i (const CapeString s1, const CapeString s2)
{
  int ret;
  
  number_t l1 = (number_t)strlen (s1);
  number_t l2 = (number_t)strlen (s2);

  if (l2 < l1)
  {
    CapeString h = cape_str_sub (s1, l2);
    
    ret = cape_str_compare (h, s2);
    
    cape_str_del (&h);
  }
  else
  {
    ret = cape_str_compare (s1, s2);
  }
  
  return ret;
}

//-----------------------------------------------------------------------------

int cape_str_find (const CapeString haystack, const CapeString needle, number_t* p_pos)
{
  if (p_pos)
  {
    // use the string.h function to find the first occourence
    char* res = strstr (haystack, needle);
    if (res)
    {
      // calculate the position
      *p_pos = res - haystack;
      return TRUE;
    }
  }

  return FALSE;
}

//-----------------------------------------------------------------------------

int cape_str_find_utf8 (const CapeString haystack, const CapeString needle, number_t* pos_len, number_t* pos_size)
{
  if (pos_len && pos_size)
  {
    const char* spos = haystack;
    number_t cpos = 0;
    number_t len = (number_t)strlen (needle);
    
    // iterate through all characters
    while (*spos)
    {
      number_t char_len = cape_str_char__len (*spos);
      
      if (strncmp (spos, needle, len) == 0)
      {
        *pos_len = cpos;
        *pos_size = spos - haystack;
        
        return TRUE;
      }
      
      spos += char_len;
      cpos ++;
    }
  }
  
  return FALSE;
}

//-----------------------------------------------------------------------------

int cape_str_next (const CapeString self, char c, number_t* p_pos)
{
  if (p_pos)
  {
    char* r = strchr (self, c);
    if (r)
    {
      *p_pos = r - self;
      return TRUE;
    }
  }
  
  return FALSE;
}

//-----------------------------------------------------------------------------

CapeString cape_str_uuid (void)
{
  CapeString self = (CapeString)CAPE_ALLOC(38);
  
#if defined _MSC_VER

  sprintf_s (self, 38, "%04X%04X-%04X-%04X-%04X-%04X%04X%04X", 
          rand() & 0xffff, rand() & 0xffff,                          // Generates a 64-bit Hex number
          rand() & 0xffff,                                           // Generates a 32-bit Hex number
          ((rand() & 0x0fff) | 0x4000),                              // Generates a 32-bit Hex number of the form 4xxx (4 indicates the UUID version)
          rand() % 0x3fff + 0x8000,                                  // Generates a 32-bit Hex number in the range [0x8000, 0xbfff]
          rand() & 0xffff, rand() & 0xffff, rand() & 0xffff);        // Generates a 96-bit Hex number

#else

  sprintf (self, "%04X%04X-%04X-%04X-%04X-%04X%04X%04X", 
          rand() & 0xffff, rand() & 0xffff,                          // Generates a 64-bit Hex number
          rand() & 0xffff,                                           // Generates a 32-bit Hex number
          ((rand() & 0x0fff) | 0x4000),                              // Generates a 32-bit Hex number of the form 4xxx (4 indicates the UUID version)
          rand() % 0x3fff + 0x8000,                                  // Generates a 32-bit Hex number in the range [0x8000, 0xbfff]
          rand() & 0xffff, rand() & 0xffff, rand() & 0xffff);        // Generates a 96-bit Hex number

#endif

  return self;
  
  /*
  CapeString self = CAPE_ALLOC(38);
  int t = 0;
  
  char *szTemp = "xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx";
  char *szHex = "0123456789ABCDEF-";
  int nLen = 36;
  unsigned char* pos = (unsigned char*)self;
  
  srand (clock());
  
  for (t = 0; t < nLen + 1; t++, pos++)
  {
    int r = rand () % 16;
    char c = ' ';   
    
    switch (szTemp[t])
    {
      case 'x' : { c = szHex [r]; } break;
      case 'y' : { c = szHex [(r & 0x03) | 0x08]; } break;
      case '-' : { c = '-'; } break;
      case '4' : { c = '4'; } break;
    }
    
    *pos = ( t < nLen ) ? c : 0x00;
  }
  
  return self;
  */
}

//-----------------------------------------------------------------------------

CapeString cape_str_random (number_t len)
{
  number_t i;
  CapeString self = CAPE_ALLOC (len + 1);
  
  for (i = 0; i < len; i++)
  {
    self[i] = (rand() % 26) + 97;
  }
  
  // set termination
  self[i] = 0;
  
  return self;
}

//-----------------------------------------------------------------------------

CapeString cape_str_flp (const CapeString format, va_list valist)
{
  CapeString ret = NULL;
  
#ifdef _MSC_VER
  
  {
    int len = _vscprintf (format, valist) + 1;
    
    ret = (CapeString)CAPE_ALLOC (len);
    
    len = vsprintf_s (ret, len, format, valist);
  }
  
#elif __GNUC__
  
  {
    char* strp;
    
    int bytesWritten = vasprintf (&strp, format, valist);
    
    if (bytesWritten == -1)
    {
      CapeErr err = cape_err_new ();
      
      cape_err_lastOSError (err);
      
      cape_log_fmt (CAPE_LL_ERROR, "CAPE", "str_fmt", "can't format string: %s", cape_err_text(err));
      
      cape_err_del (&err);
    }
    else if (bytesWritten == 0)
    {
      cape_log_fmt (CAPE_LL_WARN, "CAPE", "str_fmt", "format of string returned 0");      
    }
    else if (bytesWritten > 0)
    {
      ret = strp;
    }
  }
  
#elif __BORLANDC__
  
  {
    int len = 1024;
    
    ret = CAPE_NEW (len);
    
    len = vsnprintf (ret, len, format, valist);    
  }
  
#endif
  
  return ret;
}

//-----------------------------------------------------------------------------

CapeString cape_str_fmt (const CapeString format, ...)
{
  CapeString ret = NULL;
  
  va_list ptr;  
  va_start(ptr, format);
  
  ret = cape_str_flp (format, ptr);
  
  va_end(ptr); 
  
  return ret;  
}

//-----------------------------------------------------------------------------

CapeString cape_str_catenate_2 (const CapeString s1, const CapeString s2)
{
  if( !s1 )
  {
    return cape_str_cp (s2);  
  }
  
  if( !s2 )
  {
    return cape_str_cp (s1);  
  }
  
  {
    /* variables */
    number_t s1_len = (number_t)strlen(s1);
	number_t s2_len = (number_t)strlen(s2);
    
    char* ret;
    char* pos;
    
    ret = (char*)CAPE_ALLOC( (s1_len + s2_len + 10) );
    if (ret == NULL) 
    {
      return NULL;
    }
    
    pos = ret;
    
    memcpy(pos, s1, s1_len);
    pos = pos + s1_len;
    
    memcpy(pos, s2, s2_len);
    pos = pos + s2_len;
    
    *pos = 0;
    
    return ret;
  }
}

//-----------------------------------------------------------------------------

CapeString cape_str_catenate_3 (const CapeString s1, const CapeString s2, const CapeString s3)
{
  number_t s1_len = (number_t)strlen(s1);
  number_t s2_len = (number_t)strlen(s2);
  number_t s3_len = (number_t)strlen(s3);
  
  char* ret = (char*)CAPE_ALLOC( (s1_len + s2_len + s3_len + 1) * sizeof(char) );
  
  char* pos = ret;
  
  memcpy(pos, s1, s1_len);
  pos = pos + s1_len;
  
  memcpy(pos, s2, s2_len);
  pos = pos + s2_len;
  
  memcpy(pos, s3, s3_len);
  pos = pos + s3_len;
  
  *pos = 0;
  
  return ret;   
}

//-----------------------------------------------------------------------------

CapeString cape_str_catenate_c (const CapeString s1, char c, const CapeString s2)
{
  // variables
  number_t s1_len;
  number_t s2_len;
  char* ret;
  char* pos;
  
  if (s1 == NULL) 
  {
    return NULL;
  }
  
  if (s2 == NULL) 
  {
    return NULL;
  }
  
  s1_len = strlen(s1);
  s2_len = strlen(s2);
  
  ret = (char*)CAPE_ALLOC( (s1_len + s2_len + 2) );
  
  if (ret == NULL) 
  {
    return NULL;
  }
  
  pos = ret;
  
  if (s1_len > 0) 
  {
    memcpy(ret, s1, s1_len);
    pos = pos + s1_len;
  }
  
  *pos = c;
  pos++;
  
  if (s2_len > 0)
  {
    memcpy(pos, s2, s2_len);
    pos = pos + s2_len;    
  }
  
  *pos = 0;
  
  return ret;  
}

//-----------------------------------------------------------------------------

CapeString cape_str_trim_utf8 (const CapeString source)
{
  const unsigned char* c = (const unsigned char*)source;
  
  const unsigned char* pos_s = c;
  const unsigned char* pos_e = c;
  
  number_t diff;
  
  // special case
  if (source == NULL)
  {
    return NULL;
  }
  
  number_t clen = 0;
  
  while (*c)
  {
    clen = cape_str_char__len (*c);
    if (clen > 1)
    {
      int i;
      // test if the char is stil in the string
      
      for (i = 1; i < clen; i++)
      {
        if (*(c + i) == 0)
        {
          break;
        }        
      }
      
      if (i == clen)
      {
        break;
      }
      else
      {
        clen = i - 1;
        pos_s += clen;        
      }
    }
    else
    {
      if (*c <= 32)
      {
        pos_s += clen;
      }
      else
      {
        // normal character
        break;
      }    
    }
    
    c += clen;
  }

  pos_e = c + clen;
  c += clen;

  while (*c)
  {
    clen = cape_str_char__len (*c);
    if (clen > 1)
    {
      int i;
      // test if the char is stil in the string
      
      for (i = 1; i < clen; i++)
      {
        if (*(c + i) == 0)
        {
          break;
        }
      }

      if (i == clen)
      {
        pos_e = c + clen;      
      }
      else
      {
        // something is corupt
        break;
      }
    }
    else
    {
      if (*c <= 32)
      {
        
      }
      else
      {
        pos_e = c + clen;      
      }    
    }
    
    c += clen;
  }
  
  diff = pos_e - pos_s;
  if (diff > 0)
  {
    return cape_str_sub ((const char*)pos_s, pos_e - pos_s);
  }
  else
  {
    return cape_str_cp ("");
  }
}

//-----------------------------------------------------------------------------

CapeString cape_str_trim_lr (const CapeString source, char l, char r)
{
  CapeString copy;
  char* pos01;
  char* pos02;
  
  if (NULL == source)
  {
    return 0;
  }
  
  copy = cape_str_cp (source);
  
  /* source position */
  pos01 = copy;
  pos02 = 0;
  /* trim from begin */
  while(*pos01)
  {
    if (*pos01 != l) break;
    
    pos01++;
  }
  pos02 = copy;
  /* copy rest */
  while(*pos01)
  {
    *pos02 = *pos01;
    
    pos01++;
    pos02++;
  }
  /* set here 0 not to run in undefined situation */
  *pos02 = 0;
  /* trim from the end */
  while( pos02 != copy )
  {
    /* decrease */
    pos02--;
    /* check if readable */
    if (*pos02 != r) break;
    /* set to zero */
    *pos02 = 0;
  }
  
  return copy;
}

//-----------------------------------------------------------------------------

CapeString cape_str_trim_c (const CapeString source, char c)
{
  return cape_str_trim_lr (source, c, c);
}

//-----------------------------------------------------------------------------

CapeString cape_str_sanitize_utf8 (const CapeString source)
{
  number_t src_len = strlen(source);
  number_t src_pos = 0;
  
  char* ret_src = CAPE_ALLOC (src_len + 1);
  
  const char* pos = source;
  char* ret = ret_src;
  
  for (src_pos = 0; src_pos < src_len;)
  {
    number_t char_len = cape_str_utf8__len (*pos);

    
    if (char_len)
    {
      src_pos += char_len;

      if (src_pos > src_len)
      {
        // invalid utf8
      }
      else
      {
        memcpy (ret, pos, char_len);

        ret += char_len;
        pos += char_len;
      }
    }
    else
    {
      // invalid utf8
      pos++;
      src_pos++;
    }
  }

  printf ("%s\n", ret);


  // add termination
  *ret = 0;
  
  return ret_src;
}

//-----------------------------------------------------------------------------

CapeString cape_str_cp_replaced (const CapeString source, const CapeString seek, const CapeString replace_with)
{
  CapeStream s;
  
  const char* fpos;
  const char* lpos;
  
  if (source == NULL)
  {
    return NULL;
  }
  
  if (seek == NULL)
  {
    return cape_str_cp (source);
  }
  
  s = cape_stream_new ();
  
  lpos = source;
  
  for (fpos = strstr (lpos, seek); fpos; fpos = strstr (lpos, seek))
  {
    cape_stream_append_buf (s, lpos, fpos - lpos);
    
    if (replace_with)
    {
      cape_stream_append_str (s, replace_with);
    }
    
    lpos = fpos + (number_t)strlen (seek);
  }
  
  cape_stream_append_str (s, lpos);
  
  return cape_stream_to_str (&s);
}

//-----------------------------------------------------------------------------

void cape_str_replace_cp (CapeString* p_self, const CapeString source)
{
  // create a new copy
  CapeString self = cape_str_cp (source);

  // free the old string  
  cape_str_del (p_self);
  
  // transfer ownership
  *p_self = self;
}

//-----------------------------------------------------------------------------

void cape_str_replace_mv (CapeString* p_self, CapeString* p_source)
{
  // free the old string  
  cape_str_del (p_self);

  // transfer ownership
  *p_self = *p_source;

  // release ownership
  *p_source = NULL;
}

//-----------------------------------------------------------------------------

void cape_str_replace (CapeString* p_self, const CapeString seek, const CapeString replace_with)
{
  if (*p_self)
  {
    CapeString h = cape_str_cp_replaced (*p_self, seek, replace_with);
    
    cape_str_del (p_self);
    
    *p_self = h;
  }
}

//-----------------------------------------------------------------------------

void cape_str_fill (CapeString self, number_t len, const CapeString source)
{
  // assume that the source will fit into the self object
#if defined _MSC_VER

  strncpy_s (self, len + 1, source, len);
  
#else

  strncpy (self, source, len);

#endif

  self[len] = '\0';
}

//-----------------------------------------------------------------------------

void cape_str_to_upper (CapeString self)
{
  char* pos = self;
  
  while (*pos)
  {
    *pos = toupper (*pos);
    pos++;
  }
}

//-----------------------------------------------------------------------------

void cape_str_to_lower (CapeString self)
{
  char* pos = self;
  
  while (*pos)
  {
    *pos = tolower (*pos);
    pos++;
  }
}

//-----------------------------------------------------------------------------
