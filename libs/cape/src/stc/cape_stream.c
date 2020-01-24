#include "cape_stream.h"
#include "fmt/cape_dragon4.h"
#include "sys/cape_types.h"

// c includes
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <memory.h>
#include <limits.h>
#include <stdlib.h>

#if defined __WINDOWS_OS
#include <winsock.h>
#else
#include <netinet/in.h>
#endif

#ifndef htonll
#define htonll(x) ((1==htonl(1)) ? (x) : (((cape_uint64)htonl((x) & 0xFFFFFFFFUL)) << 32) | htonl((cape_uint32)((x) >> 32)))
#endif

//-----------------------------------------------------------------------------

struct CapeStream_s
{
  number_t size;
  
  char* buffer;
  
  char* pos;
  
};

//-----------------------------------------------------------------------------

number_t cape_stream_size (CapeStream self)
{
  return self->pos - self->buffer;
}

//-----------------------------------------------------------------------------

void cape_stream_allocate (CapeStream self, unsigned long amount)
{
  // safe how much we have used from the buffer
  unsigned long usedBytes = cape_stream_size (self);
  
  // use realloc to minimalize coping the buffer
  self->size += amount;
  self->buffer = realloc(self->buffer, self->size + 1);
  
  // reset the position
  self->pos = self->buffer + usedBytes;
}

//-----------------------------------------------------------------------------

void cape_stream_reserve (CapeStream self, number_t amount)
{
  number_t diffBytes = cape_stream_size (self) + amount + 1;
  
  if (diffBytes > self->size)
  {
    if (amount > self->size)
    {
      cape_stream_allocate (self, amount);
    }
    else
    {
      cape_stream_allocate (self, self->size + self->size);
    }
  }
}

//-----------------------------------------------------------------------------

CapeStream cape_stream_new ()
{
  CapeStream self = CAPE_NEW(struct CapeStream_s);
  
  self->size = 0;
  self->buffer = 0;
  self->pos = self->buffer;
  
  // initial alloc
  cape_stream_allocate (self, 100);
  
  // clean
  cape_stream_clr (self);
  
  return self;
}

//-----------------------------------------------------------------------------

void cape_stream_del (CapeStream* pself)
{
  CapeStream self = *pself;
  
  if (self)
  {
    free (self->buffer);
    
    CAPE_DEL (pself, struct CapeStream_s);
  }  
}

//-----------------------------------------------------------------------------

CapeString cape_stream_to_str (CapeStream* pself)
{
  CapeStream self = *pself;  
  CapeString ret = self->buffer;
  
  // set terminator
  *(self->pos) = 0;
  
  CAPE_DEL(pself, struct CapeStream_s);
  
  return ret;
}

//-----------------------------------------------------------------------------

number_t cape_stream_to_n (CapeStream self)
{
  number_t ret;

  // prepare and add termination
  *(self->pos) = '\0';
  
  ret = strtol (self->buffer, NULL, 10);
    
  cape_stream_clr (self);
  
  return ret;
}

//-----------------------------------------------------------------------------

CapeString cape_stream_to_s (CapeStream self)
{
  CapeString ret = cape_str_sub (self->buffer, self->pos - self->buffer);
  
  cape_stream_clr (self);
  
  return ret;
}

//-----------------------------------------------------------------------------

void cape_stream_clr (CapeStream self)
{
  self->pos = self->buffer;
}

//-----------------------------------------------------------------------------

const char* cape_stream_get (CapeStream self)
{
  // set terminator
  *(self->pos) = 0;
  
  return self->buffer;
}

//-----------------------------------------------------------------------------

const char* cape_stream_data (CapeStream self)
{
  return self->buffer;
}

//-----------------------------------------------------------------------------

void cape_stream_cap (CapeStream self, number_t bytes_reserve)
{
  cape_stream_reserve (self, bytes_reserve);
}
  
//-----------------------------------------------------------------------------

void cape_stream_set (CapeStream self, number_t bytes_appended)
{
  self->pos += bytes_appended;
}

//-----------------------------------------------------------------------------

char* cape_stream_pos (CapeStream self)
{
  return self->pos;
}

//-----------------------------------------------------------------------------

void cape_stream_append_str (CapeStream self, const char* s)
{
  if (s)
  {
    // need to find the length
    cape_stream_append_buf (self, s, strlen(s));
  }
}

//-----------------------------------------------------------------------------

void cape_stream_append_buf (CapeStream self, const char* buffer, unsigned long size)
{
  if (size > 0)
  {
    cape_stream_reserve (self, size);
    
    memcpy (self->pos, buffer, size);
    self->pos += size;
  }
}

//-----------------------------------------------------------------------------

void cape_stream_append_fmt (CapeStream self, const char* format, ...)
{
  // variables
  va_list valist;
  va_start (valist, format);
  
  #ifdef _MSC_VER
  
  {
    int len = _vscprintf (format, valist) + 1;
    
    cape_stream_reserve (self, len);
    
    len = vsprintf_s (self->pos, len, format, valist);
    
    self->pos += len;
  }
  
  #elif _GCC
  
  {
    char* strp;
    
    int bytesWritten = vasprintf (&strp, format, valist);
    if ((bytesWritten > 0) && strp)
    {
      cape_stream_append_buf (self, strp, bytesWritten);
      free(strp);
    }
  }
  
  #elif __BORLANDC__
  
  {
    int len = 1024;
    
    cape_stream_reserve (self, len);
    
    len = vsnprintf (self->pos, len, format, valist);
    
    self->pos += len;
  }
  
  #endif
  
  va_end(valist);
}

//-----------------------------------------------------------------------------

void cape_stream_append_c (CapeStream self, char c)
{
  cape_stream_reserve (self, 1);
  
  *(self->pos) = c;
  self->pos++;
}

//-----------------------------------------------------------------------------

void cape_stream_append_n (CapeStream self, number_t val)
{
  cape_stream_reserve (self, 26);  // for very long intergers
  
#ifdef _MSC_VER
  
  self->pos += _snprintf_s (self->pos, 24, _TRUNCATE, "%li", val);
  
#else
  
  self->pos += snprintf(self->pos, 24, "%li", val);
  
#endif
}

//-----------------------------------------------------------------------------

void cape_stream_append_f (CapeStream self, double val)
{
  int res;
  CapeErr err = cape_err_new ();
  
  CapeDragon4 dragon4 = cape_dragon4_new ();
  
  cape_dragon4_positional (dragon4, CAPE_DRAGON4__DMODE_UNIQUE, CAPE_DRAGON4__CMODE_TOTAL, -1, FALSE, CAPE_DRAGON4__TMODE_ONE_ZERO, 0, 0);
  
  cape_stream_reserve (self, 1024);  // for very long intergers

  res = cape_dragon4_run (dragon4, self->pos, 1024, val, err);
  if (res)
  {
    
  }
  else
  {
  }

  self->pos += cape_dragon4_len (dragon4);

  cape_dragon4_del (&dragon4);
  
  cape_err_del (&err);
}

//-----------------------------------------------------------------------------

void cape_stream_append_d (CapeStream self, const CapeDatetime* val)
{
  if (val)
  {
    CapeString h = cape_datetime_s__std (val);
    
    cape_stream_append_buf (self, h, cape_str_size (h));
    
    cape_str_del (&h);
  }
}

//-----------------------------------------------------------------------------

void cape_stream_append_stream (CapeStream self, CapeStream stream)
{
  unsigned long usedBytes = stream->pos - stream->buffer;
  
  cape_stream_reserve (self, usedBytes);
  
  memcpy (self->pos, stream->buffer, usedBytes);
  self->pos += usedBytes;
}

//-----------------------------------------------------------------------------

void cape_stream_append_08 (CapeStream self, cape_uint8 val)
{
  cape_stream_reserve (self, 1);
  
  *(self->pos) = val;
  self->pos++;
}

//-----------------------------------------------------------------------------

void cape_stream_append_16 (CapeStream self, cape_uint16 val, int network_byte_order)
{
  cape_stream_reserve (self, 2);

  if (network_byte_order)
  {
    *((cape_uint16*)(self->pos)) = htons (val);
  }
  else
  {
    *((cape_uint16*)(self->pos)) = val;
  }
  
  self->pos += 2;
}

//-----------------------------------------------------------------------------

void cape_stream_append_32 (CapeStream self, cape_uint32 val, int network_byte_order)
{
  cape_stream_reserve (self, 4);
  
  if (network_byte_order)
  {
    *((cape_uint32*)(self->pos)) = htonl (val);
  }
  else
  {
    *((cape_uint32*)(self->pos)) = val;
  }
  
  self->pos += 4;
}

//-----------------------------------------------------------------------------

void cape_stream_append_64 (CapeStream self, cape_uint64 val, int network_byte_order)
{
  cape_stream_reserve (self, 8);
  
  if (network_byte_order)
  {
    *((cape_uint64*)(self->pos)) = htonll (val);
  }
  else
  {
    *((cape_uint64*)(self->pos)) = val;
  }
  
  self->pos += 8;
}

//-----------------------------------------------------------------------------

void cape_stream_append_bd (CapeStream self, double val, int network_byte_order)
{
  cape_uint64 h;

  cape_stream_reserve (self, 8);
    
  memcpy (&h, &val, 8);
  
  if (network_byte_order)
  {    
    *((cape_uint64*)(self->pos)) = htonll (h);
  }
  else
  {
    *((cape_uint64*)(self->pos)) = h;
  }
  
  self->pos += 8;
}

//-----------------------------------------------------------------------------
