#include "qwebs_multipart.h"

// cape includes
#include <stc/cape_stream.h>
#include <stc/cape_map.h>
#include <stc/cape_list.h>
#include <fmt/cape_tokenizer.h>
#include <sys/cape_log.h>

// c includes
#include <ctype.h>

//-----------------------------------------------------------------------------

struct QWebsMultipart_s
{
  CapeString boundary;

  CapeStream buffer;
  CapeStream line;
  
  CapeStream content;
  CapeMap params;

  number_t boundary_len__min;
  number_t boundary_len__max;

  void* ptr;
  qwebs_multipart__on_part on_part;

  int state;
  int breakType;
  int state2;
  int line_break_add;
};

//-----------------------------------------------------------------------------

void __STDCALL qwebs_multipart__params__on_del (void* key, void* val)
{
  {
    CapeString h = key; cape_str_del (&h);
  }
  {
    CapeString h = val; cape_str_del (&h);
  }
}

//-----------------------------------------------------------------------------

QWebsMultipart qwebs_multipart_new (const CapeString boundary, void* user_ptr, qwebs_multipart__on_part on_part)
{
  QWebsMultipart self = CAPE_NEW (struct QWebsMultipart_s);

  self->boundary = cape_str_catenate_2 ("--", boundary);
  
  self->boundary_len__min = cape_str_size (self->boundary);
  self->boundary_len__max = self->boundary_len__min + 4;
  
  self->on_part = on_part;
  self->ptr = user_ptr;
  
  self->buffer = cape_stream_new ();
  self->line = cape_stream_new ();
  self->params = cape_map_new (NULL, qwebs_multipart__params__on_del, NULL);
  
  self->state = 0;
  self->breakType = 0;

  self->content = NULL;
  self->line_break_add = FALSE;
  
  return self;
}

//-----------------------------------------------------------------------------

void qwebs_multipart_del (QWebsMultipart* p_self)
{
  if (*p_self)
  {
    QWebsMultipart self = *p_self;
    
    cape_str_del (&(self->boundary));

    cape_stream_del (&(self->buffer));
    cape_stream_del (&(self->line));

    cape_stream_del (&(self->content));
    cape_map_del (&(self->params));

    CAPE_DEL (p_self, struct QWebsMultipart_s);
  }
}

//-----------------------------------------------------------------------------

typedef struct
{
  const char* pos;
  number_t len;
  
  CapeString name;
  
} QWebsParserData;

//-----------------------------------------------------------------------------

void qwebs_multipart__pd__is_boundary (QWebsMultipart self, int addToContent)
{
  number_t lsize = cape_stream_size (self->line);
  
  // reached boundary
  if ((lsize < self->boundary_len__max) && (lsize >= self->boundary_len__min) && cape_str_begins (cape_stream_data (self->line), self->boundary))
  {
    // create a new http content
    if (self->on_part)
    {
      self->on_part (self->ptr, cape_stream_data (self->content), cape_stream_size (self->content), self->params);
    }
    
    // clear the map
    cape_map_clr (self->params);
    
    // remove the content
    cape_stream_del (&(self->content));

    self->line_break_add = FALSE;
  }
  else if (addToContent)
  {
    if (self->line_break_add)
    {
      switch (self->breakType)
      {
        case 1: cape_stream_append_c (self->content, '\r'); break;
        case 2: cape_stream_append_c (self->content, '\n'); break;
        case 3:
        {
          cape_stream_append_c (self->content, '\r');
          cape_stream_append_c (self->content, '\n');

          break;
        }
      }
      
      self->line_break_add = FALSE;
    }
    
    // append to content
    cape_stream_append_stream (self->content, self->line);
    
    self->line_break_add = TRUE;
  }
}

//-----------------------------------------------------------------------------

void qwebs_multipart__pd__param (QWebsMultipart self)
{
  // add special header value to map
  CapeString key = NULL;
  CapeString val = NULL;
  
  if (cape_tokenizer_split (cape_stream_get (self->line), ':', &key, &val))
  {
    // better to trim it here
    CapeString h_key = cape_str_trim_utf8 (key);
    CapeString h_val = cape_str_trim_utf8 (val);

    // we need unqiue keys, so better to use uppercase letters only
    cape_str_to_upper (h_key);

    // add to the map
    cape_map_insert (self->params, h_key, h_val);
  }

  // cleanup
  cape_str_del (&key);
  cape_str_del (&val);
}

//-----------------------------------------------------------------------------

int qwebs_multipart__pd__line (QWebsMultipart self, QWebsParserData* pd)
{
  if (self->content)
  {
    qwebs_multipart__pd__is_boundary (self, TRUE);
  }
  else if (cape_stream_size (self->line) == 0)
  {
    self->content = cape_stream_new ();
  }
  else
  {
    // add to parameters
    qwebs_multipart__pd__param (self);
  }
  
  cape_stream_clr (self->line);

  return TRUE;
}

//-----------------------------------------------------------------------------

int qwebs_multipart__pd__state0 (QWebsMultipart self, QWebsParserData* pd)
{
  // initial line
  // find what kind of line breaks we have and parse
  const char* c;
  for (c = pd->pos; (pd->len > 0); c++)
  {
    (pd->len)--;
    
    if (*c == '\r')
    {
      if (self->breakType > 0)
      {
        cape_log_msg (CAPE_LL_WARN, "QWEBS", "multipart", "wrong sequence of breaks #1");
        return FALSE;
      }
      
      self->breakType = 1;
      continue;
    }
    
    if (*c == '\n')
    {
      if (self->breakType == 0)
      {
        self->breakType = 2;
        continue;
      }
      else if (self->breakType == 1)
      {
        self->breakType = 3;
        continue;
      }
      else
      {
        cape_log_msg (CAPE_LL_WARN, "QWEBS", "multipart", "wrong sequence of breaks #2");
        return FALSE;
      }
    }
    
    // done, finding break type
    if (self->breakType > 0)
    {
      // TODO: parse first line
      cape_stream_clr (self->line);
      
      (pd->len)++;
      self->state = 1;
      pd->pos = c;
      
      return TRUE;
    }
  }
  
  return FALSE;
}

//-----------------------------------------------------------------------------

int qwebs_multipart__pd__state1 (QWebsMultipart self, QWebsParserData* pd)
{
  int a = 0;
  const char* c;
  for (c = pd->pos; (pd->len > 0); c++)
  {
    (pd->len)--;
    
    // check break char
    if (((self->breakType == 2) && (*c == '\n')) || ((self->breakType == 1) && (*c == '\r')))
    {
      cape_stream_append_buf (self->line, pd->pos, a);

      pd->pos = c + 1;
      return qwebs_multipart__pd__line (self, pd);
    }
    
    // check break char
    if ((self->breakType == 3) && (*c == '\r'))
    {
      cape_stream_append_buf (self->line, pd->pos, a);

      self->state = 2;
      pd->pos = c + 1;

      return TRUE;
    }
    
    if ((NULL == self->content) && ((*c == '\r')||(*c == '\n')))
    {
      cape_log_fmt (CAPE_LL_WARN, "QWEBS", "multipart", "wrong line break type %i with [%i]", self->breakType, *c);
      return FALSE;
    }
    
    a++;
  }
  
  cape_stream_append_buf (self->line, pd->pos, a);
    
  if (self->content)
  {
    qwebs_multipart__pd__is_boundary (self, FALSE);
  }
    
  return FALSE;
}

//-----------------------------------------------------------------------------

int qwebs_multipart__pd__state2 (QWebsMultipart self, QWebsParserData* pd)
{
  const char* c;
  for (c = pd->pos; (pd->len > 0); c++)
  {
    if ((self->breakType == 3) && (*c == '\n'))
    {
      (pd->len)--;

      self->state = 1;
      pd->pos = c + 1;
      
      return qwebs_multipart__pd__line (self, pd);
    }
    
    if (self->content)
    {
      cape_stream_append_c (self->line, '\r');
      self->state = 1;
      
      return TRUE;
    }
    else
    {
      cape_log_fmt (CAPE_LL_WARN, "QWEBS", "multipart", "wrong sequence of breaks #3, break type %i with [%s]", self->breakType, *c);
      return FALSE;
    }
  }
  
  return FALSE;
}

//-----------------------------------------------------------------------------

int qwebs_multipart__pd__state3 (QWebsMultipart self, QWebsParserData* pd)
{
  const char* c;
  for (c = pd->pos; pd->len > 0; c++)
  {
    (pd->len)--;
    
    if (((self->breakType == 2) && (*c == '\n')) || ((self->breakType == 1) && (*c == '\r')))
    {
      // line break !!
      self->state = 1;
      return TRUE;
    }
    
    if ((self->breakType == 3) && (*c == '\r'))
    {
      self->state = 4;
      return TRUE;
    }
    
    //ecdevstream_appendc (self->devsteam, *c);
  }
  
  return FALSE;
}

//-----------------------------------------------------------------------------

int qwebs_multipart__pd__state4 (QWebsMultipart self, QWebsParserData* pd)
{
  const char* c;
  for (c = pd->pos; pd->len > 0; c++)
  {
    (pd->len)--;
    
    if ((self->breakType == 3) && (*c == '\n'))
    {
      // line break !!
      self->state = 1;
      return TRUE;
    }
    
    //ecdevstream_appendc (self->devsteam, '\r');
    //ecdevstream_appendc (self->devsteam, *c);

    // continue
    self->state = 3;
    return TRUE;
  }
  
  return FALSE;
}

//-----------------------------------------------------------------------------

int qwebs_multipart__pd__states (QWebsMultipart self, QWebsParserData* pd)
{
  switch (self->state)
  {
    case 0: return qwebs_multipart__pd__state0 (self, pd);
    case 1: return qwebs_multipart__pd__state1 (self, pd);
    case 2: return qwebs_multipart__pd__state2 (self, pd);
    case 3: return qwebs_multipart__pd__state3 (self, pd);
    case 4: return qwebs_multipart__pd__state4 (self, pd);
  }
  
  return FALSE;
}

//-----------------------------------------------------------------------------

void qwebs_multipart_process (QWebsMultipart self, const char* bufdat, number_t buflen)
{
  QWebsParserData pd;
  
  pd.pos = bufdat;
  pd.len = buflen;
  pd.name = NULL;
  
  while (qwebs_multipart__pd__states (self, &pd));
}

//-----------------------------------------------------------------------------

CapeString qwebs_parse_line (const CapeString line, const CapeString key_to_seek)
{
  CapeListCursor* cursor;
  CapeList tokens;
  CapeString ret = NULL;
  int run = TRUE;
  
  // split the string into its parts
  tokens = cape_tokenizer_buf (line, cape_str_size (line), ';');
  
  // run through all parts to find the one which fits the key
  cursor = cape_list_cursor_create (tokens, CAPE_DIRECTION_FORW);
  while (run && cape_list_cursor_next (cursor))
  {
    CapeString token = cape_str_trim_utf8 (cape_list_node_data (cursor->node));
    
    CapeString key = NULL;
    CapeString val = NULL;
    
    if (cape_tokenizer_split (token, '=', &key, &val))
    {
      if (cape_str_equal (key_to_seek, key))
      {
        ret = cape_str_trim_c (val, '"');
        run = FALSE;
      }
    }
    
    cape_str_del (&token);
    cape_str_del (&key);
    cape_str_del (&val);
  }
  
  // cleanup
  cape_list_cursor_destroy (&cursor);
  cape_list_del (&tokens);
  
  return ret;
}

//-----------------------------------------------------------------------------

struct QWebsEncoder_s
{
  //char rfc3986[256];
  char html5[256];
};

//-----------------------------------------------------------------------------

QWebsEncoder qwebs_encode_new (void)
{
  QWebsEncoder self = CAPE_NEW (struct QWebsEncoder_s);
  
  {
    int i;
    for (i = 0; i < 256; i++)
    {
      //self->rfc3986[i] = isalnum (i) || i == '~' || i == '-' || i == '.' || i == '_' ? i : 0;
      self->html5[i] = isalnum (i) || i == '*' || i == '-' || i == '.' || i == '_' ? i : (i == ' ') ? '+' : 0;
    }
  }

  return self;
}

//-----------------------------------------------------------------------------

void qwebs_encode_del (QWebsEncoder* p_self)
{
  if (*p_self)
  {
    CAPE_DEL (p_self, struct QWebsEncoder_s);
  }
}

//-----------------------------------------------------------------------------

CapeString qwebs_encode_run (QWebsEncoder self, const CapeString url)
{
  CapeStream stream = cape_stream_new ();
  
  const char* s = url;
  
  for (; *s; s++)
  {
    int bytes_written = 0;
    
    if (self->html5[*s])
    {
      cape_stream_cap (stream, 1);
      
      bytes_written = sprintf (cape_stream_pos (stream), "%c", self->html5[*s]);
    }
    else
    {
      cape_stream_cap (stream, 4);

      bytes_written = sprintf (cape_stream_pos (stream), "%%%02X", *s);
    }
    
    if (bytes_written > 0)
    {
      cape_stream_set (stream, bytes_written);
    }
  }

  return cape_stream_to_str (&stream);
}

//-----------------------------------------------------------------------------
