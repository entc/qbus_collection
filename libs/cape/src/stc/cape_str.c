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

number_t cape_str_to_n (const CapeString s)
{
  return strtol (s, NULL, 10);
}

//-----------------------------------------------------------------------------

char cape_str_to_f__seek (const CapeString s)
{
  // first to to find what kind separator is used
  number_t s_len = cape_str_size (s);
  const char* pos;
  
  for (pos = s + s_len; pos > s; pos--)
  {
    switch (*pos)
    {
      case '.':
      {
        return '.';
      }
      case ',':
      {
        return ',';
      }
    }
  }
  
  return 0;
}

//-----------------------------------------------------------------------------

double cape_str_to_f (const CapeString s)
{
  double ret = .0;
  
  switch (cape_str_to_f__seek (s))
  {
    case '.':
    {
      // remove all ','
      CapeString h = cape_str_cp_replaced (s, ",", "");
      
      ret = strtod (h, NULL);
      
      cape_str_del (&h);
      
      break;
    }
    case ',':
    {
      // remove all '.'
      CapeString h = cape_str_cp_replaced (s, ".", "");
      
      cape_str_replace (&h, ",", ".");
      
      ret = strtod (h, NULL);
      
      cape_str_del (&h);
      
      break;
    }
    default:
    {
      ret = strtol (s, NULL, 10);
      
      break;
    }
  }
  
  return ret;
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

CapeString cape_str_random_s (number_t len)
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

CapeString cape_str_random_n (number_t len)
{
  number_t i;
  CapeString self = CAPE_ALLOC (len + 1);
  
  for (i = 0; i < len; i++)
  {
    self[i] = (rand() % 10) + 48;
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
        // corrupt UTF-8 character
        if (i == 1)
        {
          clen = 1;
        }
        else
        {
          clen = i - 1;
        }

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
  number_t src_len = strlen (source);
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

  // add termination
  *ret = 0;
  
  return ret_src;
}

//-----------------------------------------------------------------------------

CapeString cape_str_to_word (const CapeString source)
{
  static unsigned char char_utf8_ascii [256] = {
    0,        // (000)
    0,        // (001)
    0,        // (002)
    0,        // (003)
    0,        // (004)
    0,        // (005)
    0,        // (006)
    0,        // (007)
    0,        // (008)
    0,        // (009)
    0,        // (010)
    0,        // (011)
    0,        // (012)
    0,        // (013)
    0,        // (014)
    0,        // (015)
    0,        // (016)
    0,        // (017)
    0,        // (018)
    0,        // (019)
    0,        // (020)
    0,        // (021)
    0,        // (022)
    0,        // (023)
    0,        // (024)
    0,        // (025)
    0,        // (026)
    0,        // (027)
    0,        // (028)
    0,        // (029)
    0,        // (030)
    0,        // (031)
    32,       // (032)
    33,       // (033) !
    34,       // (034) "
    35,       // (035) #
    36,       // (036) $
    37,       // (037) %
    38,       // (038) &
    39,       // (039) '
    40,       // (040) (
    41,       // (041) )
    42,       // (042) *
    43,       // (043) +
    44,       // (044) ,
    45,       // (045) -
    46,       // (046) .
    47,       // (047) /
    48,       // (048) 0
    49,       // (049) 1
    50,       // (050) 2
    51,       // (051) 3
    52,       // (052) 4
    53,       // (053) 5
    54,       // (054) 6
    55,       // (055) 7
    56,       // (056) 8
    57,       // (057) 9
    58,       // (058) :
    59,       // (059) ;
    60,       // (060) <
    61,       // (061) =
    62,       // (062) >
    63,       // (063) ?
    64,       // (064) @
    65,       // (065) A
    66,       // (066) B
    67,       // (067) C
    68,       // (068) D
    69,       // (069) E
    70,       // (070) F
    71,       // (071) G
    72,       // (072) H
    73,       // (073) I
    74,       // (074) J
    75,       // (075) K
    76,       // (076) L
    77,       // (077) M
    78,       // (078) N
    79,       // (079) O
    80,       // (080) P
    81,       // (081) Q
    82,       // (082) R
    83,       // (083) S
    84,       // (084) T
    85,       // (085) U
    86,       // (086) V
    87,       // (087) W
    88,       // (088) X
    89,       // (089) Y
    90,       // (090) Z
    91,       // (091) [
    92,       // (092)
    93,       // (093) ]
    94,       // (094) ^
    95,       // (095) _
    96,       // (096) `
    97,       // (097) a
    98,       // (098) b
    99,       // (099) c
    100,      // (100) d
    101,      // (101) e
    102,      // (102) f
    103,      // (103) g
    104,      // (104) h
    105,      // (105) i
    106,      // (106) j
    107,      // (107) k
    108,      // (108) l
    109,      // (109) m
    110,      // (110) n
    111,      // (111) o
    112,      // (112) p
    113,      // (113) q
    114,      // (114) r
    115,      // (115) s
    116,      // (116) t
    117,      // (117) u
    118,      // (118) v
    119,      // (119) w
    120,      // (120) x
    121,      // (121) y
    122,      // (122) z
    123,      // (123) {
    124,      // (124) |
    125,      // (125) }
    126,      // (126) ~
    0,        // (127)
    128,      // (128) €
    0,        // (129) ?
    0,        // (130) ?
    0,        // (131) ?
    0,        // (132) ?
    0,        // (133) ?
    0,        // (134) ?
    0,        // (135) ?
    0,        // (136) ?
    0,        // (137) ?
    138,      // (138) Š
    0,        // (139) ?
    140,      // (140) Œ
    0,        // (141) ?
    142,      // (142) Ž
    0,        // (143) ?
    0,        // (144) ?
    0,        // (145) ?
    0,        // (146) ?
    0,        // (147) ?
    0,        // (148) ?
    0,        // (149) ?
    0,        // (150) ?
    0,        // (151) ?
    0,        // (152) ?
    0,        // (153) ?
    154,      // (154) š
    0,        // (155) ?
    156,      // (156) œ
    0,        // (157) ?
    158,      // (158) ž
    159,      // (159) Ÿ
    0,        // (160) ?
    0,        // (161) ?
    0,        // (162) ?
    0,        // (163) ?
    0,        // (164) ?
    0,        // (165) ?
    0,        // (166) ?
    0,        // (167) ?
    0,        // (168) ?
    169,      // (169) ©
    0,        // (170) ?
    0,        // (171) ?
    0,        // (172) ?
    0,        // (173) ?
    0,        // (174) ?
    0,        // (175) ?
    0,        // (176) ?
    0,        // (177) ?
    0,        // (178) ?
    0,        // (179) ?
    0,        // (180) ?
    0,        // (181) ?
    0,        // (182) ?
    0,        // (183) ?
    0,        // (184) ?
    0,        // (185) ?
    0,        // (186) ?
    0,        // (187) ?
    0,        // (188) ?
    0,        // (189) ?
    0,        // (190) ?
    0,        // (191) ?
    192,      // (192) À
    193,      // (193) Á
    194,      // (194) Â
    195,      // (195) Ã
    196,      // (196) Ä
    197,      // (197) Å
    198,      // (198) Æ
    199,      // (199) Ç
    200,      // (200) È
    201,      // (201) É
    202,      // (202) Ê
    203,      // (203) Ë
    204,      // (204) Ì
    205,      // (205) Í
    206,      // (206) Î
    207,      // (207) Ï
    208,      // (208) Ð
    209,      // (209) Ñ
    210,      // (210) Ò
    211,      // (211) Ó
    212,      // (212) Ô
    213,      // (213) Õ
    214,      // (214) Ö
    215,      // (215) ×
    216,      // (216) Ø
    217,      // (217) Ù
    218,      // (218) Ú
    219,      // (219) Û
    220,      // (220) Ü
    221,      // (221) Ý
    222,      // (222) Þ
    223,      // (223) ß
    224,      // (224) à
    225,      // (225) á
    226,      // (226) â
    227,      // (227) ã
    228,      // (228) ä
    229,      // (229) å
    230,      // (230) æ
    231,      // (231) ç
    232,      // (232) è
    233,      // (233) é
    234,      // (234) ê
    235,      // (235) ë
    236,      // (236) ì
    237,      // (237) í
    238,      // (238) î
    239,      // (239) ï
    240,      // (240) ð
    241,      // (241) ñ
    242,      // (242) ò
    243,      // (243) ó
    244,      // (244) ô
    245,      // (245) õ
    246,      // (246) ö
    247,      // (247) ÷
    248,      // (248) ø
    249,      // (249) ù
    250,      // (250) ú
    251,      // (251) û
    252,      // (252) ü
    253,      // (253) ý
    254,      // (254) þ
    255       // (255) ÿ
  };
  
  number_t src_len = strlen (source);
  char* ret_src = CAPE_ALLOC (src_len + 1);
  
  char* ret_pos = ret_src;
  const unsigned char* src_pos = (const unsigned char*)source;

  number_t i;
  
  for (i = 0; i < src_len; i++)
  {
    unsigned char c = *src_pos;
    
    if (0x20 <= c && c <= 0x7E)
    {
      // check for char in the UTF8 ASCII table
      // -> if the char is "accepted != 0"
      // -> it seems to be no special character
      c = char_utf8_ascii [c];
      if (c)
      {
        *ret_pos = c;
        ret_pos++;
      }

      src_pos++;
    }
    else if ((c & 0xE0) == 0xC0)
    {
      i += 1;

      // check LATIN characters only
      // -> we can accept all on a specific segment
      switch (c)
      {
        case 0xC3:
        case 0xC4:
        case 0xC5:
        case 0xC6:
        case 0xC7:
        case 0xC8:
        case 0xC9:
        {
          if (i < src_len)
          {
            memcpy (ret_pos, src_pos, 2);
            ret_pos += 2;
          }
          
          break;
        }
      }

      src_pos += 2;
    }
    else if ((c & 0xF0) == 0xE0)
    {
      // skip everything here
      
      src_pos += 3;
      i += 2;
    }
    else if ((c & 0xF8) == 0xF0)
    {
      // skip everything here

      src_pos += 4;
      i += 3;
    }
    else if ((c & 0xFC) == 0xF8)
    {
      // skip everything here

      src_pos += 5;
      i += 4;
    }
    else if ((c & 0xFE) == 0xFC)
    {
      // skip everything here

      src_pos += 6;
      i += 5;
    }
  }
  
  // add termination
  *ret_pos = 0;
  
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
  if (self)
  {
    char* pos = self;
    
    while (*pos)
    {
      *pos = toupper (*pos);
      pos++;
    }
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

number_t cape_str_wchar_utf8 (wchar_t wc, char* bufdat)
{
  if ( 0 <= wc && wc <= 0x7f )
  {
    bufdat[0] = wc;

    return 1;
  }
  else if ( 0x80 <= wc && wc <= 0x7ff )
  {
    bufdat[0] = 0xc0 | (wc >> 6);
    bufdat[1] = 0x80 | (wc & 0x3f);
    
    return 2;
  }
  else if ( 0x800 <= wc && wc <= 0xffff )
  {
    bufdat[0] = 0xe0 | (wc >> 12);
    bufdat[1] = 0x80 | ((wc >> 6) & 0x3f);
    bufdat[2] = 0x80 | (wc & 0x3f);
    
    return 3;
  }
  else if ( 0x10000 <= wc && wc <= 0x1fffff )
  {
    bufdat[0] = 0xf0 | (wc >> 18);
    bufdat[1] = 0x80 | ((wc >> 12) & 0x3f);
    bufdat[2] = 0x80 | ((wc >> 6) & 0x3f);
    bufdat[3] = 0x80 | (wc & 0x3f);
    
    return 4;
  }
  else if ( 0x200000 <= wc && wc <= 0x3ffffff )
  {
    bufdat[0] = 0xf8 | (wc >> 24);
    bufdat[1] = 0x80 | ((wc >> 18) & 0x3f);
    bufdat[2] = 0x80 | ((wc >> 12) & 0x3f);
    bufdat[3] = 0x80 | ((wc >> 6) & 0x3f);
    bufdat[4] = 0x80 | (wc & 0x3f);

    return 5;
  }
  else if ( 0x4000000 <= wc && wc <= 0x7fffffff )
  {
    bufdat[0] = 0xfc | (wc >> 30);
    bufdat[1] = 0x80 | ((wc >> 24) & 0x3f);
    bufdat[2] = 0x80 | ((wc >> 18) & 0x3f);
    bufdat[3] = 0x80 | ((wc >> 12) & 0x3f);
    bufdat[4] = 0x80 | ((wc >> 6) & 0x3f);
    bufdat[5] = 0x80 | (wc & 0x3f);

    return 6;
  }
  
  return 0;
}

//-----------------------------------------------------------------------------

char* cape_str__search_mget (char* d, number_t row_len, number_t i_t, number_t j_s)
{
  return d + i_t * row_len + j_s;
}

//-----------------------------------------------------------------------------

#define MIN(a,b) (((a)<(b))?(a):(b))

//-----------------------------------------------------------------------------

number_t cape_str_distance (const CapeString s1, number_t l1, const CapeString s2, number_t l2)
{
  number_t ret = l1;
  int i_t, j_s;
  
  number_t h = l1 + 1;
  number_t w = l2 + 1;
  
  number_t n = w * h;
  
  // allocate memory for matrix
  char* d = CAPE_ALLOC (n);
  
  // initialize matrix
  memset (d, 0, n);
  
  for (i_t = 0; i_t < h; i_t++)
  {
    char* c = cape_str__search_mget (d, w, i_t, 0);
    *c = i_t;
  }
  
  for (j_s = 0; j_s < w; j_s++)
  {
    char* c = cape_str__search_mget (d, w, 0, j_s);
    *c = j_s;
  }
  
  number_t cost = 0;
  
  for (i_t = 1; i_t < h; i_t++)
  {
    for (j_s = 1; j_s < w; j_s++)
    {
      char* c0 = cape_str__search_mget (d, w, i_t, j_s);
      char* c1 = cape_str__search_mget (d, w, i_t - 1, j_s);
      char* c2 = cape_str__search_mget (d, w, i_t, j_s - 1);
      char* c3 = cape_str__search_mget (d, w, i_t - 1, j_s - 1);
      
      const char* a = s1 + i_t - 1;
      const char* b = s2 + j_s - 1;
      
      if (*a == *b)
      {
        cost = 0;
      }
      else
      {
        cost = 1;
      }
      
      *c0 = MIN (*c1 + 1, MIN (*c2 + 1, *c3 + cost));
      
      if (i_t > 1 && j_s > 1)
      {
        const char* a2 = s1 + i_t - 2;
        const char* b2 = s2 + j_s - 1;
        
        if (*a == *b2 && *a2 == *b)
        {
          char* c4 = cape_str__search_mget (d, w, i_t - 2, j_s - 2);
          
          *c0 = MIN (*c0, *c4 + 1);
        }
      }
    }
  }
  
  {
    char* c = cape_str__search_mget (d, w, h - 1, w - 1);
    ret = *c;
  }
  
  CAPE_FREE (d);
  
  return ret;
}

//-----------------------------------------------------------------------------
