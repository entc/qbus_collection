#include "cape_stream.h"
#include "fmt/cape_dragon4.h"
#include "sys/cape_types.h"
#include "sys/cape_file.h"

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

  CapeString mime_type;
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

  self->mime_type = NULL;

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
    cape_str_del (&(self->mime_type));
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

CapeStream cape_stream_from_buf (const char* bufdat, number_t buflen)
{
  CapeStream self = CAPE_NEW (struct CapeStream_s);

  // allocate memory and set the size
  self->buffer = CAPE_ALLOC (buflen + 1);
  self->size = buflen;

  // copy the data
  memcpy (self->buffer, bufdat, buflen);

  // set the new position
  self->pos = self->buffer + buflen;

  // no mime type
  self->mime_type = NULL;

  return self;
}

//-----------------------------------------------------------------------------

CapeStream cape_stream_sub (CapeStream self, number_t start, number_t length, int overflow)
{
  CapeStream ret = NULL;

  // do some checks
  if (start < (self->pos - self->buffer))
  {
    char* pos_start = self->buffer + start;
    number_t maxlen = self->pos - pos_start;

    if (length > maxlen)
    {
      if (overflow)
      {
        // allow if overflow was enabled
        // use the maximal length possible
        ret = cape_stream_from_buf (pos_start, maxlen);
      }
    }
    else
    {
      ret = cape_stream_from_buf (pos_start, length);
    }
  }

  return ret;
}

//-----------------------------------------------------------------------------

void cape_stream_replace_mv (CapeStream* p_self, CapeStream* p_source)
{
  // free the old stream
  cape_stream_del (p_self);
  
  // transfer ownership
  *p_self = *p_source;
  
  // release ownership
  *p_source = NULL;
}

//-----------------------------------------------------------------------------

void cape_stream_mime_set (CapeStream self, const CapeString mime)
{
  cape_str_replace_cp (&(self->mime_type), mime);
}

//-----------------------------------------------------------------------------

const CapeString cape_stream_mime_get (CapeStream self)
{
  return self->mime_type;
}

//-----------------------------------------------------------------------------

CapeString cape_stream_serialize (CapeStream self, fct_cape_stream_base64_encode cb_encode)
{
  if (cb_encode)
  {
    CapeString h = cb_encode (self);
    if (h)
    {
      CapeStream s = cape_stream_new ();

      cape_stream_append_str (s, "data:");
      cape_stream_append_str (s, self->mime_type ? self->mime_type : "application/octet-stream");
      cape_stream_append_str (s, ";base64,");

      cape_stream_append_str (s, h);
      return cape_stream_to_str (&s);
    }
  }

  return NULL;
}

//-----------------------------------------------------------------------------

CapeStream cape_stream_deserialize (CapeString source, fct_cape_stream_base64_decode cb_decode)
{
  number_t pos;

  if (cb_decode && cape_str_begins (source, "data:") && cape_str_find (source + 5, ";base64,", &pos))
  {
    CapeStream s = cb_decode (source + pos + 8);
    if (s)
    {
      s->mime_type = cape_str_sub (source + 5, pos);
      return s;
    }
  }

  return NULL;
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

void cape_stream_dec (CapeStream self, number_t bytes_reverted)
{
  self->pos -= bytes_reverted;
  
  if (self->pos < self->buffer)
  {
    self->pos = self->buffer;
  }
}

//-----------------------------------------------------------------------------

char* cape_stream_pos (CapeStream self)
{
  return self->pos;
}

//-----------------------------------------------------------------------------

number_t cape_stream_a_pos (CapeStream self)
{
  return (self->pos - self->buffer);
}

//-----------------------------------------------------------------------------

char* cape_stream_pos_a (CapeStream self, number_t absolute_position)
{
  return (self->buffer + absolute_position);
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

number_t cape_stream_append_buf (CapeStream self, const char* buffer, number_t size)
{
  if (size > 0)
  {
    cape_stream_reserve (self, size);

    memcpy (self->pos, buffer, size);
    self->pos += size;
  }

  return size;
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

void cape_stream_append_c_series (CapeStream self, char c, number_t amount)
{
  if (amount > 0)
  {
    cape_stream_reserve (self, amount);
    
    memset (self->pos, c, amount);
    self->pos += amount;
  }
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
  //cape_dragon4_scientific (dragon4, CAPE_DRAGON4__DMODE_EXACT, 16, FALSE, CAPE_DRAGON4__TMODE_ONE_ZERO, 0, 4);

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
    CapeString h = cape_datetime_s__std_msec (val);

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

int cape_stream_to_file (CapeStream self, const CapeString file, CapeErr err)
{
  int res;
  
  // local objects
  CapeFileHandle fh = cape_fh_new (NULL, file);
  
  // try to create a new file
  res = cape_fh_open (fh, O_CREAT | O_TRUNC | O_WRONLY, err);
  if (res)
  {
    goto exit_and_cleanup;
  }
  
  // write the content of the stream into a file
  {
    number_t bytes_to_write = self->pos - self->buffer;
    number_t bytes_written = 0;
      
    while (bytes_written < bytes_to_write)
    {
      number_t bytes_written_part = cape_fh_write_buf (fh, self->buffer + bytes_written, bytes_to_write - bytes_written);
      if (bytes_written_part == 0)
      {
        // some error happened
        res = cape_err_lastOSError (err);
        goto exit_and_cleanup;
      }
      
      bytes_written = bytes_written + bytes_written_part;
    }
  }
  
  res = CAPE_ERR_NONE;
  
exit_and_cleanup:

  cape_fh_del (&fh);
  return res;
}

//-----------------------------------------------------------------------------
