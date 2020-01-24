#include "cape_cursor.h"

// c includes
#include <stdio.h>

#if defined __WINDOWS_OS
#include <winsock.h>
#else
#include <netinet/in.h>
#endif

#ifndef ntohll
#define ntohll(x) ((1==ntohl(1)) ? (x) : (((cape_uint64)ntohl((x) & 0xFFFFFFFFUL)) << 32) | ntohl((cape_uint32)((x) >> 32)))
#endif

//-----------------------------------------------------------------------------

struct CapeCursor_s
{
  number_t size;
  
  const char* buffer;
  const char* pos;
};

//-----------------------------------------------------------------------------

CapeCursor cape_cursor_new ()
{
  CapeCursor self = CAPE_NEW(struct CapeCursor_s);
  
  self->size = 0;
  self->buffer = NULL;
  self->pos = self->buffer;
  
  return self;
}

//-----------------------------------------------------------------------------

void cape_cursor_del (CapeCursor* p_self)
{
  if (*p_self)
  {
    CapeCursor self = *p_self;
    
    CAPE_DEL (p_self, struct CapeCursor_s);
  }
}

//-----------------------------------------------------------------------------

int cape_cursor__has_data (CapeCursor self, number_t len)
{
  return (len <= cape_cursor_tail (self));
}

//-----------------------------------------------------------------------------

number_t cape_cursor_tail (CapeCursor self)
{
  return self->buffer + self->size - self->pos;
}

//-----------------------------------------------------------------------------

const char* cape_cursor_dpos (CapeCursor self)
{
  return self->pos;
}

//-----------------------------------------------------------------------------

void cape_cursor_set (CapeCursor self, const char* bufdat, number_t buflen)
{
  self->buffer = bufdat;
  self->size = buflen;
  self->pos = self->buffer;
}

//-----------------------------------------------------------------------------

number_t cape_cursor_size (CapeCursor self)
{
  return self->size;
}

//-----------------------------------------------------------------------------

const char* cape_cursor_data (CapeCursor self)
{
  return self->buffer;
}

//-----------------------------------------------------------------------------

char* cape_cursor_scan_s (CapeCursor self, number_t len)
{
  if (cape_cursor__has_data (self, 2))
  {
    char* h = cape_str_sub (self->pos, len);
    
    self->pos += len;
    
    return h;
  }
  else
  {
    return NULL;
  }
}

//-----------------------------------------------------------------------------

cape_uint8 cape_cursor_scan_08 (CapeCursor self)
{
  cape_uint8 ret = 0;
  
  if (cape_cursor__has_data (self, 1))
  {
    ret = *(self->pos);
    self->pos += 1;
  }
  
  return ret;
}

//-----------------------------------------------------------------------------

cape_uint16 cape_cursor_scan_16 (CapeCursor self, int network_byte_order)
{
  cape_uint16 ret = 0;
  
  if (cape_cursor__has_data (self, 2))
  {
    if (network_byte_order)
    {
      ret = ntohs (*((cape_uint16*)(self->pos)));
    }
    else
    {
      ret = *((cape_uint16*)(self->pos));
    }
    
    self->pos += 2;
  }
  
  return ret;
}

//-----------------------------------------------------------------------------

cape_uint32 cape_cursor_scan_32 (CapeCursor self, int network_byte_order)
{
  cape_uint32 ret = 0;
  
  if (cape_cursor__has_data (self, 4))
  {
    if (network_byte_order)
    {
      ret = ntohl (*((cape_uint32*)(self->pos)));
    }
    else
    {
      ret = *((cape_uint32*)(self->pos));
    }
    
    self->pos += 4;
  }
  
  return ret;
}

//-----------------------------------------------------------------------------

cape_uint64 cape_cursor_scan_64 (CapeCursor self, int network_byte_order)
{
  cape_uint64 ret = 0;
  
  if (cape_cursor__has_data (self, 8))
  {
    if (network_byte_order)
    {  
      ret = ntohll (*((cape_uint64*)(self->pos)));
    }
    else
    {
      ret = *((cape_uint64*)(self->pos));
    }
    
    self->pos += 8;
  }
  
  return ret;
}

//-----------------------------------------------------------------------------

double cape_cursor_scan_bd (CapeCursor self, int network_byte_order)
{
  double ret = 0;
  
  if (cape_cursor__has_data (self, 8))
  {
    cape_uint64 h;
    
    if (network_byte_order)
    {
      h = ntohll (*((cape_uint64*)(self->pos)));
    }
    else
    {
      h = *((cape_uint64*)(self->pos));
    }
    
    memcpy (&ret, &h, 8);
    
    self->pos += 8;
  }
  
  return ret;
}

//-----------------------------------------------------------------------------
