#include "cape_parser_line.h"
#include "stc/cape_stream.h"

//-----------------------------------------------------------------------------

// state
#define SLP_STATE_TEXT     0
#define SLP_STATE_BR_R     1
#define SLP_STATE_BR_N     2

// mode
#define SLP_MODE_BR_NONE   0
#define SLP_MODE_BR_R      1
#define SLP_MODE_BR_N      2
#define SLP_MODE_BR_RN     3

//-----------------------------------------------------------------------------

struct CapeParserLine_s
{
  int state;   // for the state machine
  int mode;

  CapeStream stream;

  // *** for callback ***
  fct_parser_line__on_newline on_newline;
  void* ptr;
};

//-----------------------------------------------------------------------------

CapeParserLine cape_parser_line_new (void* ptr, fct_parser_line__on_newline on_newline)
{
  CapeParserLine self = CAPE_NEW (struct CapeParserLine_s);
  
  self->on_newline = on_newline;
  self->ptr = ptr;
  
  self->state = SLP_STATE_TEXT;
  self->mode = SLP_MODE_BR_NONE;
  
  self->stream = cape_stream_new ();
  
  return self;
}

//-----------------------------------------------------------------------------

void cape_parser_line_del (CapeParserLine* p_self)
{
  if (*p_self)
  {
    CapeParserLine self = *p_self;
    
    cape_stream_del (&(self->stream));
    
    CAPE_DEL (p_self, struct CapeParserLine_s);
  }
}

//-----------------------------------------------------------------------------

void cape_parser_line__record (CapeParserLine self)
{
  if (cape_stream_size (self->stream) > 0)
  {
    if (self->on_newline)
    {
      self->on_newline (self->ptr, cape_stream_get (self->stream));
    }
    
    cape_stream_clr (self->stream);
  }
  else
  {
    if (self->on_newline)
    {
      self->on_newline (self->ptr, NULL);
    }
  }
}

//-----------------------------------------------------------------------------

int cape_parser_line_process (CapeParserLine self, const char* buffer, number_t size, CapeErr err)
{
  int i;
  const char* c = buffer;
  
  for (i = 0; i < size; i++, c++)
  {
    switch (*c)
    {
      case 0:
      {
        // string terminated
        return CAPE_ERR_NONE;
      }
      case '\r':
      {
        switch (self->mode)
        {
          case SLP_MODE_BR_R:
          {
            // record line break
            cape_parser_line__record (self);
            break;            
          }
          case SLP_MODE_BR_N:
          {
            // wrong line break -> ignore ??
            break;
          }
          case SLP_MODE_BR_RN:
          {
            self->state = SLP_STATE_BR_R;
            break;
          }
          default: 
          {
            switch (self->state)
            {
              case SLP_STATE_TEXT:
              {
                self->state = SLP_STATE_BR_R;
                break;          
              }
              case SLP_STATE_BR_R:
              {
                // set new mode
                self->mode = SLP_MODE_BR_R;
                self->state = SLP_STATE_TEXT;
                
                // record line break
                cape_parser_line__record (self);
                break;
              }
              case SLP_STATE_BR_N:
              {
                // not supported mode
                self->state = SLP_STATE_TEXT;
                
                // record line break
                cape_parser_line__record (self);
                break;
              }
            }
            break;
          }
        }
        break;
      }
      case '\n':
      {
        switch (self->mode)
        {
          case SLP_MODE_BR_R:
          {
            // wrong line break -> ignore ??
            break;            
          }
          case SLP_MODE_BR_N:
          {
            // record line break            
            cape_parser_line__record (self);
            break;
          }
          case SLP_MODE_BR_RN:
          {
            if (self->state == SLP_STATE_BR_R)
            {
              self->state = SLP_STATE_TEXT;
              cape_parser_line__record (self);
            }
            else
            {
              // wrong line break -> ignore ??  
            }
            break;
          }
          default:
          {
            switch (self->state)
            {
              case SLP_STATE_TEXT:
              {
                self->state = SLP_STATE_BR_N;
                break;
              }
              case SLP_STATE_BR_R:
              {
                // set new mode
                self->mode = SLP_MODE_BR_RN;
                self->state = SLP_STATE_TEXT;

                // record line break
                cape_parser_line__record (self);
                break;
              }
              case SLP_STATE_BR_N:
              {
                // set new mode
                self->mode = SLP_MODE_BR_N;
                self->state = SLP_STATE_TEXT;

                // record line break
                cape_parser_line__record (self);
                break;
              }
            }
          }
        }
        break;
      }
      default:
      {
        switch (self->state)
        {
          case SLP_STATE_BR_R:
          {
            // set new mode
            self->mode = SLP_MODE_BR_R;
            self->state = SLP_STATE_TEXT;

            // record line break
            cape_parser_line__record (self);
            break;
          }
          case SLP_STATE_BR_N:
          {
            // set new mode
            self->mode = SLP_MODE_BR_N;
            self->state = SLP_STATE_TEXT;

            // record line break
            cape_parser_line__record (self);
            break;
          }
        }
        
        // record char
        cape_stream_append_c (self->stream, *c);
        
        break;
      }
    }
  }
  
  switch (self->state)
  {
    case SLP_STATE_BR_R:
    {
      self->state = SLP_STATE_TEXT;
      
      // record line break
      cape_parser_line__record (self);
      break;
    }
    case SLP_STATE_BR_N:
    {
      self->state = SLP_STATE_TEXT;
      
      // record line break
      cape_parser_line__record (self);
      break;
    }
  }
  
  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

int cape_parser_line_finalize (CapeParserLine self, CapeErr err)
{
  switch (self->state)
  {
    case SLP_STATE_TEXT:
    {
      cape_parser_line__record (self);

      break;
    }
  }

  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

void __STDCALL cape_parser_lines__lines__on_del (void* ptr)
{
  CapeString h = ptr; cape_str_del (&h);
}

//-----------------------------------------------------------------------------

void __STDCALL cape_parser_lines__on_line (void* ptr, const CapeString line)
{
  cape_list_push_back (ptr, cape_str_cp (line));
}

//-----------------------------------------------------------------------------

CapeList cape_parser_lines (const char* buffer, number_t size, CapeErr err)
{
  int res;
  CapeList ret = NULL;

  // local objects
  CapeList lines = cape_list_new (cape_parser_lines__lines__on_del);
  CapeParserLine parser = cape_parser_line_new (lines, cape_parser_lines__on_line);
  
  res = cape_parser_line_process (parser, buffer, size, err);
  if (res)
  {
    goto exit_and_cleanup;
  }

  res = cape_parser_line_finalize (parser, err);
  if (res)
  {
    goto exit_and_cleanup;
  }

  // transfer ownership
  ret = lines;
  lines = NULL;
  
exit_and_cleanup:
  
  cape_list_del (&lines);
  cape_parser_line_del (&parser);

  return ret;
}

//-----------------------------------------------------------------------------
