#include "cape_json.h"

// cape includes
#include "fmt/cape_parser_json.h"
#include "sys/cape_log.h"
#include "stc/cape_stream.h"
#include "sys/cape_file.h"

// c includes
#include <wchar.h>
#include <stdio.h>

//-----------------------------------------------------------------------------------------------------------

static void __STDCALL cape_json_onItem (void* ptr, void* obj, int type, void* val, const char* key, int index)
{
  switch (type)
  {
    case CAPE_JPARSER_OBJECT_NODE:
    case CAPE_JPARSER_OBJECT_LIST:
    {
      CapeUdc h = val;
      
      cape_udc_add_name (obj, &h, key);
      break;
    }
    case CAPE_JPARSER_OBJECT_TEXT:
    {
      CapeStream s = cape_stream_deserialize (val, ptr);
      
      if (s)
      {
        CapeUdc h = cape_udc_new (CAPE_UDC_STREAM, key);
        
        cape_udc_set_m_mv (h, &s);
        
        cape_udc_add (obj, &h);
      }
      else
      {
        CapeUdc h = cape_udc_new (CAPE_UDC_STRING, key);
        
        cape_udc_set_s_cp (h, val);
        
        cape_udc_add (obj, &h);
      }
      
      break;
    }
    case CAPE_JPARSER_OBJECT_NUMBER:
    {
      CapeUdc h = cape_udc_new (CAPE_UDC_NUMBER, key);
      
      number_t* dat = val;
      cape_udc_set_n (h, *dat);
      
      cape_udc_add (obj, &h);
      break;
    }
    case CAPE_JPARSER_OBJECT_FLOAT:
    {
      CapeUdc h = cape_udc_new (CAPE_UDC_FLOAT, key);
      
      double* dat = val;
      cape_udc_set_f (h, *dat);
      
      cape_udc_add (obj, &h);
      break;
    }
    case CAPE_JPARSER_OBJECT_BOLEAN:
    {
      CapeUdc h = cape_udc_new (CAPE_UDC_BOOL, key);
      
      long dat = (long)val;
      cape_udc_set_b (h, (int)dat);
      
      cape_udc_add (obj, &h);
      break;
    }
    case CAPE_JPARSER_OBJECT_DATETIME:
    {
      CapeUdc h = cape_udc_new (CAPE_UDC_DATETIME, key);

      cape_udc_set_d (h, val);
      
      cape_udc_add (obj, &h);
      break;
    }
    case CAPE_JPARSER_OBJECT_NULL:
    {
      break;
    }
  }
}

//-----------------------------------------------------------------------------------------------------------

static void* __STDCALL cape_json_onObjCreate (void* ptr, int type)
{
  switch (type)
  {
    case CAPE_JPARSER_OBJECT_NODE:
    {
      return cape_udc_new (CAPE_UDC_NODE, NULL);
    }
    case CAPE_JPARSER_OBJECT_LIST:
    {
      return cape_udc_new (CAPE_UDC_LIST, NULL);
    }
  }
  
  return NULL;
}

//-----------------------------------------------------------------------------------------------------------

static void __STDCALL cape_json_onObjDestroy (void* ptr, void* obj)
{
  CapeUdc h = obj; cape_udc_del (&h);
}

//-----------------------------------------------------------------------------

CapeUdc cape_json_from_buf (const char* buffer, number_t size, fct_cape_stream_base64_decode cb_decode)
{
  CapeUdc ret = NULL;
  int res;
  
  CapeErr err = cape_err_new ();
  
  // create a new parser for the json format
  CapeParserJson parser_json = cape_parser_json_new (cb_decode, cape_json_onItem, cape_json_onObjCreate, cape_json_onObjDestroy);
 
  // try to parse the current buffer
  res = cape_parser_json_process (parser_json, buffer, size, err);
  if (res)
  {
    //printf ("ERROR JSON PARSER: %s\n", cape_err_text(err));
  }
  else
  {
    ret = cape_parser_json_object (parser_json);
    
    if (ret)
    {
      // set name
      // cape_udc_set_name (ret, name);
    }
    else
    {
      //eclog_msg (LL_WARN, "JSON", "reader", "returned NULL object as node");
    }
  }
  
  // clean up
  cape_parser_json_del (&parser_json);
  
  cape_err_del (&err);
  
  return ret;
}

//-----------------------------------------------------------------------------

CapeUdc cape_json_from_s (const CapeString source)
{
  if (source)
  {
    return cape_json_from_buf (source, cape_str_size (source), NULL);
  }
  else
  {
    return NULL;
  }
}

//-----------------------------------------------------------------------------------------------------------

void cape_json_escape (CapeStream stream, const CapeString source)
{
  if (source)
  {
    const unsigned char* c = (const unsigned char*)source;
    for (; *c; c++)
    {
      switch (*c)
      {
        case '"':
        {
          cape_stream_append_c (stream, '\\');
          cape_stream_append_c (stream, '"');
          break;
        }
        case '\\':
        {
          cape_stream_append_c (stream, '\\');
          cape_stream_append_c (stream, '\\');
          break;
        }
        /*
         *        case '/':
         *        {
         *          ecstream_append_c (stream, '\\');
         *          ecstream_append_c (stream, '/');
         *          break;
      }
      */
        case '\r':
        {
          cape_stream_append_c (stream, '\\');
          cape_stream_append_c (stream, 'r');
          break;
        }
        case '\n':
        {
          cape_stream_append_c (stream, '\\');
          cape_stream_append_c (stream, 'n');
          break;
        }
        case '\t':
        {
          cape_stream_append_c (stream, '\\');
          cape_stream_append_c (stream, 't');
          break;
        }
        case '\b':
        {
          cape_stream_append_c (stream, '\\');
          cape_stream_append_c (stream, 'b');
          break;
        }
        case '\f':
        {
          cape_stream_append_c (stream, '\\');
          cape_stream_append_c (stream, 'f');
          break;
        }
        default:
        {
          // implementation taken from https://gist.github.com/rechardchen/3321830
          // 
          if (0x20 <= *c && *c <= 0x7E) // )
          {
            //printf ("UTF8 [0] %c\n", *c);
            
            cape_stream_append_c (stream, *c);
          }
          else if ((*c & 0xE0) == 0xC0) 
          {
            char* buf = CAPE_ALLOC(10);
            
            // convert UTF8 into 4 hex digits
            wchar_t wc;
            
            wc = (c[0] & 0x1F) << 6;
            wc |= (c[1] & 0x3F);
            
#if defined _WIN64 || defined _WIN32
            sprintf_s (buf, 8, "\\u%.4x", wc);
#else
            // TODO: there might be a better way to do it
            sprintf (buf, "\\u%.4x", wc);
#endif
            
            cape_stream_append_buf (stream, buf, 6);
            
            c += 1;
            
            CAPE_FREE(buf);
          }
          else if ((*c & 0xF0) == 0xE0) 
          {
            int i;
            
            for (i = 1; i < 2; i++)
            {
              if (*(c + i) == 0)
              {
                break;
              }
            }
            
            if (i == 2)
            {
              char* buf = CAPE_ALLOC(10);
              wchar_t wc;
              
              wc = (c[0] & 0xF) << 12;
              wc |= (c[1] & 0x3F) << 6;
              wc |= (c[2] & 0x3F);
              
              c += 2;
              
              #if defined _WIN64 || defined _WIN32
              sprintf_s (buf, 8, "\\u%.4x", wc);
              #else
              // TODO: there might be a better way to do it
              sprintf (buf, "\\u%.4x", wc);
              #endif
              cape_stream_append_buf (stream, buf, 6);
              
              CAPE_FREE(buf);
            }
            else
            {
              c += 1;
              cape_stream_append_c (stream, '?');
            }            
          }
          else if ( (*c & 0xF8) == 0xF0 )
          {
            int i;
            
            for (i = 1; i < 3; i++)
            {
              if (*(c + i) == 0)
              {
                break;
              }
            }
            
            if (i == 3)
            {
              char* buf = CAPE_ALLOC(10);
              wchar_t wc;
              
              wc = (c[0] & 0x7) << 18;
              wc |= (c[1] & 0x3F) << 12;
              wc |= (c[2] & 0x3F) << 6;
              wc |= (c[3] & 0x3F);
              
              c += 3;
              
              #if defined _WIN64 || defined _WIN32
              sprintf_s (buf, 8, "\\u%.4x", wc);
              #else
              // TODO: there might be a better way to do it
              sprintf (buf, "\\u%.4x", wc);
              #endif
              
              cape_stream_append_buf (stream, buf, 6);
              
              CAPE_FREE(buf);
            }
            else
            {
              c += 1;
              cape_stream_append_c (stream, '?');
            }
          }
          else if ( (*c & 0xFC) == 0xF8 )
          {
            int i;
            
            for (i = 1; i < 4; i++)
            {
              if (*(c + i) == 0)
              {
                break;
              }
            }
            
            if (i == 4)
            {
              char* buf = CAPE_ALLOC(10);
              wchar_t wc;
              
              wc = (c[0] & 0x3) << 24;
              wc |= (c[1] & 0x3F) << 18;
              wc |= (c[2] & 0x3F) << 12;
              wc |= (c[3] & 0x3F) << 6;
              wc |= (c[4] & 0x3F);
              
              c += 4;
              
              #if defined _WIN64 || defined _WIN32
              sprintf_s (buf, 8, "\\u%.4x", wc);
              #else
              // TODO: there might be a better way to do it
              sprintf (buf, "\\u%.4x", wc);
              #endif
              
              cape_stream_append_buf (stream, buf, 6);
              
              CAPE_FREE(buf);
            }
            else
            {
              c += 1;
              cape_stream_append_c (stream, '?');
            }
          }
          else if ( (*c & 0xFE) == 0xFC )
          {
            int i;
            
            for (i = 1; i < 5; i++)
            {
              if (*(c + i) == 0)
              {
                break;
              }
            }
            
            if (i == 5)
            {
              char* buf = CAPE_ALLOC(10);
              wchar_t wc;
              
              wc = (c[0] & 0x1) << 30;
              wc |= (c[1] & 0x3F) << 24;
              wc |= (c[2] & 0x3F) << 18;
              wc |= (c[3] & 0x3F) << 12;
              wc |= (c[4] & 0x3F) << 6;
              wc |= (c[5] & 0x3F);
              
              c += 5;
              
              #if defined _WIN64 || defined _WIN32
              sprintf_s (buf, 8, "\\u%.4x", wc);
              #else
              // TODO: there might be a better way to do it
              sprintf (buf, "\\u%.4x", wc);
              #endif
              
              cape_stream_append_buf (stream, buf, 6);
              
              CAPE_FREE(buf);
            }
            else
            {
              c += 1;
              cape_stream_append_c (stream, '?');
            }
          }
          else
          {
            // not supported character
          }
        }
      }
    }
  }
  else
  {
    cape_stream_append_str (stream, "");
  }
}

#define INDENTATION_SPACES 2

//-----------------------------------------------------------------------------------------------------------

void cape_json_fill (CapeStream stream, const CapeUdc node, number_t max_bytes, fct_cape_stream_base64_encode cb_encode, int nice, number_t indentation_prev)
{
  number_t indentation = indentation_prev;
  
  switch (cape_udc_type (node))
  {
    case CAPE_UDC_LIST:
    {
      CapeUdcCursor* cursor = cape_udc_cursor_new (node, CAPE_DIRECTION_FORW);
      
      cape_stream_append_c (stream, '[');

      indentation += INDENTATION_SPACES;

      if (nice)
      {
        cape_stream_append_c (stream, '\n'); 
        cape_stream_append_c_series (stream, ' ', indentation);
      }
      
      while (cape_udc_cursor_next (cursor))
      {
        if (cursor->position)
        {
          cape_stream_append_c (stream, ',');

          if (nice)
          {
            cape_stream_append_c (stream, '\n');        
            cape_stream_append_c_series (stream, ' ', indentation);
          }
        }
        
        cape_json_fill (stream, cursor->item, max_bytes, cb_encode, nice, indentation);
      }

      indentation -= INDENTATION_SPACES;
      
      if (nice)
      {
        cape_stream_append_c (stream, '\n');        
        cape_stream_append_c_series (stream, ' ', indentation);
      }
      
      cape_stream_append_c (stream, ']');

      cape_udc_cursor_del (&cursor);

      break;
    }
    case CAPE_UDC_NODE:
    {
      CapeUdcCursor* cursor = cape_udc_cursor_new (node, CAPE_DIRECTION_FORW);
            
      cape_stream_append_c (stream, '{');
      
      indentation += INDENTATION_SPACES;

      if (nice)
      {
        cape_stream_append_c (stream, '\n'); 
        cape_stream_append_c_series (stream, ' ', indentation);
      }
      
      while (cape_udc_cursor_next (cursor))
      {
        if (cursor->position)
        {
          cape_stream_append_c (stream, ',');
          
          if (nice)
          {
            cape_stream_append_c (stream, '\n');        
            cape_stream_append_c_series (stream, ' ', indentation);
          }
        }
        
        cape_stream_append_c (stream, '"');
        
        cape_json_escape (stream, cape_udc_name (cursor->item));
        
        cape_stream_append_str (stream, "\":");

        cape_json_fill (stream, cursor->item, max_bytes, cb_encode, nice, indentation);
      }
      
      indentation -= INDENTATION_SPACES;
      
      if (nice)
      {
        cape_stream_append_c (stream, '\n');        
        cape_stream_append_c_series (stream, ' ', indentation);
      }
      
      cape_stream_append_c (stream, '}');
      
      cape_udc_cursor_del (&cursor);

      break;
    }
    case CAPE_UDC_STRING:
    {
      const CapeString h = cape_udc_s (node, NULL);

      cape_stream_append_c (stream, '"');

      if (max_bytes > 0)
      {
        number_t len = cape_str_size (h);
        if (len < max_bytes)
        {
          cape_json_escape (stream, h);
        }
        else
        {
          cape_json_escape (stream, "!OVERSIZE!");
        }
      }
      else
      {
        cape_json_escape (stream, h);
      }

      cape_stream_append_c (stream, '"');

      break;
    }
    case CAPE_UDC_BOOL:
    {
      cape_stream_append_str (stream, cape_udc_b (node, FALSE) ? "true" : "false");
      break;
    }
    case CAPE_UDC_NUMBER:
    {
      cape_stream_append_n (stream, cape_udc_n (node, 0));
      break;
    }
    case CAPE_UDC_FLOAT:
    {
      cape_stream_append_f (stream, cape_udc_f (node, 0));
      break;
    }
    case CAPE_UDC_DATETIME:
    {
      cape_stream_append_c (stream, '"');
      cape_stream_append_d (stream, cape_udc_d (node, NULL));
      cape_stream_append_c (stream, '"');

      break;
    }
    case CAPE_UDC_STREAM:
    {
      const CapeStream s = cape_udc_m (node);
      
      CapeString h = cape_stream_serialize (s, cb_encode);
      if (h)
      {
        cape_stream_append_c (stream, '"');
        cape_stream_append_str (stream, h);
        cape_stream_append_c (stream, '"');
      }
      else
      {
        cape_stream_append_c (stream, '"');
        cape_stream_append_c (stream, '"');
      }
      
      cape_str_del (&h);
      
      break;
    }
  }
}

//-----------------------------------------------------------------------------

CapeString __STDCALL cape_json_to_s__encode_stream (const CapeStream s)
{
  return cape_str_fmt ("BUF: %lu", cape_stream_size (s));
}

//-----------------------------------------------------------------------------

CapeString cape_json_to_s (const CapeUdc source)
{
  CapeStream stream = cape_stream_new ();
  
  cape_json_fill (stream, source, 0, cape_json_to_s__encode_stream, FALSE, 0);
  
  return cape_stream_to_str (&stream);
}

//-----------------------------------------------------------------------------------------------------------

CapeString cape_json_to_s__ex (const CapeUdc source, fct_cape_stream_base64_encode cb_encode)
{
  CapeStream stream = cape_stream_new ();
  
  cape_json_fill (stream, source, 0, cb_encode, FALSE, 0);
  
  return cape_stream_to_str (&stream);
}

//-----------------------------------------------------------------------------

CapeString cape_json_to_s_max (const CapeUdc source, number_t max_item_size)
{
  CapeStream stream = cape_stream_new ();

  cape_json_fill (stream, source, max_item_size, NULL, FALSE, 0);

  return cape_stream_to_str (&stream);
}

//-----------------------------------------------------------------------------

CapeStream cape_json_to_stream (const CapeUdc source)
{
  CapeStream stream = cape_stream_new ();
  
  cape_json_fill (stream, source, 0, NULL, FALSE, 0);

  return stream;
}

//-----------------------------------------------------------------------------

int __STDCALL cape_json_from_file__on_load (void* ptr, const char* bufdat, number_t buflen, CapeErr err)
{
  int res = cape_parser_json_process (ptr, bufdat, buflen, err);
  if (res)
  {
    cape_log_fmt (CAPE_LL_ERROR, "CAPE", "json from file", "error in pasing: %s", cape_err_text(err));    
  }
  
  return res;
}

//-----------------------------------------------------------------------------

CapeUdc cape_json_from_file (const CapeString file, CapeErr err)
{
  int res;
  CapeUdc ret = NULL;
  
  // local objects
  CapeParserJson parser_json = NULL;
  
  // create a new parser for the json format
  parser_json = cape_parser_json_new (NULL, cape_json_onItem, cape_json_onObjCreate, cape_json_onObjDestroy);
  
  // load the file in buffer chunks
  // for each chunk the callback method is called
  res = cape_fs_file_load (NULL, file, parser_json, cape_json_from_file__on_load, err);
  if (res)
  {
    goto exit_and_cleanup;
  }

  ret = cape_parser_json_object (parser_json);
  if (ret)
  {
    // set name
    //cape_udc_set_name (ret, name);
  }
  else
  {
    //eclog_msg (LL_WARN, "JSON", "reader", "returned NULL object as node");
  }
  
exit_and_cleanup:  
  
  if (parser_json)
  {
    cape_parser_json_del (&parser_json); 
  }
  
  return ret;
}

//-----------------------------------------------------------------------------

int cape_json_to_file (const CapeString file, const CapeUdc source, int nice, CapeErr err)
{
  int res;  
  CapeFileHandle fh = NULL;
  
  // allocate mem
  fh = cape_fh_new (NULL, file);
  
  // try to open the file
  res = cape_fh_open (fh, O_CREAT | O_TRUNC | O_WRONLY, err);
  if (res)
  {
    cape_err_set_fmt (err, cape_err_code (err), "can't open '%s': %s", file, cape_err_text (err));
    goto exit_and_cleanup;
  }

  {
    number_t bytes_to_write;
    number_t bytes_cn_write;
    const char* buf;
    CapeStream stream = cape_stream_new ();
  
    cape_json_fill (stream, source, 0, NULL, nice, 0);
  
    buf = cape_stream_data (stream);

    bytes_to_write = cape_stream_size (stream);
    bytes_cn_write = 0;
    
    while (bytes_cn_write < bytes_to_write)
    {
      bytes_cn_write += cape_fh_write_buf (fh, buf + bytes_cn_write, bytes_to_write - bytes_cn_write);
    }
    
    cape_stream_del (&stream);
  }
    
  res = CAPE_ERR_NONE;
  
exit_and_cleanup:  
  
  cape_fh_del (&fh);
  
  return res;
}

//-----------------------------------------------------------------------------
