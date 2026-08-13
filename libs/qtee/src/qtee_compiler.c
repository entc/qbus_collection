#include "qtee_compiler.h"
#include "qtee_eval.h"
#include "qtee_format.h"

#include <fcntl.h>
#include <math.h>

// cape includes
#include "stc/cape_stream.h"
#include "stc/cape_map.h"
#include "stc/cape_list.h"
#include "sys/cape_log.h"
#include "sys/cape_time.h"
#include "sys/cape_file.h"
#include "fmt/cape_tokenizer.h"
#include "fmt/cape_json.h"

//-----------------------------------------------------------------------------

struct QTeeCompiler_s
{
  
    int state;
    
    CapeStream sb;
    
    QTeePart part;   // reference

};

//-----------------------------------------------------------------------------

QTeeCompiler qtee_compiler_new (QTeePart part)
{
    QTeeCompiler self = CAPE_NEW (struct QTeeCompiler_s);
  
    self->state = 0;
    self->sb = cape_stream_new ();
  
    self->part = part;
  
    return self;
}

//-----------------------------------------------------------------------------

void qtee_compiler_del (QTeeCompiler* p_self)
{
    if (*p_self)
    {
        QTeeCompiler self = *p_self;
      
        cape_stream_del (&(self->sb));
      
        CAPE_DEL (p_self, struct QTeeCompiler_s);
    }
}

//-----------------------------------------------------------------------------

void qtee_compiler_module__parse (const CapeString buf, CapeString* p_name, CapeString* p_cont, CapeString *p_fomt)
{
  number_t size = cape_str_size (buf);
  
  const char* c = buf;
  int i;
  number_t state = 0;

  // local objects
  CapeStream s = cape_stream_new ();

  for (i = 0; i < size; i++, c++)
  {
    switch (state)
    {
      case 0:
      {
        if (*c == '{')
        {
          *p_name = cape_str_sub (buf, i);
          state = 1;
        }

        break;
      }
      case 1:
      {
        if (*c == '}')
        {
          goto exit_and_cleanup;
        }
        else
        {
          cape_stream_append_c (s, *c);
        }

        break;
      }
    }
  }
  
exit_and_cleanup:
  
  *p_fomt = cape_str_sub (c, size - i);
  *p_cont = cape_stream_to_str (&s);
}

//-----------------------------------------------------------------------------

void qtee_compiler_module (QTeeCompiler self, const CapeString raw_name)
{
    // local objects
    CapeString name = NULL;
    CapeString cont = NULL;
    CapeString fomt = NULL;
    
    // use a small parser to retrieve the 3 string needed for a part module
    qtee_compiler_module__parse (raw_name, &name, &cont, &fomt);

    {
        QTeePart tmplpart = qtee_part_new (PART_TYPE_MOD, fomt, self->part);

        qtee_part_set (tmplpart, &cont, &name);
        
        // add tmplpart as child
        qtee_part_add (self->part, tmplpart);
    }

    cape_str_del (&name);
    cape_str_del (&cont);
    cape_str_del (&fomt);
}

//-----------------------------------------------------------------------------

void qtee_compiler_part (QTeeCompiler self, int type)
{
    switch (type)
    {
        case PART_TYPE_TEXT:
        case PART_TYPE_FILE:
        {
            if (cape_stream_size (self->sb) > 0)
            {
                const CapeString text = cape_stream_get (self->sb);
                
                qtee_part_add (self->part, qtee_part_new (type, text, NULL));
                cape_stream_clr (self->sb);
            }
            
            break;
        }
        case PART_TYPE_TAG:
        {
            const CapeString name = cape_stream_get (self->sb);
            switch (name[0])
            {
              case '#':
              {
                  QTeePart new_part = qtee_part_new (type, name + 1, self->part);
                  
                  // add the new part to the current part
                  qtee_part_add (self->part, new_part);
                  
                  // now change the current part to the new part, that we go one level up
                  self->part = new_part;
                  
                  break;
              }
              case '/':
              {
                  // is the current part the ending tag
                  if (qtee_part_equal (self->part, name + 1))
                  {
                      // has the current part a parent
                      QTeePart parent_part = qtee_part_parent (self->part);
                      if (parent_part)
                      {
                          // change back the current part to the parent, that we go one level down
                          self->part = parent_part;
                      }
                  }

                  break;
              }
              case '$':   // extra modules
              {
                  qtee_compiler_module (self, name);
                  
                  break;
              }
              default:
              {
                  qtee_part_add (self->part, qtee_part_new (type, name, self->part));
                  
                  break;
              }
            }
            
            // always clear
            cape_stream_clr (self->sb);
            
            break;
        }
    }
}

//-----------------------------------------------------------------------------

#define QTEE_TEMPLATE_PARSE_TYPE__NONE            0
#define QTEE_TEMPLATE_PARSE_TYPE__ITEM_PRE        1
#define QTEE_TEMPLATE_PARSE_TYPE__ITEM            2
#define QTEE_TEMPLATE_PARSE_TYPE__ITEM_RET        3
#define QTEE_TEMPLATE_PARSE_TYPE__FILE_PRE        4
#define QTEE_TEMPLATE_PARSE_TYPE__FILE            5
#define QTEE_TEMPLATE_PARSE_TYPE__FILE_RET        6
#define QTEE_TEMPLATE_PARSE_TYPE__ESCS_PRE        7
#define QTEE_TEMPLATE_PARSE_TYPE__ESCS            8
#define QTEE_TEMPLATE_PARSE_TYPE__ESCS_RET        9
#define QTEE_TEMPLATE_PARSE_TYPE__SPECIAL        10

#define QTEE_TEMPLATE_SYNTAX_ITEM_S              '{'
#define QTEE_TEMPLATE_SYNTAX_ITEM_E              '}'
#define QTEE_TEMPLATE_SYNTAX_FILE_S              '['
#define QTEE_TEMPLATE_SYNTAX_FILE_E              ']'
#define QTEE_TEMPLATE_SYNTAX_ESCS_S              '('
#define QTEE_TEMPLATE_SYNTAX_ESCS_E              ')'

//-----------------------------------------------------------------------------

void qtee_compiler_parse (QTeeCompiler self, const char* buffer, number_t size)
{
    const char* c = buffer;
    int i;
    
    for (i = 0; i < size; i++, c++)
    {
        switch (self->state)
        {
            case QTEE_TEMPLATE_PARSE_TYPE__NONE:
            {
                // start with a simple devider
                if (QTEE_TEMPLATE_SYNTAX_ITEM_S == *c)
                {
                    self->state = QTEE_TEMPLATE_PARSE_TYPE__ITEM_PRE;
                }
                else if (QTEE_TEMPLATE_SYNTAX_FILE_S == *c)
                {
                    self->state = QTEE_TEMPLATE_PARSE_TYPE__FILE_PRE;
                }
                else if (QTEE_TEMPLATE_SYNTAX_ESCS_S == *c)
                {
                    self->state = QTEE_TEMPLATE_PARSE_TYPE__ESCS_PRE;
                }
                else
                {
                    cape_stream_append_c (self->sb, *c);
                }

                break;
            }
            case QTEE_TEMPLATE_PARSE_TYPE__ITEM_PRE:
            {
                if (QTEE_TEMPLATE_SYNTAX_ITEM_S == *c)
                {
                    qtee_compiler_part (self, PART_TYPE_TEXT);

                    self->state = QTEE_TEMPLATE_PARSE_TYPE__ITEM;
                }
                else
                {
                    self->state = QTEE_TEMPLATE_PARSE_TYPE__NONE;
                    
                    // add missing
                    cape_stream_append_c (self->sb, QTEE_TEMPLATE_SYNTAX_ITEM_S);
                    cape_stream_append_c (self->sb, *c);
                }

                break;
            }
            case QTEE_TEMPLATE_PARSE_TYPE__ITEM:
            {
                if (QTEE_TEMPLATE_SYNTAX_ITEM_E == *c)
                {
                    self->state = QTEE_TEMPLATE_PARSE_TYPE__ITEM_RET;
                }
                else
                {
                    cape_stream_append_c (self->sb, *c);
                }

                break;
            }
            case QTEE_TEMPLATE_PARSE_TYPE__ITEM_RET:
            {
                if (QTEE_TEMPLATE_SYNTAX_ITEM_E == *c)
                {
                    qtee_compiler_part (self, PART_TYPE_TAG);
                    
                    self->state = QTEE_TEMPLATE_PARSE_TYPE__NONE;
                }
                else
                {
                    self->state = QTEE_TEMPLATE_PARSE_TYPE__ITEM;
                    
                    // add missing
                    cape_stream_append_c (self->sb, QTEE_TEMPLATE_SYNTAX_ITEM_E);
                    cape_stream_append_c (self->sb, *c);
                }

                break;
            }
            case QTEE_TEMPLATE_PARSE_TYPE__FILE_PRE:
            {
                if (QTEE_TEMPLATE_SYNTAX_FILE_S == *c)
                {
                    qtee_compiler_part (self, PART_TYPE_TEXT);
                    
                    self->state = QTEE_TEMPLATE_PARSE_TYPE__FILE;
                }
                else
                {
                    self->state = QTEE_TEMPLATE_PARSE_TYPE__NONE;
                    
                    // add missing
                    cape_stream_append_c (self->sb, QTEE_TEMPLATE_SYNTAX_FILE_S);
                    cape_stream_append_c (self->sb, *c);
                }

                break;
            }
            case QTEE_TEMPLATE_PARSE_TYPE__FILE:
            {
                if (QTEE_TEMPLATE_SYNTAX_FILE_E == *c)
                {
                    self->state = QTEE_TEMPLATE_PARSE_TYPE__FILE_RET;
                }
                else
                {
                    cape_stream_append_c (self->sb, *c);
                }

                break;
            }
            case QTEE_TEMPLATE_PARSE_TYPE__FILE_RET:
            {
                if (QTEE_TEMPLATE_SYNTAX_FILE_E == *c)
                {
                    qtee_compiler_part (self, PART_TYPE_FILE);

                    self->state = QTEE_TEMPLATE_PARSE_TYPE__NONE;
                }
                else
                {
                    // add missing
                    cape_stream_append_c (self->sb, QTEE_TEMPLATE_SYNTAX_FILE_E);
                    cape_stream_append_c (self->sb, *c);

                    self->state = QTEE_TEMPLATE_PARSE_TYPE__FILE;
                }

                break;
            }
            case QTEE_TEMPLATE_PARSE_TYPE__ESCS_PRE:
            {
                if (QTEE_TEMPLATE_SYNTAX_ESCS_S == *c)
                {
                    self->state = QTEE_TEMPLATE_PARSE_TYPE__ESCS;
                }
                else
                {
                    self->state = QTEE_TEMPLATE_PARSE_TYPE__NONE;

                    // add missing
                    cape_stream_append_c (self->sb, QTEE_TEMPLATE_SYNTAX_ESCS_S);
                    cape_stream_append_c (self->sb, *c);
                }

                break;
            }
            case QTEE_TEMPLATE_PARSE_TYPE__ESCS:
            {
                if (QTEE_TEMPLATE_SYNTAX_ESCS_E == *c)
                {
                    self->state = QTEE_TEMPLATE_PARSE_TYPE__ESCS_RET;
                }
                else
                {
                    cape_stream_append_c (self->sb, *c);
                }

                break;
            }
            case QTEE_TEMPLATE_PARSE_TYPE__ESCS_RET:
            {
                if (QTEE_TEMPLATE_SYNTAX_ESCS_E == *c)
                {
                    self->state = QTEE_TEMPLATE_PARSE_TYPE__NONE;
                }
                else
                {
                    self->state = QTEE_TEMPLATE_PARSE_TYPE__ESCS;
                    
                    // add missing
                    cape_stream_append_c (self->sb, QTEE_TEMPLATE_SYNTAX_ESCS_E);
                    cape_stream_append_c (self->sb, *c);
                }

                break;
            }
        }
    }
    
    // add last part as text
    qtee_compiler_part (self, PART_TYPE_TEXT);
}

//-----------------------------------------------------------------------------
