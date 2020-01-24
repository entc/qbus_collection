#include "cape_parser_json.h"

// cape includes
#include "stc/cape_stream.h"
#include "stc/cape_list.h"

// c includes
#include <stdlib.h>
#include <math.h>

//-----------------------------------------------------------------------------

#define JPARSER_STATE_NONE           0
#define JPARSER_STATE_KEY_BEG        1
#define JPARSER_STATE_LIST           2
#define JPARSER_STATE_KEY_RUN        3
#define JPARSER_STATE_KEY_END        4
#define JPARSER_STATE_VAL_BEG        5

#define JPARSER_STATE_STR_RUN        6
#define JPARSER_STATE_NODE_RUN       7
#define JPARSER_STATE_LIST_RUN       8
#define JPARSER_STATE_NUMBER_RUN     9
#define JPARSER_STATE_FLOAT_RUN     10
#define JPARSER_STATE_STR_ESCAPE    11
#define JPARSER_STATE_STR_UNICODE   12
#define JPARSER_STATE_KEY_ESCAPE    13
#define JPARSER_STATE_KEY_UNICODE   14

//=============================================================================

struct CapeParserJsonItem_s
{
  // original type
  int type;
  
  // original state
  int state;
  
  // for keeping the content
  CapeStream stream;
  
  // for counting an index
  int index;
  
  // for keeping the object
  void* obj;
  
  // callback
  CapeParserJson parser;
  fct_parser_json_onObjDel onDel;
  
}; typedef struct CapeParserJsonItem_s* CapeParserJsonItem;

//-----------------------------------------------------------------------------

CapeParserJsonItem cape_parser_json_item_new (int type, int state, CapeParserJson parser, fct_parser_json_onObjDel onDel)
{
  CapeParserJsonItem self = CAPE_NEW (struct CapeParserJsonItem_s);
  
  self->stream = cape_stream_new ();
  self->index = 0;
  self->type = type;
  self->state = state;
  self->obj = NULL;
  
  self->parser = parser;
  self->onDel = onDel;
  
  return self;
}

//-----------------------------------------------------------------------------

void cape_parser_json_item_delLastObject (CapeParserJsonItem self)
{
  if (self->obj && self->onDel)
  {
    self->onDel (self->parser, self->obj);
    self->obj = NULL;
  }
}

//-----------------------------------------------------------------------------

void cape_parser_json_item_del (CapeParserJsonItem* p_self)
{
  CapeParserJsonItem self = *p_self;
  
  cape_stream_del (&(self->stream));
  
  cape_parser_json_item_delLastObject (self);
    
  CAPE_DEL (p_self, struct CapeParserJsonItem_s);
}

//-----------------------------------------------------------------------------

void cape_parser_json_item_setObject (CapeParserJsonItem self, void** obj, int type)
{
  cape_parser_json_item_delLastObject (self);
  
  // transfer object
  self->obj = *obj;
  *obj = NULL;
  
  self->type = type;
}

//-----------------------------------------------------------------------------

struct CapeParserJson_s
{
  fct_parser_json_onItem onItem;
  fct_parser_json_onObjNew onObjNew;
  fct_parser_json_onObjDel onObjDel;
  
  void* ptr;
  
  CapeList stack;
  
  CapeParserJsonItem keyElement;
  CapeParserJsonItem valElement;
  
  // for unicode decoding
  unsigned char unicode_data[10];
  int unicode_pos;
};

//-----------------------------------------------------------------------------

static void __STDCALL cape_parser_json_stack_onDel (void* ptr)
{
  CapeParserJsonItem item = ptr;
  
  cape_parser_json_item_del (&item);
}

//-----------------------------------------------------------------------------

static void __STDCALL cape_parser_json_element_push_onObjDel (void* ptr, void* obj)
{
  CapeParserJson self = ptr;
  
  if (self->onObjDel)
  {
    self->onObjDel (self->ptr, obj);
  }
}

//-----------------------------------------------------------------------------

CapeParserJson cape_parser_json_new (void* ptr, fct_parser_json_onItem onItem, fct_parser_json_onObjNew onObjNew, fct_parser_json_onObjDel onObjDel)
{
  CapeParserJson self = CAPE_NEW (struct CapeParserJson_s);
  
  self->ptr = ptr;
  self->onItem = onItem;
  self->onObjNew = onObjNew;
  self->onObjDel = onObjDel;
  
  self->keyElement = NULL; // not yet created
  self->valElement = cape_parser_json_item_new (CAPE_JPARSER_UNDEFINED, JPARSER_STATE_NONE, self, cape_parser_json_element_push_onObjDel);
  
  self->stack = cape_list_new (cape_parser_json_stack_onDel);
  
  // prepare the hex notation 0x0000
  self->unicode_data [0] = '0';
  self->unicode_data [1] = 'x';
  self->unicode_data [2] = '\0';
  self->unicode_data [3] = '\0';
  self->unicode_data [4] = '\0';
  self->unicode_data [5] = '\0';
  self->unicode_data [6] = '\0';
  
  return self;
}

//-----------------------------------------------------------------------------

void cape_parser_json_del (CapeParserJson* p_self)
{
  CapeParserJson self = *p_self;
  
  cape_list_del (&(self->stack));
  
  cape_parser_json_item_del (&(self->valElement));
    
  CAPE_DEL (p_self, struct CapeParserJson_s);
}

//-----------------------------------------------------------------------------

void cape_parser_json_push (CapeParserJson self, int type, int state)
{
  CapeParserJsonItem item = cape_parser_json_item_new (type, state, self, cape_parser_json_element_push_onObjDel);
  
  if (self->onObjNew)
  {
    item->obj = self->onObjNew (self->ptr, type);
  }
  
  cape_list_push_back (self->stack, item);
  
  self->keyElement = item;
}

//-----------------------------------------------------------------------------

void cape_parser_json_pop (CapeParserJson self)
{
  // transfer all element values to the content element
  {
    CapeParserJsonItem element = cape_list_pop_back (self->stack);
    
    cape_parser_json_item_setObject (self->valElement, &(element->obj), element->type);
    
    cape_parser_json_stack_onDel (element);
  }
  
  // fetch last element
  {
    CapeListCursor cursor;
    cape_list_cursor_init (self->stack, &cursor, CAPE_DIRECTION_PREV);
    
    if (cape_list_cursor_prev (&cursor))
    {
      self->keyElement = cape_list_node_data (cursor.node);
    }
    else
    {
      self->keyElement = NULL;
    }
  }
}

//-----------------------------------------------------------------------------

int cape_parser_json_leave_value (CapeParserJson self, int state)
{
  if (state == JPARSER_STATE_LIST_RUN)
  {
    cape_stream_clr (self->valElement->stream);
    return JPARSER_STATE_VAL_BEG;
  }
  else if (state == JPARSER_STATE_NODE_RUN)
  {
    return JPARSER_STATE_KEY_BEG;
  }
  
  return JPARSER_STATE_NONE;
}

//-----------------------------------------------------------------------------

/*
void cape_parser_json_decode_unicode (unsigned int unicode, CapeStream dest)
{
  if (unicode < 0x80)
  {
    cape_stream_append_c (dest, unicode);
  }
  else if (unicode < 0x800)
  {
    cape_stream_append_c (dest, 192 + unicode / 64);
    cape_stream_append_c (dest, 128 + unicode % 64);
  }
  else if (unicode - 0xd800u < 0x800)
  {
    // error
  }
  else if (unicode < 0x10000)
  {
    cape_stream_append_c (dest, 224 + unicode / 4096);
    cape_stream_append_c (dest, 128 + unicode /64 % 64);
    cape_stream_append_c (dest, 128 + unicode % 64);
  }
  else if (unicode < 0x110000)
  {
    cape_stream_append_c (dest, 240 + unicode / 262144);
    cape_stream_append_c (dest, 128 + unicode / 4096 % 64);
    cape_stream_append_c (dest, 128 + unicode / 64 % 64);
    cape_stream_append_c (dest, 128 + unicode % 64);
  }
  else
  {
    // error
  }
}

//-----------------------------------------------------------------------------

unsigned int cape_parser_json_decode_unicode_point (const char** pc)
{
  int i;
  const char* pos = *pc;
  unsigned int unicode = 0;
  
  for (i = 0; (i < 4) && *pos; ++i, ++pos)
  {
    char c = *pos;
    unicode *= 16;
    
    if ( c >= '0'  &&  c <= '9' )
      unicode += c - '0';
    else if ( c >= 'a'  &&  c <= 'f' )
      unicode += c - 'a' + 10;
    else if ( c >= 'A'  &&  c <= 'F' )
      unicode += c - 'A' + 10;
    else
    {
      // "Bad unicode escape sequence in string: hexadecimal digit expected."
      return 0;
    }
  }
  
  *pc = pos - 1;
  
  return unicode;
}
*/

//-----------------------------------------------------------------------------

void cape_parser_json_decode_unicode_hex (CapeParserJson self, CapeStream dest)
{
  // convert from HEX into wide char
  wchar_t wc = strtol ((const char*)self->unicode_data, NULL, 16);
  
  //printf ("HEX: %s -> %u\n", self->unicode_data, wc);
  
  if ( 0 <= wc && wc <= 0x7f )
  {
    cape_stream_append_c (dest, wc);
  }
  else if ( 0x80 <= wc && wc <= 0x7ff )
  {
    cape_stream_append_c (dest, 0xc0 | (wc >> 6));
    cape_stream_append_c (dest, 0x80 | (wc & 0x3f));
  }
  else if ( 0x800 <= wc && wc <= 0xffff )
  {
    cape_stream_append_c (dest, 0xe0 | (wc >> 12));
    cape_stream_append_c (dest, 0x80 | ((wc >> 6) & 0x3f));
    cape_stream_append_c (dest, 0x80 | (wc & 0x3f));
  }
  else if ( 0x10000 <= wc && wc <= 0x1fffff )
  {
    cape_stream_append_c (dest, 0xf0 | (wc >> 18));
    cape_stream_append_c (dest, 0x80 | ((wc >> 12) & 0x3f));
    cape_stream_append_c (dest, 0x80 | ((wc >> 6) & 0x3f));
    cape_stream_append_c (dest, 0x80 | (wc & 0x3f));
  }
  else if ( 0x200000 <= wc && wc <= 0x3ffffff )
  {
    cape_stream_append_c (dest, 0xf8 | (wc >> 24));
    cape_stream_append_c (dest, 0x80 | ((wc >> 18) & 0x3f));
    cape_stream_append_c (dest, 0x80 | ((wc >> 12) & 0x3f));
    cape_stream_append_c (dest, 0x80 | ((wc >> 6) & 0x3f));
    cape_stream_append_c (dest, 0x80 | (wc & 0x3f));
  }
  else if ( 0x4000000 <= wc && wc <= 0x7fffffff )
  {
    cape_stream_append_c (dest, 0xfc | (wc >> 30) );
    cape_stream_append_c (dest, 0x80 | ((wc >> 24) & 0x3f) );
    cape_stream_append_c (dest, 0x80 | ((wc >> 18) & 0x3f) );
    cape_stream_append_c (dest, 0x80 | ((wc >> 12) & 0x3f) );
    cape_stream_append_c (dest, 0x80 | ((wc >> 6) & 0x3f) );
    cape_stream_append_c (dest, 0x80 | (wc & 0x3f) );
  }
}

//-----------------------------------------------------------------------------

void cape_parser_json_item_next (CapeParserJson self, int type, const char* key, int index)
{
  switch (type)
  {
    case CAPE_JPARSER_OBJECT_TEXT:
    {
      // parse again if text might be a datetime : 2012-04-23T18:25:43.511Z
      if (cape_stream_size (self->valElement->stream) == 24)
      {
        const char* buf = cape_stream_get (self->valElement->stream);
        
        if (buf[4] == '-' && buf[7] == '-' && buf[10] == 'T' && buf[13] == ':' && buf[16] == ':' && buf[19] == '.' && buf[23] == 'Z')
        {
          CapeDatetime dt;
          
          if (cape_datetime__std (&dt, buf))
          {
            if (self->onItem)
            {
              // void* ptr, void* obj, int type, const char* key, void* val
              self->onItem (self->ptr, self->keyElement->obj, CAPE_JPARSER_OBJECT_DATETIME, (void*)&dt, key, index);
            }
            
            cape_stream_clr (self->valElement->stream);
            break;
          }
        }
      }
      
      if (self->onItem)
      {
        // void* ptr, void* obj, int type, const char* key, void* val
        self->onItem (self->ptr, self->keyElement->obj, CAPE_JPARSER_OBJECT_TEXT, (void*)cape_stream_get (self->valElement->stream), key, index);
      }
      
      cape_stream_clr (self->valElement->stream);
      break;
    }
    case CAPE_JPARSER_OBJECT_NUMBER:
    {
      if (self->onItem)
      {
        char* endptr = NULL;
        const char* val = cape_stream_get (self->valElement->stream);
        
#ifdef _WIN32
        number_t dat = atoi (val);

        self->onItem (self->ptr, self->keyElement->obj, CAPE_JPARSER_OBJECT_NUMBER, (void*)&dat, key, index);
#else
        number_t dat = strtoll (val, &endptr, 10);
        if (endptr == NULL)
        {
          // was not able to transform
          
        }
        else
        {
          // void* ptr, void* obj, int type, const char* key, void* val
          self->onItem (self->ptr, self->keyElement->obj, CAPE_JPARSER_OBJECT_NUMBER, (void*)&dat, key, index);
        }        
#endif
      }
      
      cape_stream_clr (self->valElement->stream);
      break;
    }
    case CAPE_JPARSER_OBJECT_FLOAT:
    {      
      if (self->onItem)
      {
        char* endptr = NULL;
        const char* val = cape_stream_get (self->valElement->stream);
        
        double dat = strtod (val, &endptr);
        if (endptr == NULL)
        {
          // was not able to transform
          
        }
        else
        {
          // void* ptr, void* obj, int type, const char* key, void* val
          self->onItem (self->ptr, self->keyElement->obj, CAPE_JPARSER_OBJECT_FLOAT, (void*)&dat, key, index);
        }
      }
      
      cape_stream_clr (self->valElement->stream);
      break;
    }
    case CAPE_JPARSER_UNDEFINED:
    {
      switch (self->valElement->type)
      {
        case CAPE_JPARSER_OBJECT_NODE:
        case CAPE_JPARSER_OBJECT_LIST:
        {
          if (self->onItem)
          {
            self->onItem (self->ptr, self->keyElement->obj, self->valElement->type, self->valElement->obj, key, index);
            
            // assume that the object was transfered
            self->valElement->obj = NULL;
            self->valElement->type = CAPE_JPARSER_UNDEFINED;
          }
          
          break;
        }
        case CAPE_JPARSER_UNDEFINED:
        {
          const char* val = cape_stream_get (self->valElement->stream);
          
          // we need to detect the kind of element
          switch (val[0])
          {
            case 't':
            {
              if (cape_str_equal ("true", val) && self->onItem)
              {
                self->onItem (self->ptr, self->keyElement->obj, CAPE_JPARSER_OBJECT_BOLEAN, (void*)TRUE, key, index);
              }
              
              cape_stream_clr (self->valElement->stream);
              break;
            }
            case 'f':
            {
              if (cape_str_equal ("false", val) && self->onItem)
              {
                self->onItem (self->ptr, self->keyElement->obj, CAPE_JPARSER_OBJECT_BOLEAN, (void*)FALSE, key, index);
              }
              
              cape_stream_clr (self->valElement->stream);
              break;
            }
            case 'n':
            {
              if (cape_str_equal ("null", val) && self->onItem)
              {
                self->onItem (self->ptr, self->keyElement->obj, CAPE_JPARSER_OBJECT_NULL, NULL, key, index);
              }
              
              if (cape_str_equal ("nan", val) && self->onItem)
              {
                // create a double as NotANumber
                double h = CAPE_MATH_NAN;
                
                self->onItem (self->ptr, self->keyElement->obj, CAPE_JPARSER_OBJECT_FLOAT, &h, key, index);
              }
              
              cape_stream_clr (self->valElement->stream);
              break;
            }
            case 'i':
            {
              if (cape_str_equal ("inf", val) && self->onItem)
              {
                // create a double as infinity value
                double h = CAPE_MATH_INFINITY;
                
                self->onItem (self->ptr, self->keyElement->obj, CAPE_JPARSER_OBJECT_FLOAT, &h, key, index);
              }
              
              cape_stream_clr (self->valElement->stream);
              break;
            }
          }
          
          cape_stream_clr (self->valElement->stream);
          break;
        }
      }
      break;
    }
  }
}

//-----------------------------------------------------------------------------

int cape_parser_json_item (CapeParserJson self, int type)
{
  if (self->keyElement)
  {
    switch (self->keyElement->state)
    {
      case JPARSER_STATE_NODE_RUN:
      {
        cape_parser_json_item_next (self, type, cape_stream_get (self->keyElement->stream), 0);
        
        cape_stream_clr (self->keyElement->stream);
        break;
      }
      case JPARSER_STATE_LIST_RUN:
      {
        cape_parser_json_item_next (self, type, NULL, self->keyElement->index);
        
        self->keyElement->index++;
        break;
      }
    }
    
    return self->keyElement->state;
  }
  
  return JPARSER_STATE_NONE;
}

//-----------------------------------------------------------------------------

int cape_parser_json_process (CapeParserJson self, const char* buffer, number_t size, CapeErr err)
{
  const char* c = buffer;
  number_t i;
  
  int state = self->valElement->state;
  
  for (i = 0; (i < size) && *c; i++, c++)
  {
    switch (*c)
    {
      case '{':
      {
        switch (state)
        {
          case JPARSER_STATE_NONE:
          {
            cape_parser_json_push (self, CAPE_JPARSER_OBJECT_NODE, JPARSER_STATE_NODE_RUN);
            
            state = JPARSER_STATE_KEY_BEG;
            break;
          }
          case JPARSER_STATE_VAL_BEG:
          {
            cape_parser_json_push (self, CAPE_JPARSER_OBJECT_NODE, JPARSER_STATE_NODE_RUN);
            
            state = JPARSER_STATE_KEY_BEG;
            break;
          }
          case JPARSER_STATE_STR_RUN:
          {
            cape_stream_append_c (self->valElement->stream, *c);
            
            break;
          }
          default:
          {
            return cape_err_set_fmt (err, CAPE_ERR_PARSER, "unexpected state [%i] in '{' : '%s'", state, c);
          }
        }
        
        break;
      }
      case '[':
      {
        switch (state)
        {
          case JPARSER_STATE_NONE:
          {
            state = JPARSER_STATE_VAL_BEG;
            cape_stream_clr (self->valElement->stream);
            
            cape_parser_json_push (self, CAPE_JPARSER_OBJECT_LIST, JPARSER_STATE_LIST_RUN);
            break;
          }
          case JPARSER_STATE_VAL_BEG:
          {
            state = JPARSER_STATE_VAL_BEG;
            cape_stream_clr (self->valElement->stream);
            
            cape_parser_json_push (self, CAPE_JPARSER_OBJECT_LIST, JPARSER_STATE_LIST_RUN);
            break;
          }
          case JPARSER_STATE_STR_RUN:
          {
            cape_stream_append_c (self->valElement->stream, *c);
            
            break;
          }
          default:
          {
            return cape_err_set (err, CAPE_ERR_PARSER, "unexpected state in '['");
          }
        }
        
        break;
      }
      case '}':
      {
        switch (state)
        {
          case JPARSER_STATE_NODE_RUN:
          {
            cape_parser_json_pop (self);
            
            if (self->keyElement)
            {
              state = JPARSER_STATE_VAL_BEG;
            }
            
            break;
          }
          case JPARSER_STATE_KEY_BEG:
          {
            cape_parser_json_pop (self);
            
            if (self->keyElement)
            {
              state = JPARSER_STATE_VAL_BEG;
            }
            
            break;
          }
          case JPARSER_STATE_VAL_BEG:
          {
            state = cape_parser_json_item (self, CAPE_JPARSER_UNDEFINED);
            
            cape_parser_json_pop (self);
            
            if (self->keyElement)
            {
              state = JPARSER_STATE_VAL_BEG;
            }
            
            break;
          }
          case JPARSER_STATE_NUMBER_RUN:
          {
            state = cape_parser_json_item (self, CAPE_JPARSER_OBJECT_NUMBER);
            
            cape_parser_json_pop (self);
            
            if (self->keyElement)
            {
              state = JPARSER_STATE_VAL_BEG;
            }
            
            break;
          }
          case JPARSER_STATE_FLOAT_RUN:
          {
            state = cape_parser_json_item (self, CAPE_JPARSER_OBJECT_FLOAT);
            
            cape_parser_json_pop (self);
            
            if (self->keyElement)
            {
              state = JPARSER_STATE_VAL_BEG;
            }
            
            break;
          }
          case JPARSER_STATE_STR_RUN:
          {
            cape_stream_append_c (self->valElement->stream, *c);
            
            break;
          }
          default:
          {
            return cape_err_set (err, CAPE_ERR_PARSER, "unexpected state in '}'");
          }
        }
        break;
      }
      case ']':
      {
        switch (state)
        {
          case JPARSER_STATE_LIST_RUN:
          {
            cape_parser_json_pop (self);
            
            if (self->keyElement)
            {
              state = JPARSER_STATE_VAL_BEG;
            }
            
            break;
          }
          case JPARSER_STATE_VAL_BEG:
          {
            state = cape_parser_json_item (self, CAPE_JPARSER_UNDEFINED);
            
            cape_parser_json_pop (self);
            
            if (self->keyElement)
            {
              state = JPARSER_STATE_VAL_BEG;
            }
            
            break;
          }
          case JPARSER_STATE_NUMBER_RUN:
          {
            state = cape_parser_json_item (self, CAPE_JPARSER_OBJECT_NUMBER);
            
            cape_parser_json_pop (self);
            
            if (self->keyElement)
            {
              state = JPARSER_STATE_VAL_BEG;
            }
            
            break;
          }
          case JPARSER_STATE_FLOAT_RUN:
          {
            state = cape_parser_json_item (self, CAPE_JPARSER_OBJECT_FLOAT);
            
            cape_parser_json_pop (self);
            
            if (self->keyElement)
            {
              state = JPARSER_STATE_VAL_BEG;
            }
            
            break;
          }
          case JPARSER_STATE_STR_RUN:
          {
            cape_stream_append_c (self->valElement->stream, *c);
            
            break;
          }
          default:
          {
            return cape_err_set (err, CAPE_ERR_PARSER, "unexpected state in ']'");
          }
        }
        break;
      }
      case '"':
      {
        switch (state)
        {
          case JPARSER_STATE_KEY_BEG:    // start of key
          {
            state = JPARSER_STATE_KEY_RUN;
            break;
          }
          case JPARSER_STATE_KEY_RUN:    // end of key
          {
            state = JPARSER_STATE_KEY_END;
            break;
          }
          case JPARSER_STATE_VAL_BEG:
          {
            state = JPARSER_STATE_STR_RUN;
            break;
          }
          case JPARSER_STATE_STR_RUN:
          {
            state = cape_parser_json_item (self, CAPE_JPARSER_OBJECT_TEXT);
            break;
          }
          case JPARSER_STATE_KEY_ESCAPE:
          {
            CapeParserJsonItem element = self->keyElement;
            
            if (element)
            {
              cape_stream_append_c (element->stream, *c);
            }
            
            state = JPARSER_STATE_KEY_RUN;
            break;
          }
          case JPARSER_STATE_STR_ESCAPE:
          {
            cape_stream_append_c (self->valElement->stream, *c);
            
            state = JPARSER_STATE_STR_RUN;
            break;
          }
          default:
          {
            return cape_err_set (err, CAPE_ERR_PARSER, "unexpected state in '\"'");
          }
        }
        break;
      }
      case '\\':
      {
        switch (state)
        {
          case JPARSER_STATE_KEY_RUN:
          {
            state = JPARSER_STATE_KEY_ESCAPE;
            break;
          }
          case JPARSER_STATE_STR_RUN:
          {
            state = JPARSER_STATE_STR_ESCAPE;
            break;
          }
          case JPARSER_STATE_KEY_ESCAPE:
          {
            CapeParserJsonItem element = self->keyElement;
            
            if (element)
            {
              cape_stream_append_c (element->stream, *c);
            }
            
            state = JPARSER_STATE_KEY_RUN;
            break;
          }
          case JPARSER_STATE_STR_ESCAPE:
          {
            cape_stream_append_c (self->valElement->stream, *c);
            
            state = JPARSER_STATE_STR_RUN;
            break;
          }
          default:
          {
            return cape_err_set (err, CAPE_ERR_PARSER, "unexpected state in '\\'");
            
          }
        }
        break;
      }
      case ',':
      {
        switch (state)
        {
          case JPARSER_STATE_NODE_RUN:
          {
            state = JPARSER_STATE_KEY_BEG;
            break;
          }
          case JPARSER_STATE_LIST_RUN:
          {
            state = JPARSER_STATE_VAL_BEG;
            cape_stream_clr (self->valElement->stream);
            
            break;
          }
          case JPARSER_STATE_VAL_BEG:
          {
            state = cape_parser_json_item (self, CAPE_JPARSER_UNDEFINED);
            
            state = cape_parser_json_leave_value (self, state);
            
            break;
          }
          case JPARSER_STATE_NUMBER_RUN:
          {
            state = cape_parser_json_item (self, CAPE_JPARSER_OBJECT_NUMBER);
            
            state = cape_parser_json_leave_value (self, state);
            
            break;
          }
          case JPARSER_STATE_FLOAT_RUN:
          {
            state = cape_parser_json_item (self, CAPE_JPARSER_OBJECT_FLOAT);
            
            state = cape_parser_json_leave_value (self, state);
            
            break;
          }
          case JPARSER_STATE_STR_RUN:
          {
            cape_stream_append_c (self->valElement->stream, *c);
            
            break;
          }
          case JPARSER_STATE_KEY_RUN:    // end of key
          {
            CapeParserJsonItem element = self->keyElement;
            
            if (element)
            {
              cape_stream_append_c (element->stream, *c);
            }
            
            break;
          }
          default:
          {
            return cape_err_set (err, CAPE_ERR_PARSER, "unexpected state in ','");
          }
        }
        break;
      }
      case ':':
      {
        switch (state)
        {
          case JPARSER_STATE_KEY_END:
          {
            state = JPARSER_STATE_VAL_BEG;
            cape_stream_clr (self->valElement->stream);
            
            break;
          }
          case JPARSER_STATE_STR_RUN:
          {
            cape_stream_append_c (self->valElement->stream, *c);
            
            break;
          }
          default:
          {
            return cape_err_set (err, CAPE_ERR_PARSER, "unexpected state in ':'");
          }
        }
        break;
      }
      case ' ':
      case '\n':
      case '\r':
      case '\t':
      {
        switch (state)
        {
          case JPARSER_STATE_STR_RUN:
          {
            cape_stream_append_c (self->valElement->stream, *c);
            
            break;
          }
          default:
          {
            // ignore
          }
        }
        
        break;
      }
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
      {
        switch (state)
        {
          case JPARSER_STATE_VAL_BEG:
          {
            state = JPARSER_STATE_NUMBER_RUN;
            
            cape_stream_append_c (self->valElement->stream, *c);
            break;
          }
          case JPARSER_STATE_NUMBER_RUN:
          {
            cape_stream_append_c (self->valElement->stream, *c);
            break;
          }
          case JPARSER_STATE_FLOAT_RUN:
          {
            cape_stream_append_c (self->valElement->stream, *c);
            break;
          }
          case JPARSER_STATE_STR_RUN:
          {
            cape_stream_append_c (self->valElement->stream, *c);
            
            break;
          }
          case JPARSER_STATE_KEY_RUN:
          {
            CapeParserJsonItem element = self->keyElement;
            
            if (element)
            {
              cape_stream_append_c (element->stream, *c);
            }
            
            break;
          }
          case JPARSER_STATE_KEY_UNICODE:
          {
            self->unicode_data [self->unicode_pos + 2] = *c;
            self->unicode_pos++;
            
            if (self->unicode_pos == 4)
            {
              CapeParserJsonItem element = self->keyElement;
              
              if (element)
              {
                cape_parser_json_decode_unicode_hex (self, element->stream);
              }
              
              state = JPARSER_STATE_KEY_RUN;
            }
            
            break;
          }
          case JPARSER_STATE_STR_UNICODE:
          {
            self->unicode_data [self->unicode_pos + 2] = *c;
            self->unicode_pos++;
            
            if (self->unicode_pos == 4)
            {
              cape_parser_json_decode_unicode_hex (self, self->valElement->stream);              
              state = JPARSER_STATE_STR_RUN;
            }
            
            break;
          }
          default:
          {
            return cape_err_set (err, CAPE_ERR_PARSER, "unexpected state by number");
          }
        }
        
        break;
      }
      case '-':
      {
        switch (state)
        {
          case JPARSER_STATE_VAL_BEG:
          {
            state = JPARSER_STATE_NUMBER_RUN;
            
            cape_stream_append_c (self->valElement->stream, *c);
            break;
          }
          case JPARSER_STATE_KEY_RUN:
          {
            CapeParserJsonItem element = self->keyElement;
            
            if (element)
            {
              cape_stream_append_c (element->stream, *c);
            }
            
            break;
          }
          case JPARSER_STATE_STR_RUN:
          {
            cape_stream_append_c (self->valElement->stream, *c);
            
            break;
          }
          default:
          {
            return cape_err_set (err, CAPE_ERR_PARSER, "unexpected state in '-'");
          }
        }
        
        break;
      }
      case '.':
      {
        switch (state)
        {
          case JPARSER_STATE_NUMBER_RUN:
          {
            state = JPARSER_STATE_FLOAT_RUN;
            
            cape_stream_append_c (self->valElement->stream, *c);
            break;
          }
          case JPARSER_STATE_KEY_RUN:
          {
            CapeParserJsonItem element = self->keyElement;
            
            if (element)
            {
              cape_stream_append_c (element->stream, *c);
            }
            
            break;
          }
          case JPARSER_STATE_STR_RUN:
          {
            cape_stream_append_c (self->valElement->stream, *c);
            
            break;
          }
          case JPARSER_STATE_VAL_BEG:
          {
            cape_stream_append_c (self->valElement->stream, *c);
            
            break;
          }
          default:
          {
            return cape_err_set (err, CAPE_ERR_PARSER, "unexpected state in '.'");
          }
        }
        
        break;
      }
      default:
      {
        switch (state)
        {
          case JPARSER_STATE_KEY_RUN:
          {
            CapeParserJsonItem element = self->keyElement;
            
            if (element)
            {
              cape_stream_append_c (element->stream, *c);
            }
            
            break;
          }
          case JPARSER_STATE_STR_RUN:
          {
            cape_stream_append_c (self->valElement->stream, *c);
            
            break;
          }
          case JPARSER_STATE_KEY_ESCAPE:
          {
            CapeParserJsonItem element = self->keyElement;
            
            if (element) switch (*c)
            {
              case '/':
              {
                cape_stream_append_c (element->stream, '/');
                
                state = JPARSER_STATE_KEY_RUN;
                break;
              }
              case 'n':
              {
                cape_stream_append_c (element->stream, '\n');
                
                state = JPARSER_STATE_KEY_RUN;
                break;
              }
              case 't':
              {
                cape_stream_append_c (element->stream, '\t');
                
                state = JPARSER_STATE_KEY_RUN;
                break;
              }
              case 'r':
              {
                cape_stream_append_c (element->stream, '\r');
                
                state = JPARSER_STATE_KEY_RUN;
                break;
              }
              case 'b':
              {
                cape_stream_append_c (element->stream, '\b');
                
                state = JPARSER_STATE_KEY_RUN;
                break;
              }
              case 'f':
              {
                cape_stream_append_c (element->stream, '\f');
                
                state = JPARSER_STATE_KEY_RUN;
                break;
              }
              case 'u':
              {
                // hex encoded unicode
                state = JPARSER_STATE_KEY_UNICODE;
                self->unicode_pos = 0;
                
                break;
              }
              default:
              {
//                eclog_fmt (LL_WARN, "CAPE", "ecjparser", "unknown escape sequence '%c' -> [%i]", *c, *c);
                
                cape_stream_append_c (element->stream, *c);
                
                state = JPARSER_STATE_KEY_RUN;
                break;
              }
            }
            break;
          }
          case JPARSER_STATE_STR_ESCAPE:
          {
            // check excape sequence
            switch (*c)
            {
              case '/':
              {
                cape_stream_append_c (self->valElement->stream, '/');
                
                state = JPARSER_STATE_STR_RUN;
                break;
              }
              case 'n':
              {
                cape_stream_append_c (self->valElement->stream, '\n');
                
                state = JPARSER_STATE_STR_RUN;
                break;
              }
              case 't':
              {
                cape_stream_append_c (self->valElement->stream, '\t');
                
                state = JPARSER_STATE_STR_RUN;
                break;
              }
              case 'r':
              {
                cape_stream_append_c (self->valElement->stream, '\r');
                
                state = JPARSER_STATE_STR_RUN;
                break;
              }
              case 'b':
              {
                cape_stream_append_c (self->valElement->stream, '\b');
                
                state = JPARSER_STATE_STR_RUN;
                break;
              }
              case 'f':
              {
                cape_stream_append_c (self->valElement->stream, '\f');
                
                state = JPARSER_STATE_STR_RUN;
                break;
              }
              case 'u':
              {
                // hex encoded unicode
                state = JPARSER_STATE_STR_UNICODE;                
                self->unicode_pos = 0;
                
                break;
              }
              default:
              {
                //eclog_fmt (LL_WARN, "CAPE", "ecjparser", "unknown escape sequence '%c' -> [%i]", *c, *c);
                
                cape_stream_append_c (self->valElement->stream, *c);
                
                state = JPARSER_STATE_STR_RUN;
                break;
              }
            }
            
            break;
          }
          case JPARSER_STATE_KEY_UNICODE:
          {
            self->unicode_data [self->unicode_pos + 2] = *c;
            self->unicode_pos++;
            
            if (self->unicode_pos == 4)
            {
              CapeParserJsonItem element = self->keyElement;
              
              if (element)
              {
                cape_parser_json_decode_unicode_hex (self, element->stream);
              }
              
              state = JPARSER_STATE_KEY_RUN;
            }
            
            break;
          }
          case JPARSER_STATE_STR_UNICODE:
          {
            self->unicode_data [self->unicode_pos + 2] = *c;
            self->unicode_pos++;
            
            if (self->unicode_pos == 4)
            {
              cape_parser_json_decode_unicode_hex (self, self->valElement->stream);
              state = JPARSER_STATE_STR_RUN;
            }
            
            break;
          }
          case JPARSER_STATE_VAL_BEG:
          {
            cape_stream_append_c (self->valElement->stream, *c);
            
            break;
          }
          default:
          {
            return cape_err_set_fmt (err, CAPE_ERR_PARSER, "unexpected state [%i] in default '%c' <- [%i]", state, *c, *c);
          }
        }
        
        break;
      }
    }
  }
  
  self->valElement->state = state;
  
  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

void* cape_parser_json_object (CapeParserJson self)
{
  void* obj = self->valElement->obj;
  
  self->valElement->obj = NULL;
  
  return obj;
}

//-----------------------------------------------------------------------------
