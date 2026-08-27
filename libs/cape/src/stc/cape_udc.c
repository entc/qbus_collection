#include "cape_udc.h"

// cape includes
#include "sys/cape_types.h"
#include "sys/cape_log.h"
#include "stc/cape_map.h"
#include "fmt/cape_json.h"
//-----------------------------------------------------------------------------
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <errno.h>

#define CAPE_FLOAT_EPSILON 1e-9
//-----------------------------------------------------------------------------

struct CapeUdc_s
{
  u_t type;

  void* data;

  CapeString name;
};

//----------------------------------------------------------------------------------------

static void __STDCALL cape_udc_node_onDel (void* key, void* val)
{
  // don't delete the key, because it is already stored in name

  CapeUdc h = val; cape_udc_del (&h);
}

//----------------------------------------------------------------------------------------

static void __STDCALL cape_udc_list_onDel (void* ptr)
{
  CapeUdc h = ptr; cape_udc_del (&h);
}

//-----------------------------------------------------------------------------

void cape_udc__alloc_data (CapeUdc self)
{
    switch (self->type)
    {
        case CAPE_UDC_NODE:
        {
            self->data = cape_map_new (NULL, cape_udc_node_onDel, NULL);
            break;
        }
        case CAPE_UDC_LIST:
        {
            self->data = cape_list_new (cape_udc_list_onDel);
            break;
        }
        case CAPE_UDC_STRING:
        {
            self->data = NULL;
            break;
        }
        case CAPE_UDC_FLOAT:
        {
            self->data = CAPE_NEW (double);
            break;
        }
        case CAPE_UDC_DATETIME:
        {
            self->data = NULL;
            break;
        }
        case CAPE_UDC_STREAM:
        {
            self->data = NULL;
            break;
        }
        default:
        {
            self->data = NULL;
            break;
        }
    }
}

//-----------------------------------------------------------------------------

void cape_udc__clear_data (CapeUdc self)
{
    switch (self->type)
    {
        case CAPE_UDC_NODE:
        {
            cape_map_del ((CapeMap*)&(self->data));
            break;
        }
        case CAPE_UDC_LIST:
        {
            cape_list_del ((CapeList*)&(self->data));
            break;
        }
        case CAPE_UDC_STRING:
        {
            cape_str_del ((CapeString*)&(self->data));
            break;
        }
        case CAPE_UDC_FLOAT:
        {
            CAPE_DEL (&(self->data), double);
            break;
        }
        case CAPE_UDC_DATETIME:
        {
            cape_datetime_del ((CapeDatetime**)&(self->data));
            break;
        }
        case CAPE_UDC_STREAM:
        {
            cape_stream_del ((CapeStream*)&(self->data));
            break;
        }
        default:
        {
            self->data = NULL;
            break;
        }
    }
}

//-----------------------------------------------------------------------------

CapeUdc cape_udc_new (u_t type, const CapeString name)
{
    CapeUdc self = CAPE_NEW(struct CapeUdc_s);

    self->type = type;
    self->data = NULL;

    self->name = cape_str_cp (name);

    cape_udc__alloc_data (self);

    return self;
}

//-----------------------------------------------------------------------------

void cape_udc_del (CapeUdc* p_self)
{
    CapeUdc self = *p_self;

    if (self == NULL)
    {
        return;
    }

    // clear name
    cape_str_del (&(self->name));

    // clear data
    cape_udc__clear_data (self);

    // release memory
    CAPE_DEL(p_self, struct CapeUdc_s);
}

//-----------------------------------------------------------------------------

static void __STDCALL cape_udc_cp__map_on_clone (void* key_original, void* val_original, void** key_clone, void** val_clone)
{
  // clone the udc object
  CapeUdc cloned_udc = cape_udc_cp (val_original);

  // set the key -> we don't need to copy it (key is owned by the udc)
  *key_clone = (void*)cape_udc_name (cloned_udc);

  // set the cloned udc
  *val_clone = (void*)cloned_udc;
}

//-----------------------------------------------------------------------------

static void* __STDCALL cape_udc_cp__list_on_clone (void* ptr)
{
  return cape_udc_cp (ptr);
}

//-----------------------------------------------------------------------------

CapeUdc cape_udc_cp (const CapeUdc self)
{
    CapeUdc clone = NULL;

    if (NULL == self)
    {
        return NULL;
    }
    
    clone = CAPE_NEW (struct CapeUdc_s);

    clone->type = self->type;
    clone->data = NULL;

    clone->name = cape_str_cp (self->name);

    switch (self->type)
    {
        case CAPE_UDC_NODE:
        {
            clone->data = cape_map_clone (self->data, cape_udc_cp__map_on_clone);
            break;
        }
        case CAPE_UDC_LIST:
        {
            clone->data = cape_list_clone (self->data, cape_udc_cp__list_on_clone);
            break;
        }
        case CAPE_UDC_STRING:
        {
            if (self->data)
            {
                clone->data = cape_str_cp (self->data);
            }
            
            break;
        }
        case CAPE_UDC_NUMBER:
        {
            clone->data = self->data;
            break;
        }
        case CAPE_UDC_BOOL:
        {
            clone->data = self->data;
            break;
        }
        case CAPE_UDC_FLOAT:
        {
            if (self->data)
            {
                // allocate memory
                clone->data = CAPE_NEW (double);

                // copy the value
                *(double*)(clone->data) = *(double*)(self->data);
            }
                
            break;
        }
        case CAPE_UDC_DATETIME:
        {
            if (self->data)
            {
                // allocate memory
                clone->data = cape_datetime_cp (self->data);
            }
            
            break;
        }
        case CAPE_UDC_STREAM:
        {
            if (self->data)
            {
                clone->data = cape_stream_cp (self->data);
            }
            
            break;
        }
        default:
        {
            cape_log_msg (CAPE_LL_WARN, "CAPE", "UDC", "cloning object with unsupported type");

            // return NULL
            cape_udc_del (&clone);
            break;
        }
    }

    return clone;
}

//-----------------------------------------------------------------------------

CapeUdc cape_udc_mv (CapeUdc* p_origin)
{
  CapeUdc ret = *p_origin;

  *p_origin = NULL;

  return ret;
}

//-----------------------------------------------------------------------------

void cape_udc_replace_cp (CapeUdc* p_self, const CapeUdc replace_with_copy)
{
  // remove if exists
  cape_udc_del (p_self);

  // set
  *p_self = cape_udc_cp (replace_with_copy);
}

//-----------------------------------------------------------------------------

void cape_udc_replace_mv (CapeUdc* p_self, CapeUdc* p_replace_with)
{
  // remove if exists
  cape_udc_del (p_self);

  // set
  *p_self = cape_udc_mv (p_replace_with);
}

//-----------------------------------------------------------------------------

void cape_udc_merge_mv__item__node (CapeUdc origin, CapeUdc other)
{
  if (origin->type == other->type)
  {
    CapeUdcCursor* cursor = cape_udc_cursor_new (other, CAPE_DIRECTION_FORW);

    while (cape_udc_cursor_next (cursor))
    {
      CapeUdc h1 = cape_udc_cursor_ext (other, cursor);
      CapeUdc h2 = cape_udc_get (origin, h1->name);

      if (h2)
      {
        // we found this node in our self node
        cape_udc_merge_mv (h2, &h1);
      }
      else
      {
        cape_udc_add (origin, &h1);
      }
    }

    cape_udc_cursor_del (&cursor);
  }
}

//-----------------------------------------------------------------------------

void cape_udc_merge_mv__item__list (CapeUdc origin, CapeUdc other)
{
  if (origin->type == other->type)
  {
    CapeUdcCursor* cursor = cape_udc_cursor_new (other, CAPE_DIRECTION_FORW);

    while (cape_udc_cursor_next (cursor))
    {
      CapeUdc h1 = cape_udc_cursor_ext (other, cursor);

      cape_udc_add (origin, &h1);
    }

    cape_udc_cursor_del (&cursor);
  }
}

//-----------------------------------------------------------------------------

void cape_udc_merge_mv__item (CapeUdc origin, CapeUdc other)
{
    switch (origin->type)
    {
        case CAPE_UDC_NODE:
        {
            cape_udc_merge_mv__item__node (origin, other);
            break;
        }
        case CAPE_UDC_LIST:
        {
            cape_udc_merge_mv__item__list (origin, other);
            break;
        }
        default:
        {
            cape_log_msg (CAPE_LL_WARN, "CAPE", "UDC", "merge object with unsupported type");
            break;
        }
    }
}

//-----------------------------------------------------------------------------

void cape_udc_merge_mv (CapeUdc self, CapeUdc* p_udc)
{
  if (*p_udc)
  {
    cape_udc_merge_mv__item (self, *p_udc);

    cape_udc_del (p_udc);
  }
}

//-----------------------------------------------------------------------------

CapeUdc cape_udc_merge__item (CapeUdc* p_origin, CapeUdc other);

//-----------------------------------------------------------------------------

void cape_udc_merge_cp__item__node (CapeUdc origin, const CapeUdc other)
{
  if (origin->type == other->type)
  {
    CapeUdcCursor* cursor = cape_udc_cursor_new (other, CAPE_DIRECTION_FORW);

    while (cape_udc_cursor_next (cursor))
    {
      CapeUdc h1 = cursor->item;
      CapeUdc h2 = cape_udc_ext (origin, h1->name);

      if (h2)
      {
        CapeUdc h = cape_udc_merge__item (&h2, h1);

        // append
        cape_udc_add (origin, &h);
      }
      else
      {
        // create a copy
        CapeUdc h = cape_udc_cp (h1);

        // append
        cape_udc_add (origin, &h);
      }
    }

    cape_udc_cursor_del (&cursor);
  }
}

//-----------------------------------------------------------------------------

void cape_udc_merge_cp__item__list (CapeUdc origin, const CapeUdc other)
{
  if (origin->type == other->type)
  {
    CapeUdcCursor* cursor = cape_udc_cursor_new (other, CAPE_DIRECTION_FORW);

    while (cape_udc_cursor_next (cursor))
    {
      CapeUdc h1 = cape_udc_cp (cursor->item);

      cape_udc_add (origin, &h1);
    }

    cape_udc_cursor_del (&cursor);
  }
}

//-----------------------------------------------------------------------------

CapeUdc cape_udc_merge__item (CapeUdc* p_origin, CapeUdc other)
{
  CapeUdc ret = cape_udc_mv (p_origin);

  if (ret->type == other->type)
  {
    switch (ret->type)
    {
      case CAPE_UDC_NODE:
      {
        cape_udc_merge_cp__item__node (ret, other);
        break;
      }
      case CAPE_UDC_LIST:
      {
        cape_udc_merge_cp__item__list (ret, other);
        break;
      }
      default:
      {
        cape_udc_replace_cp (&ret, other);
        break;
      }
    }
  }
  else
  {
    cape_udc_replace_cp (&ret, other);
  }

  return ret;
}

//-----------------------------------------------------------------------------

void cape_udc_merge_cp (CapeUdc self, const CapeUdc udc)
{
    if (udc)
    {
        switch (self->type)
        {
            case CAPE_UDC_NODE:
            {
              cape_udc_merge_cp__item__node (self, udc);
              break;
            }
            case CAPE_UDC_LIST:
            {
              cape_udc_merge_cp__item__list (self, udc);
              break;
            }
            default:
            {
                cape_log_msg (CAPE_LL_WARN, "CAPE", "UDC", "merge object with unsupported type");
                break;
            }
        }
    }
}

//-----------------------------------------------------------------------------

void cape_udc_clr (CapeUdc self)
{
    switch (self->type)
    {
        case CAPE_UDC_NODE:
        {
            cape_map_clr (self->data);
            break;
        }
        case CAPE_UDC_LIST:
        {
            cape_list_clr (self->data);
            break;
        }
        default:
        {
            cape_log_msg (CAPE_LL_WARN, "CAPE", "UDC", "clear object with unsupported type");
            break;
        }
    }
}

//-----------------------------------------------------------------------------

void cape_udc_set_type (CapeUdc self, u_t type)
{
    if (type != self->type)
    {
        // clear old data
        cape_udc__clear_data (self);

        // change type to string
        self->type = type;
        
        // initialize the new type
        cape_udc__alloc_data (self);
    }
}

//-----------------------------------------------------------------------------

static int cape_udc__convert_to_s (CapeUdc self)
{
    switch (self->type)
    {
        case CAPE_UDC_STRING:
        {
            // we are fine here
            return TRUE;
        }
        case CAPE_UDC_NUMBER:
        {
            CapeString value = cape_str_n ((number_t)self->data);
            
            if (!value)
            {
                return FALSE;
            }
            
            // change the type
            self->type = CAPE_UDC_STRING;
            self->data = value;

            return TRUE;
        }
        case CAPE_UDC_BOOL:
        {
            CapeString value = cape_str_n ((number_t)self->data);

            if (!value)
            {
                return FALSE;
            }

            // change the type
            self->type = CAPE_UDC_STRING;
            self->data = value;

            return TRUE;
        }
        case CAPE_UDC_FLOAT:
        {
            double* h = self->data;

            CapeString value = cape_str_f (*h);

            if (!value)
            {
                return FALSE;
            }

            // de-allocate old value
            CAPE_DEL (&h, double);

            // change type and set value
            self->type = CAPE_UDC_STRING;
            self->data = value;

            return TRUE;
        }
        case CAPE_UDC_DATETIME:
        {
            CapeDatetime* h = self->data;

            CapeString value = cape_datetime_s__std_msec (h);

            if (!value)
            {
                return FALSE;
            }

            // de-allocate old value
            cape_datetime_del (&h);

            // change type and set value
            self->type = CAPE_UDC_STRING;
            self->data = value;

            return TRUE;
        }
        case CAPE_UDC_NODE:
        {
            CapeString value = cape_json_to_s (self);
            
            if (!value)
            {
                return FALSE;
            }

            // de-allocate old value
            cape_map_del ((CapeMap*)&(self->data));

            // change type and set value
            self->type = CAPE_UDC_STRING;
            self->data = value;

            return TRUE;
        }
        case CAPE_UDC_LIST:
        {
            CapeString value = cape_json_to_s (self);
            
            if (!value)
            {
                return FALSE;
            }

            // de-allocate old value
            cape_list_del ((CapeList*)&(self->data));
            
            // change type and set value
            self->type = CAPE_UDC_STRING;
            self->data = value;

            return TRUE;
        }
    }

    return FALSE;
}

//-----------------------------------------------------------------------------

static int cape_udc__convert_to_n (CapeUdc self)
{
    switch (self->type)
    {
        case CAPE_UDC_NUMBER:
        {
            // we are fine here
            return TRUE;
        }
        case CAPE_UDC_BOOL:
        {
            // change the type
            self->type = CAPE_UDC_NUMBER;

            // we are fine here
            return TRUE;
        }
        case CAPE_UDC_FLOAT:
        {
            double* h = self->data;

            // change type and set value
            self->type = CAPE_UDC_NUMBER;
            self->data = (void*)((number_t)*h);

            CAPE_DEL (&h, double);

            return TRUE;
        }
        case CAPE_UDC_DATETIME:
        {
            CapeDatetime* h = self->data;

            // change type and set value
            self->type = CAPE_UDC_NUMBER;
            self->data = (void*)((number_t)cape_datetime_n__unix (h));  // convert into unix time (seconds since 1970)

            cape_datetime_del (&h);

            return TRUE;
        }
        case CAPE_UDC_STRING:
        {
            if (self->data)
            {
                errno = 0;

                char * pEnd;
                number_t h = strtoll (self->data, &pEnd, 10);

                if ((pEnd != self->data) && (*pEnd == '\0') && (errno != ERANGE))  // convertion was possible
                {
                    cape_str_del ((CapeString*)&(self->data));

                    // change the type
                    self->type = CAPE_UDC_NUMBER;
                    self->data = (void*)h;

                    return TRUE;
                }
            }
            
            break;
        }
    }

    return FALSE;
}

//-----------------------------------------------------------------------------

static int cape_udc__convert_to_f (CapeUdc self)
{
    switch (self->type)
    {
        case CAPE_UDC_FLOAT:
        {
            // we are fine here
            return TRUE;
        }
        case CAPE_UDC_NUMBER:
        case CAPE_UDC_BOOL:
        {
            number_t h = (number_t)(self->data);

            // change the type and allocate memory
            self->type = CAPE_UDC_FLOAT;
            self->data = CAPE_NEW (double);

            // set the value
            *(double*)(self->data) = h;

            return TRUE;
        }
        case CAPE_UDC_DATETIME:
        {
            CapeDatetime* h = self->data;

            // change the type and allocate memory
            self->type = CAPE_UDC_FLOAT;
            self->data = CAPE_NEW (double);

            // set the value
            *(double*)(self->data) = (double)cape_datetime_n__unix (h);  // convert into unix time (seconds since 1970)

            cape_datetime_del (&h);

            return TRUE;
        }
        case CAPE_UDC_STRING:
        {
            if (self->data)
            {
                errno = 0;
                
                char * pEnd;
                double h = strtod (self->data, &pEnd);  // try to convert

                if ((pEnd != self->data) && (*pEnd == '\0') && (errno != ERANGE))  // convertion was possible
                {
                  // cleanup
                  cape_str_del ((CapeString*)&(self->data));

                  // change the type and allocate memory
                  self->type = CAPE_UDC_FLOAT;
                  self->data = CAPE_NEW (double);

                  // set the value
                  *(double*)(self->data) = h;

                  return TRUE;
                }
            }

            break;
        }
    }

    return FALSE;
}

//-----------------------------------------------------------------------------

static int cape_udc__convert_to_b (CapeUdc self)
{
    switch (self->type)
    {
        case CAPE_UDC_BOOL:
        {
            // we are fine here
            return TRUE;
        }
        case CAPE_UDC_NUMBER:
        {
            number_t h = (number_t)(self->data);

            self->type = CAPE_UDC_BOOL;
            self->data = (void*)((number_t)(h ? TRUE : FALSE));

            return TRUE;
        }
        case CAPE_UDC_FLOAT:
        {
            double* p_val = self->data;

            // change type and set value
            self->type = CAPE_UDC_BOOL;

            if (p_val)
            {
                self->data = (void*)((number_t)(*p_val == .0 ? FALSE : TRUE));
                CAPE_DEL (&p_val, double);
            }
            else
            {
                self->data = FALSE;
            }

            return TRUE;
        }
        case CAPE_UDC_STRING:
        {
            if (self->data)
            {
                if (cape_str_equal (self->data, "1"))
                {
                    // cleanup
                    cape_str_del ((CapeString*)&(self->data));

                    // change type and set value
                    self->type = CAPE_UDC_BOOL;
                    self->data = (void*)TRUE;

                    return TRUE;
                }
                else if (cape_str_equal (self->data, "0"))
                {
                    // cleanup
                    cape_str_del ((CapeString*)&(self->data));

                    // change type and set value
                    self->type = CAPE_UDC_BOOL;
                    self->data = FALSE;

                    return TRUE;
                }
            }

            break;
        }
    }

    return FALSE;
}

//-----------------------------------------------------------------------------

static int cape_udc__convert_to_d (CapeUdc self)
{
  switch (self->type)
  {
      case CAPE_UDC_DATETIME:
      {
          // we are fine here
          return TRUE;
      }
      case CAPE_UDC_NUMBER:
      {
          number_t h = (number_t)(self->data);

          CapeDatetime dt;

          // assume the value is in seconds
          cape_datetime_utc__unix (&dt, h);
          
          // change type and set value
          self->type = CAPE_UDC_DATETIME;
          self->data = cape_datetime_cp (&dt);

          return TRUE;
      }
      case CAPE_UDC_STRING:
      {
          if (self->data)
          {
              CapeDatetime dt;

              if (cape_datetime__std_msec (&dt, self->data) || cape_datetime__std_usec (&dt, self->data) || cape_datetime__str_msec (&dt, self->data) || cape_datetime__str (&dt, self->data) || cape_datetime__date_de (&dt, self->data) || cape_datetime__date_iso (&dt, self->data))
              {
                  // cleanup
                  cape_str_del ((CapeString*)&(self->data));

                  // change type and set value
                  self->type = CAPE_UDC_DATETIME;
                  self->data = cape_datetime_cp (&dt);

                  return TRUE;
              }
          }

          break;
      }
  }

  return FALSE;
}

//-----------------------------------------------------------------------------

static int cape_udc__convert_to_node (CapeUdc self)
{
    switch (self->type)
    {
        case CAPE_UDC_STRING:
        {
            if (self->data)
            {
                CapeUdc new_content = cape_json_from_s ((CapeString)self->data);
                
                if (new_content && (cape_udc_type (new_content) == CAPE_UDC_NODE))
                {
                    // free old value
                    cape_str_del ((CapeString*)&(self->data));
                    
                    // transfer new value
                    self->data = cape_mv (&(new_content->data));
                    
                    // set new type
                    self->type = CAPE_UDC_NODE;
                    
                    cape_udc_del (&new_content);
                    return TRUE;
                }

                cape_udc_del (&new_content);
            }
            
            break;
        }
    }

    return FALSE;
}

//-----------------------------------------------------------------------------

static int cape_udc__convert_to_list (CapeUdc self)
{
    switch (self->type)
    {
        case CAPE_UDC_STRING:
        {
            if (self->data)
            {
                CapeUdc value = cape_json_from_s ((CapeString)self->data);
                
                if (NULL == value)
                {
                    cape_log_msg (CAPE_LL_WARN, "CAPE", "UDC", "convert to list: can't deserialize input string");
                    return FALSE;
                }
                
                if (cape_udc_type (value) != CAPE_UDC_LIST)
                {
                    cape_udc_del (&value);
                    
                    cape_log_msg (CAPE_LL_WARN, "CAPE", "UDC", "convert to list: deserialized object is NOT a list");
                    return FALSE;
                }
                
                // free old value
                cape_str_del ((CapeString*)&(self->data));
                
                // transfer new value
                self->data = cape_mv (&(value->data));
                
                // set new type
                self->type = CAPE_UDC_LIST;
                
                cape_udc_del (&value);
                return TRUE;
            }
            
            break;
        }
    }

    return FALSE;
}

//-----------------------------------------------------------------------------

static int cape_udc__convert_data (CapeUdc self, u_t type)
{
    switch (type)
    {
        case CAPE_UDC_STRING:
        {
            return cape_udc__convert_to_s (self);
        }
        case CAPE_UDC_NUMBER:
        {
            return cape_udc__convert_to_n (self);
        }
        case CAPE_UDC_FLOAT:
        {
            return cape_udc__convert_to_f (self);
        }
        case CAPE_UDC_BOOL:
        {
            return cape_udc__convert_to_b (self);
        }
        case CAPE_UDC_DATETIME:
        {
            return cape_udc__convert_to_d (self);
        }
        case CAPE_UDC_NODE:
        {
            return cape_udc__convert_to_node (self);
        }
        case CAPE_UDC_LIST:
        {
            return cape_udc__convert_to_list (self);
        }
        default:
        {
            return FALSE;
        }
    }
}

//-----------------------------------------------------------------------------

int cape_udc_merge_type (CapeUdc self, u_t type)
{
    if (!self)
    {
        return FALSE;
    }
    
    return cape_udc__convert_data (self, type);
}

//-----------------------------------------------------------------------------

const CapeString  cape_udc_name (const CapeUdc self)
{
  return self->name;
}

//-----------------------------------------------------------------------------

u_t cape_udc_type (const CapeUdc self)
{
    return (NULL == self) ? CAPE_UDC_UNDEFINED : self->type;
}

//-----------------------------------------------------------------------------

void* cape_udc_data (const CapeUdc self)
{
  switch (self->type)
  {
    case CAPE_UDC_STRING:
    {
      return self->data;
    }
    case CAPE_UDC_FLOAT:
    {
      return self->data;
    }
    case CAPE_UDC_NUMBER:
    {
      return &(self->data);
    }
    case CAPE_UDC_BOOL:
    {
      return &(self->data);
    }
    case CAPE_UDC_DATETIME:
    {
      return self->data;
    }
  }

  return NULL;
}

//-----------------------------------------------------------------------------

number_t cape_udc_size (const CapeUdc self)
{
  if (self)
  {
    switch (self->type)
    {
      case CAPE_UDC_NODE:
      {
        return cape_map_size (self->data);
      }
      case CAPE_UDC_LIST:
      {
        return cape_list_size (self->data);
      }
      default:
      {
        return 0;
      }
    }
  }

  return 0;
}

//-----------------------------------------------------------------------------

CapeUdc cape_udc_add (CapeUdc self, CapeUdc* p_item)
{
    switch (self->type)
    {
        case CAPE_UDC_NODE:
        {
            CapeUdc h = *p_item;

            cape_map_insert (self->data, h->name, h);

            *p_item = NULL;

            return h;
        }
        case CAPE_UDC_LIST:
        {
            CapeUdc h = *p_item;

            cape_list_push_back (self->data, h);

            *p_item = NULL;

            return h;
        }
        default:
        {
            cape_log_msg (CAPE_LL_WARN, "CAPE", "UDC", "add object on unsupported type");

            // we can't add this item, but we can delete it
            cape_udc_del (p_item);

            return NULL;
        }
    }
}

//-----------------------------------------------------------------------------

void cape_udc_set_name (const CapeUdc self, const CapeString name)
{
  cape_str_replace_cp (&(self->name), name);
}

//-----------------------------------------------------------------------------

CapeUdc cape_udc_add_name (CapeUdc self, CapeUdc* p_item, const CapeString name)
{
  if (*p_item)
  {
    cape_udc_set_name (*p_item, name);

    return cape_udc_add (self, p_item);
  }
  else
  {
    return NULL;
  }
}

//-----------------------------------------------------------------------------

CapeUdc cape_udc_get (CapeUdc self, const CapeString name)
{
  // better to check here
  if (self == NULL)
  {
    return NULL;
  }

  // if we don't have a name we cannot find something
  if (name == NULL)
  {
    return NULL;
  }

  switch (self->type)
  {
    case CAPE_UDC_NODE:
    {
      CapeMapNode n = cape_map_find (self->data, (void*)name);

      if (n)
      {
        return cape_map_node_value (n);
      }
      else
      {
        return NULL;
      }
    }
    default:
    {
      return NULL;
    }
  }
}

//-----------------------------------------------------------------------------

CapeUdc cape_udc_ext (CapeUdc self, const CapeString name)
{
  // better to check here
  if (self == NULL)
  {
    return NULL;
  }

  // if we don't have a name we cannot find something
  if (name == NULL)
  {
    return NULL;
  }

  switch (self->type)
  {
    case CAPE_UDC_NODE:
    {
      CapeMapNode n = cape_map_find (self->data, (void*)name);

      if (n)
      {
        CapeUdc h;

        n = cape_map_extract (self->data, n);
        
        // ransfer ownership of the value
        h = cape_map_node_mv (n);

        cape_map_del_node (self->data, &n);

        return h;
      }
      else
      {
        return NULL;
      }
    }
    default:
    {
      return NULL;
    }
  }
}

//-----------------------------------------------------------------------------

void cape_udc_rm (CapeUdc self, const CapeString name)
{
  // better to check here
  if (self == NULL)
  {
    return;
  }

  // if we don't have a name we cannot find something
  if (name == NULL)
  {
    return;
  }

  switch (self->type)
  {
    case CAPE_UDC_NODE:
    {
      CapeMapNode n = cape_map_find (self->data, (void*)name);

      if (n)
      {
        CapeUdc h;

        n = cape_map_extract (self->data, n);
        h = cape_map_node_mv (n);

        cape_map_del_node (self->data, &n);
        cape_udc_del (&h);
      }

      break;
    }
  }
}

//-----------------------------------------------------------------------------

void cape_udc_set_s_cp (CapeUdc self, const CapeString val)
{
    cape_udc_set_type (self, CAPE_UDC_STRING);

    cape_str_replace_cp ((CapeString*)&(self->data), val);
}

//-----------------------------------------------------------------------------

void cape_udc_set_s_mv (CapeUdc self, CapeString* p_val)
{
    cape_udc_set_type (self, CAPE_UDC_STRING);

    cape_str_replace_mv ((CapeString*)&(self->data), p_val);
}

//-----------------------------------------------------------------------------

void cape_udc_set_n (CapeUdc self, number_t val)
{
    cape_udc_set_type (self, CAPE_UDC_NUMBER);

    self->data = (void*)val;
}

//-----------------------------------------------------------------------------

void cape_udc_set_f (CapeUdc self, double val)
{
    cape_udc_set_type (self, CAPE_UDC_FLOAT);

    {
        double* h = self->data;

        *h = val;
    }
}

//-----------------------------------------------------------------------------

void cape_udc_set_b (CapeUdc self, int val)
{
    cape_udc_set_type (self, CAPE_UDC_BOOL);

    self->data = val ? (void*)1 : NULL;
}

//-----------------------------------------------------------------------------

void cape_udc_set_d (CapeUdc self, const CapeDatetime* val)
{
    cape_udc_set_type (self, CAPE_UDC_DATETIME);

    if (val)
    {
        if (self->data == NULL)
        {
            self->data = cape_datetime_new ();
        }

        memcpy (self->data, val, sizeof(CapeDatetime));
    }
    else
    {
        cape_datetime_del ((CapeDatetime**)&(self->data));
    }
}

//-----------------------------------------------------------------------------

void cape_udc_set_m_cp (CapeUdc self, const CapeStream val)
{
    cape_udc_set_type (self, CAPE_UDC_STREAM);

    if (self->data)
    {
        cape_stream_del ((CapeStream*)&(self->data));
    }
    
    if (val)
    {
        self->data = cape_stream_cp (val);
    }
}

//-----------------------------------------------------------------------------

void cape_udc_set_node_cp (CapeUdc self, const CapeUdc val)
{
    if (self == val)
    {
        // avoid self copying
        return;
    }
    
    if (NULL == self->data)
    {
        // this should not happen
        cape_log_msg (CAPE_LL_ERROR, "CAPE", "UDC", "set node failed, no map object exists");
        return;
    }
    
    if (NULL == val)
    {
        // correct type
        cape_udc_set_type (self, CAPE_UDC_NODE);

        // erase the current map
        cape_map_clr (self->data);

        return;
    }

    if (CAPE_UDC_NODE != val->type)
    {
        // we should return if it is the wrong type
        cape_log_msg (CAPE_LL_WARN, "CAPE", "UDC", "set node failed, value has wrong type");
        return;
    }

    if (NULL == val->data)
    {
        // this should not happen
        cape_log_msg (CAPE_LL_ERROR, "CAPE", "UDC", "set node failed, value has no map object");
        return;
    }
    
    cape_udc_set_type (self, CAPE_UDC_NODE);

    // release map object to replace it
    cape_map_del ((CapeMap*)&(self->data));
    
    // clone a new map object by using the map clone function
    self->data = cape_map_clone (val->data, cape_udc_cp__map_on_clone);
}

//-----------------------------------------------------------------------------

void cape_udc_set_node_mv (CapeUdc self, CapeUdc* p_val)
{
    if (NULL == p_val)
    {
        cape_log_msg (CAPE_LL_ERROR, "CAPE", "UDC", "set node failed, invalid pointer");
        return;
    }
    
    if (NULL == self->data)
    {
        // this should not happen
        cape_log_msg (CAPE_LL_ERROR, "CAPE", "UDC", "set node failed, no map object exists");
        return;
    }

    {
        CapeUdc val = *p_val;

        if (self == val)
        {
            // this is safe because self still holds the object
            *p_val = NULL;
            
            // avoid self copying
            return;
        }

        if (NULL == val)
        {
            cape_udc_set_type (self, CAPE_UDC_NODE);

            // this might be valid to erase the current map
            cape_map_clr (self->data);

            cape_udc_del (p_val);
            
            return;
        }
        
        if (CAPE_UDC_NODE != val->type)
        {
            // we should return if it is the wrong type
            cape_log_msg (CAPE_LL_WARN, "CAPE", "UDC", "set node failed, value has wrong type");
            return;
        }

        if (NULL == val->data)
        {
            // this should not happen
            cape_log_msg (CAPE_LL_ERROR, "CAPE", "UDC", "set node failed, value has no map object");
            return;
        }

        cape_udc_set_type (self, CAPE_UDC_NODE);

        // release map object to replace it
        cape_map_del ((CapeMap*)&(self->data));

        self->data = cape_mv (&(val->data));
    }

    cape_udc_del (p_val);
}

//-----------------------------------------------------------------------------

void cape_udc_set_list_cp (CapeUdc self, const CapeUdc val)
{
    if (self == val)
    {
        // avoid self copying
        return;
    }
    
    if (NULL == self->data)
    {
        // this should not happen
        cape_log_msg (CAPE_LL_ERROR, "CAPE", "UDC", "set list failed, no list object exists");
        return;
    }
    
    if (NULL == val)
    {
        cape_udc_set_type (self, CAPE_UDC_LIST);

        // this might be valid to erase the current list
        cape_list_clr (self->data);

        return;
    }

    if (CAPE_UDC_LIST != val->type)
    {
        // we should return if it is the wrong type
        cape_log_msg (CAPE_LL_WARN, "CAPE", "UDC", "set list failed, value has wrong type");
        return;
    }

    if (NULL == val->data)
    {
        // this should not happen
        cape_log_msg (CAPE_LL_ERROR, "CAPE", "UDC", "set list failed, value has no list object");
        return;
    }
    
    cape_udc_set_type (self, CAPE_UDC_LIST);

    // release list object to replace it
    cape_list_del ((CapeList*)&(self->data));
    
    // clone a new list object by using the list clone function
    self->data = cape_list_clone (val->data, cape_udc_cp__list_on_clone);
}

//-----------------------------------------------------------------------------

void cape_udc_set_list_mv (CapeUdc self, CapeUdc* p_val)
{
    if (NULL == p_val)
    {
        cape_log_msg (CAPE_LL_ERROR, "CAPE", "UDC", "set list failed, invalid pointer");
        return;
    }
    
    if (NULL == self->data)
    {
        // this should not happen
        cape_log_msg (CAPE_LL_ERROR, "CAPE", "UDC", "set list failed, no list object exists");
        return;
    }

    {
        CapeUdc val = *p_val;

        if (self == val)
        {
            // this is safe because self still holds the object
            *p_val = NULL;
            
            // avoid self copying
            return;
        }

        if (NULL == val)
        {
            cape_udc_set_type (self, CAPE_UDC_LIST);

            // this might be valid to erase the current list
            cape_list_clr (self->data);

            cape_udc_del (p_val);
            
            return;
        }
        
        if (CAPE_UDC_LIST != val->type)
        {
            // we should return if it is the wrong type
            cape_log_msg (CAPE_LL_WARN, "CAPE", "UDC", "set list failed, value has wrong type");
            return;
        }

        if (NULL == val->data)
        {
            // this should not happen
            cape_log_msg (CAPE_LL_ERROR, "CAPE", "UDC", "set list failed, value has no list object");
            return;
        }

        cape_udc_set_type (self, CAPE_UDC_LIST);

        // release list object to replace it
        cape_list_del ((CapeList*)&(self->data));

        self->data = cape_mv (&(val->data));
    }

    cape_udc_del (p_val);
}

//-----------------------------------------------------------------------------

void cape_udc_set_m_mv (CapeUdc self, CapeStream* p_val)
{
    cape_udc_set_type (self, CAPE_UDC_STREAM);

    if (self->data)
    {
        cape_stream_del ((CapeStream*)&(self->data));
    }

    if (p_val)
    {
        self->data = cape_mv ((void**)p_val);
    }
}

//-----------------------------------------------------------------------------

const CapeString cape_udc_s (CapeUdc self, const CapeString alt)
{
  switch (self->type)
  {
    case CAPE_UDC_STRING:
    {
      return self->data;
    }
    default:
    {
      return alt;
    }
  }
}

//-----------------------------------------------------------------------------

CapeString cape_udc_s_mv (CapeUdc self, const CapeString alt)
{
  switch (self->type)
  {
    case CAPE_UDC_STRING:
    {
      CapeString h = self->data;

      self->data = NULL;

      return h;
    }
    default:
    {
      return cape_str_cp (alt);
    }
  }
}

//-----------------------------------------------------------------------------

number_t cape_udc_n (CapeUdc self, number_t alt)
{
    switch (self->type)
    {
        case CAPE_UDC_NUMBER:
        {
            return (number_t)(self->data);
        }
        case CAPE_UDC_FLOAT:
        {
            double* h = self->data;
            return *h;
        }
        case CAPE_UDC_STRING:
        {
            return cape_str_to_n (self->data);
        }
        default:
        {
            return alt;
        }
    }
}

//-----------------------------------------------------------------------------

double cape_udc_f (CapeUdc self, double alt)
{
  switch (self->type)
  {
    case CAPE_UDC_NUMBER:
    {
        return (number_t)(self->data);
    }
    case CAPE_UDC_FLOAT:
    {
        double* h = self->data;

        return *h;
    }
    case CAPE_UDC_STRING:
    {
        return cape_str_to_f ((CapeString)self->data);
    }
    default:
    {
        return alt;
    }
  }
}

//-----------------------------------------------------------------------------

int cape_udc_b (CapeUdc self, int alt)
{
  switch (self->type)
  {
    case CAPE_UDC_BOOL:
    {
      return self->data ? TRUE : FALSE;
    }
    default:
    {
      return alt;
    }
  }
}

//-----------------------------------------------------------------------------

const CapeDatetime* cape_udc_d (CapeUdc self, const CapeDatetime* alt)
{
  switch (self->type)
  {
    case CAPE_UDC_DATETIME:
    {
      return self->data;
    }
    default:
    {
      return alt;
    }
  }
}

//-----------------------------------------------------------------------------

CapeDatetime* cape_udc_d_mv (CapeUdc self, const CapeDatetime* alt)
{
  switch (self->type)
  {
    case CAPE_UDC_DATETIME:
    {
      CapeDatetime* h = self->data;

      self->data = NULL;

      return h;
    }
    default:
    {
      return cape_datetime_cp (alt);
    }
  }
}

//-----------------------------------------------------------------------------

CapeList cape_udc_list_mv (CapeUdc self)
{
  switch (self->type)
  {
    case CAPE_UDC_LIST:
    {
      CapeList h = self->data;

      // create an empty list
      self->data = cape_list_new (cape_udc_list_onDel);

      return h;
    }
    default:
    {
      return NULL;
    }
  }
}

//-----------------------------------------------------------------------------

const CapeStream cape_udc_m (CapeUdc self)
{
  switch (self->type)
  {
    case CAPE_UDC_STREAM:
    {
      return self->data;
    }
    default:
    {
      return NULL;
    }
  }
}

//-----------------------------------------------------------------------------

CapeStream cape_udc_m_mv (CapeUdc self)
{
  switch (self->type)
  {
    case CAPE_UDC_STREAM:
    {
      CapeStream h = self->data;

      self->data = NULL;

      return h;
    }
    default:
    {
      return NULL;
    }
  }
}

//-----------------------------------------------------------------------------

CapeUdc cape_udc_add_s_cp (CapeUdc self, const CapeString name, const CapeString val)
{
  CapeUdc h = cape_udc_new (CAPE_UDC_STRING, name);

  cape_udc_set_s_cp (h, val);

  return cape_udc_add (self, &h);
}

//-----------------------------------------------------------------------------

CapeUdc cape_udc_add_s_mv (CapeUdc self, const CapeString name, CapeString* p_val)
{
  CapeUdc h = cape_udc_new (CAPE_UDC_STRING, name);

  cape_udc_set_s_mv (h, p_val);

  return cape_udc_add (self, &h);
}

//-----------------------------------------------------------------------------

CapeUdc cape_udc_add_n (CapeUdc self, const CapeString name, number_t val)
{
  CapeUdc h = cape_udc_new (CAPE_UDC_NUMBER, name);

  cape_udc_set_n (h, val);

  return cape_udc_add (self, &h);
}

//-----------------------------------------------------------------------------

CapeUdc cape_udc_add_f (CapeUdc self, const CapeString name, double val)
{
  CapeUdc h = cape_udc_new (CAPE_UDC_FLOAT, name);

  cape_udc_set_f (h, val);

  return cape_udc_add (self, &h);
}

//-----------------------------------------------------------------------------

CapeUdc cape_udc_add_b (CapeUdc self, const CapeString name, int val)
{
  CapeUdc h = cape_udc_new (CAPE_UDC_BOOL, name);

  cape_udc_set_b (h, val);

  return cape_udc_add (self, &h);
}

//-----------------------------------------------------------------------------

CapeUdc cape_udc_add_d (CapeUdc self, const CapeString name, const CapeDatetime* val)
{
  CapeUdc h = cape_udc_new (CAPE_UDC_DATETIME, name);

  cape_udc_set_d (h, val);

  return cape_udc_add (self, &h);
}

//-----------------------------------------------------------------------------

CapeUdc cape_udc_add_z (CapeUdc self, const CapeString name)
{
  CapeUdc h = cape_udc_new (CAPE_UDC_NULL, name);

  return cape_udc_add (self, &h);
}

//-----------------------------------------------------------------------------

CapeUdc cape_udc_add_node (CapeUdc self, const CapeString name)
{
  CapeUdc h = cape_udc_new (CAPE_UDC_NODE, name);

  return cape_udc_add (self, &h);
}

//-----------------------------------------------------------------------------

CapeUdc cape_udc_add_list (CapeUdc self, const CapeString name)
{
  CapeUdc h = cape_udc_new (CAPE_UDC_LIST, name);

  return cape_udc_add (self, &h);
}

//-----------------------------------------------------------------------------

CapeUdc cape_udc_add_m_cp (CapeUdc self, const CapeString name, const CapeStream val)
{
  CapeUdc h = cape_udc_new (CAPE_UDC_STREAM, name);

  cape_udc_set_m_cp (h, val);

  return cape_udc_add (self, &h);
}

//-----------------------------------------------------------------------------

CapeUdc cape_udc_add_m_mv (CapeUdc self, const CapeString name, CapeStream* p_val)
{
  CapeUdc h = cape_udc_new (CAPE_UDC_STREAM, name);

  cape_udc_set_m_mv (h, p_val);

  return cape_udc_add (self, &h);
}

//-----------------------------------------------------------------------------

CapeUdc cape_udc_add_node_cp (CapeUdc self, const CapeString name, const CapeUdc val)
{
    CapeUdc h = cape_udc_new (CAPE_UDC_NODE, name);

    cape_udc_set_node_cp (h, val);

    return cape_udc_add (self, &h);
}

//-----------------------------------------------------------------------------

CapeUdc cape_udc_add_node_mv (CapeUdc self, const CapeString name, CapeUdc* p_val)
{
    CapeUdc h = cape_udc_new (CAPE_UDC_NODE, name);

    cape_udc_set_node_mv (h, p_val);

    return cape_udc_add (self, &h);
}

//-----------------------------------------------------------------------------

CapeUdc cape_udc_add_list_cp (CapeUdc self, const CapeString name, const CapeUdc val)
{
    CapeUdc h = cape_udc_new (CAPE_UDC_LIST, name);

    cape_udc_set_list_cp (h, val);

    return cape_udc_add (self, &h);
}

//-----------------------------------------------------------------------------

CapeUdc cape_udc_add_list_mv (CapeUdc self, const CapeString name, CapeUdc* p_val)
{
    CapeUdc h = cape_udc_new (CAPE_UDC_LIST, name);

    cape_udc_set_list_mv (h, p_val);

    return cape_udc_add (self, &h);
}

//-----------------------------------------------------------------------------

const CapeString cape_udc_get_s (CapeUdc self, const CapeString name, const CapeString alt)
{
  CapeUdc h = cape_udc_get (self, name);

  if (h)
  {
    return cape_udc_s (h, alt);
  }

  return alt;
}

//-----------------------------------------------------------------------------

number_t cape_udc_get_n (CapeUdc self, const CapeString name, number_t alt)
{
  CapeUdc h = cape_udc_get (self, name);

  if (h)
  {
    return cape_udc_n (h, alt);
  }

  return alt;
}

//-----------------------------------------------------------------------------

double cape_udc_get_f (CapeUdc self, const CapeString name, double alt)
{
  CapeUdc h = cape_udc_get (self, name);

  if (h)
  {
    return cape_udc_f (h, alt);
  }

  return alt;
}

//-----------------------------------------------------------------------------

int cape_udc_get_b (CapeUdc self, const CapeString name, int alt)
{
  CapeUdc h = cape_udc_get (self, name);

  if (h)
  {
    return cape_udc_b (h, alt);
  }

  return alt;
}

//-----------------------------------------------------------------------------

const CapeDatetime* cape_udc_get_d (CapeUdc self, const CapeString name, const CapeDatetime* alt)
{
  CapeUdc h = cape_udc_get (self, name);

  if (h)
  {
    return cape_udc_d (h, alt);
  }

  return alt;
}

//-----------------------------------------------------------------------------

const CapeStream cape_udc_get_m (CapeUdc self, const CapeString name)
{
  CapeUdc h = cape_udc_get (self, name);

  if (h)
  {
    return cape_udc_m (h);
  }

  return NULL;
}

//-----------------------------------------------------------------------------

CapeUdc cape_udc_get_node (CapeUdc self, const CapeString name)
{
  CapeUdc h = cape_udc_get (self, name);

  if (h)
  {
    if (h->type == CAPE_UDC_NODE)
    {
      return h;
    }
  }

  return NULL;
}

//-----------------------------------------------------------------------------

CapeUdc cape_udc_get_list (CapeUdc self, const CapeString name)
{
  CapeUdc h = cape_udc_get (self, name);

  if (h)
  {
    if (h->type == CAPE_UDC_LIST)
    {
      return h;
    }
  }

  return NULL;
}

//-----------------------------------------------------------------------------

void cape_udc_put_s_cp (CapeUdc self, const CapeString name, const CapeString val)
{
    CapeUdc h = cape_udc_get (self, name);
    if (h)
    {
        cape_udc_set_s_cp (h, val);
    }
    else
    {
        cape_udc_add_s_cp (self, name, val);
    }
}

//-----------------------------------------------------------------------------

void cape_udc_put_s_mv (CapeUdc self, const CapeString name, CapeString* p_val)
{
    CapeUdc h = cape_udc_get (self, name);
    if (h)
    {
        cape_udc_set_s_mv (h, p_val);
    }
    else
    {
        cape_udc_add_s_mv (self, name, p_val);
    }
}

//-----------------------------------------------------------------------------

void cape_udc_put_n (CapeUdc self, const CapeString name, number_t val)
{
  CapeUdc h = cape_udc_get (self, name);
  if (h)
  {
    cape_udc_set_n (h, val);
  }
  else
  {
    cape_udc_add_n (self, name, val);
  }
}

//-----------------------------------------------------------------------------

void cape_udc_put_f (CapeUdc self, const CapeString name, double val)
{
  CapeUdc h = cape_udc_get (self, name);
  if (h)
  {
    cape_udc_set_f (h, val);
  }
  else
  {
    cape_udc_add_f (self, name, val);
  }
}

//-----------------------------------------------------------------------------

void cape_udc_put_b (CapeUdc self, const CapeString name, int val)
{
  CapeUdc h = cape_udc_get (self, name);
  if (h)
  {
    cape_udc_set_b (h, val);
  }
  else
  {
    cape_udc_add_b (self, name, val);
  }
}

//-----------------------------------------------------------------------------

void cape_udc_put_m_cp (CapeUdc self, const CapeString name, const CapeStream val)
{
  CapeUdc h = cape_udc_get (self, name);
  if (h)
  {
    cape_udc_set_m_cp (h, val);
  }
  else
  {
    cape_udc_add_m_cp (self, name, val);
  }
}

//-----------------------------------------------------------------------------

void cape_udc_put_m_mv (CapeUdc self, const CapeString name, CapeStream* p_val)
{
  CapeUdc h = cape_udc_get (self, name);
  if (h)
  {
    cape_udc_set_m_mv (h, p_val);
  }
  else
  {
    cape_udc_add_m_mv (self, name, p_val);
  }
}

//-----------------------------------------------------------------------------

void cape_udc_put_node__replace (CapeMapNode n, CapeUdc new_value)
{
  // retrieve current value
  CapeUdc node_to_del = cape_map_node_mv (n);
  
  // replace the name
  cape_str_replace_mv (&(new_value->name), &(node_to_del->name));
  
  // delete current value
  cape_udc_del (&node_to_del);
  
  // override the value in the map node
  cape_map_node_set (n, new_value);
}

//-----------------------------------------------------------------------------

void cape_udc_put_node_cp (CapeUdc self, const CapeString name, CapeUdc node)
{
  switch (self->type)
  {
    case CAPE_UDC_NODE:
    {
      // create a copy of the node
      CapeUdc h = cape_udc_cp (node);
      
      CapeMapNode n = cape_map_find (self->data, (void*)name);
      
      if (n)
      {
        cape_udc_put_node__replace (n, h);
      }
      else
      {
        cape_map_insert (self->data, h->name, h);
      }
    }
  }
}

//-----------------------------------------------------------------------------

void cape_udc_put_node_mv (CapeUdc self, const CapeString name, CapeUdc* p_node)
{
  switch (self->type)
  {
    case CAPE_UDC_NODE:
    {
      CapeMapNode n = cape_map_find (self->data, (void*)name);
      
      if (n)
      {
        cape_udc_put_node__replace (n, cape_udc_mv (p_node));
      }
      else
      {
        CapeUdc h = *p_node;
        
        // set the name in case the udc has a different name
        cape_str_replace_cp (&(h->name), name);

        cape_map_insert (self->data, h->name, h);
        
        *p_node = NULL;
      }
    }
  }
}

//-----------------------------------------------------------------------------

CapeUdc cape_udc_get_first (CapeUdc self)
{
  switch (self->type)
  {
    case CAPE_UDC_LIST:
    {
      CapeListNode n = cape_list_node_front (self->data);

      if (n)
      {
        return cape_list_node_data (n);
      }
      else
      {
        return NULL;
      }
    }
    case CAPE_UDC_NODE:
    {
      CapeMapNode n = cape_map_first (self->data);

      if (n)
      {
        return cape_map_node_value (n);
      }
      else
      {
        return NULL;
      }
    }
    default:
    {
      return NULL;
    }
  }
}

//-----------------------------------------------------------------------------

CapeUdc cape_udc_get_last (CapeUdc self)
{
  switch (self->type)
  {
    case CAPE_UDC_LIST:
    {
      CapeListNode n = cape_list_node_back (self->data);

      if (n)
      {
        return cape_list_node_data (n);
      }
      else
      {
        return NULL;
      }
    }
    case CAPE_UDC_NODE:
    {
      CapeMapNode n = cape_map_last (self->data);

      if (n)
      {
        return cape_map_node_value (n);
      }
      else
      {
        return NULL;
      }
    }
    default:
    {
      return NULL;
    }
  }
}

//-----------------------------------------------------------------------------

CapeUdc cape_udc_find_n__iterate (CapeUdc self, const CapeString name, number_t value)
{
  CapeUdc ret = NULL;
  
  // local objects
  CapeUdcCursor* cursor = cape_udc_cursor_new (self, CAPE_DIRECTION_FORW);
  
  while (cape_udc_cursor_next (cursor))
  {
    CapeUdc seek_node = cape_udc_get (cursor->item, name);
    if (seek_node)
    {
      if ((seek_node->type == CAPE_UDC_NUMBER) && ((number_t)(seek_node->data) == value))
      {
        ret = cursor->item;
        break;
      }
    }
  }
  
  cape_udc_cursor_del (&cursor);
  return ret;
}

//-----------------------------------------------------------------------------

CapeUdc cape_udc_find_n (CapeUdc self, const CapeString name, number_t value)
{
    CapeUdc ret = NULL;

    switch (self->type)
    {
        case CAPE_UDC_LIST:
        case CAPE_UDC_NODE:
        {
            ret = cape_udc_find_n__iterate (self, name, value);
            break;
        }
    }

    return ret;
}

//-----------------------------------------------------------------------------

CapeUdc cape_udc_find_s__iterate (CapeUdc self, const CapeString name, const CapeString value)
{
    CapeUdc ret = NULL;

    // local objects
    CapeUdcCursor* cursor = cape_udc_cursor_new (self, CAPE_DIRECTION_FORW);

    while (cape_udc_cursor_next (cursor))
    {
        CapeUdc seek_node = cape_udc_get (cursor->item, name);
        if (seek_node)
        {
            if ((seek_node->type == CAPE_UDC_STRING) && cape_str_equal ((CapeString)(seek_node->data), value))
            {
                ret = cursor->item;
                break;
            }
        }
    }

    cape_udc_cursor_del (&cursor);
    return ret;
}

//-----------------------------------------------------------------------------

CapeUdc cape_udc_find_s (CapeUdc self, const CapeString name, const CapeString value)
{
    CapeUdc ret = NULL;

    switch (self->type)
    {
        case CAPE_UDC_LIST:
        case CAPE_UDC_NODE:
        {
            ret = cape_udc_find_s__iterate (self, name, value);
            break;
        }
    }

    return ret;
}

//-----------------------------------------------------------------------------

void cape_udc_reduce_s__iterate (CapeUdc self, const CapeString name, const CapeString value)
{
  CapeUdcCursor* cursor = cape_udc_cursor_new (self, CAPE_DIRECTION_FORW);

  while (cape_udc_cursor_next (cursor))
  {
    // compare user ids
    if (cape_str_equal (value, cape_udc_get_s (cursor->item, name, NULL)))
    {
      CapeUdc h = cape_udc_cursor_ext (self, cursor);
      cape_udc_del (&h);
      
      break;
    }
  }
  
  cape_udc_cursor_del (&cursor);
}

//-----------------------------------------------------------------------------

void cape_udc_reduce_s (CapeUdc self, const CapeString name, const CapeString value)
{
  switch (self->type)
  {
    case CAPE_UDC_LIST:
    case CAPE_UDC_NODE:
    {
      cape_udc_reduce_s__iterate (self, name, value);
      break;
    }
  }
}

//-----------------------------------------------------------------------------

int cape_udc_equal (CapeUdc self, CapeUdc other)
{
    int ret = FALSE;

    switch (self->type)
    {
        case CAPE_UDC_LIST:
        {
            if (other->type == CAPE_UDC_LIST)
            {
                CapeListCursor cursor1; cape_list_cursor_init (self->data, &cursor1, CAPE_DIRECTION_FORW);
                CapeListCursor cursor2; cape_list_cursor_init (other->data, &cursor2, CAPE_DIRECTION_FORW);

                ret = TRUE;

                while (ret && cape_list_cursor_next (&cursor1))
                {
                    if (cape_list_cursor_next (&cursor2))
                    {
                        ret = cape_udc_equal (cape_list_node_data (cursor1.node), cape_list_node_data (cursor2.node));
                    }
                    else
                    {
                        ret = FALSE;
                    }
                }

                if (ret && cape_list_cursor_next (&cursor2))
                {
                    ret = FALSE;
                }
            }

            break;
        }
        case CAPE_UDC_NODE:
        {
            if (other->type == CAPE_UDC_NODE)
            {
                CapeMapCursor cursor1; cape_map_cursor_init (self->data, &cursor1, CAPE_DIRECTION_FORW);
                CapeMapCursor cursor2; cape_map_cursor_init (other->data, &cursor2, CAPE_DIRECTION_FORW);

                ret = TRUE;

                while (ret && cape_map_cursor_next (&cursor1))
                {
                    if (cape_map_cursor_next (&cursor2))
                    {
                        ret = cape_udc_equal (cape_map_node_value (cursor1.node), cape_map_node_value (cursor2.node));
                    }
                    else
                    {
                        ret = FALSE;
                    }
                }
                
                if (ret && cape_map_cursor_next (&cursor2))
                {
                    ret = FALSE;
                }
            }

            break;
        }
        case CAPE_UDC_STRING:
        {
            if (other->type == CAPE_UDC_STRING)
            {
                ret = cape_str_equal (self->data, other->data);
            }

            break;
        }
        case CAPE_UDC_NUMBER:
        {
            if (other->type == CAPE_UDC_NUMBER)
            {
                ret = ((number_t)(self->data) == (number_t)(other->data));
            }

            break;
        }
        case CAPE_UDC_FLOAT:
        {
            if (other->type == CAPE_UDC_FLOAT)
            {
                double* d1 = self->data;
                double* d2 = other->data;

                if (d1 && d2)
                {
                    double diff = fabs(*d1 - *d2);
                    double scale = fmax(fabs(*d1), fabs(*d2));
                    
                    ret = (diff <= CAPE_FLOAT_EPSILON * (scale > 1.0 ? scale : 1.0));
                }
            }

            break;
        }
        case CAPE_UDC_BOOL:
        {
            if (other->type == CAPE_UDC_BOOL)
            {
                ret = ((number_t)(self->data) == (number_t)(other->data));
            }

            break;
        }
        case CAPE_UDC_DATETIME:
        {
            if (other->type == CAPE_UDC_DATETIME)
            {
                ret = (0 == cape_datetime_cmp (self->data, other->data));
            }

            break;
        }
        case CAPE_UDC_STREAM:
        {
            // TODO: compare 2 streams

            break;
        }
    }

    return ret;
}

//-----------------------------------------------------------------------------

int cape_udc_has__iterate (CapeUdc self, CapeUdc to_find)
{
    int ret = FALSE;

    CapeUdcCursor* cursor = cape_udc_cursor_new (to_find, CAPE_DIRECTION_FORW);

    while (cape_udc_cursor_next (cursor))
    {
        // try to find a node with the same name
        CapeUdc node_found = cape_udc_get (self, cape_udc_name (cursor->item));
        
        if (node_found)
        {
            ret = cape_udc_equal (cursor->item, node_found);
        }
    }

    cape_udc_cursor_del (&cursor);

    return ret;
}

//-----------------------------------------------------------------------------

int cape_udc_has__node (CapeUdc self, CapeUdc to_find)
{
    int ret = FALSE;

    switch (to_find->type)
    {
        case CAPE_UDC_NODE:
        {
            ret = cape_udc_has__iterate (self, to_find);
            break;
        }
    }

    return ret;
}

//-----------------------------------------------------------------------------

int cape_udc_has (CapeUdc self, CapeUdc to_find)
{
    int ret = FALSE;

    switch (self->type)
    {
        case CAPE_UDC_NODE:
        {
            ret = cape_udc_has__node (self, to_find);
            break;
        }
    }

    return ret;
}

//-----------------------------------------------------------------------------

CapeMap cape_udc_map_n (CapeUdc self, const CapeString key_name)
{
    CapeMap ret = NULL;
    
    if (self == NULL)
    {
        return NULL;
    }
    
    switch (self->type)
    {
        case CAPE_UDC_LIST:
        {
            ret = cape_map_new (cape_map__compare__n, cape_udc_node_onDel, NULL);

            {
                CapeUdcCursor* cursor = cape_udc_cursor_new (self, CAPE_DIRECTION_FORW);
                
                while (cape_udc_cursor_next (cursor))
                {
                    number_t key = cape_udc_get_n (cursor->item, key_name, 0);
                    
                    CapeMapNode n = cape_map_find (ret, (void*)key);
                    
                    CapeUdc list;
                    
                    if (n)
                    {
                        list = cape_map_node_value (n);
                    }
                    else
                    {
                        list = cape_udc_new (CAPE_UDC_LIST, NULL);
                        
                        cape_map_insert (ret, (void*)key, list);
                    }

                    {
                        CapeUdc item = cape_udc_cursor_ext (self, cursor);

                        cape_udc_add (list, &item);
                    }
                }

                cape_udc_cursor_del (&cursor);
            }

            break;
        }
    }

    return ret;
}

//-----------------------------------------------------------------------------

CapeString cape_udc_ext_s (CapeUdc self, const CapeString name)
{
  switch (self->type)
  {
    case CAPE_UDC_NODE:
    {
      CapeMapNode n = cape_map_find (self->data, (void*)name);
      if (n)
      {
        CapeUdc h = cape_map_node_value (n);

        if (h->type == CAPE_UDC_STRING)
        {
          CapeString ret;

          // remove the UDC (h) from the map
          n = cape_map_extract (self->data, n);

          // get the content
          ret = h->data;
          h->data = NULL;

          // clean up
          cape_map_del_node (self->data, &n);

          return ret;
        }
      }

      return NULL;
    }
    default:
    {
      return NULL;
    }
  }
}

//-----------------------------------------------------------------------------

CapeDatetime* cape_udc_ext_d (CapeUdc self, const CapeString name)
{
  switch (self->type)
  {
    case CAPE_UDC_NODE:
    {
      CapeMapNode n = cape_map_find (self->data, (void*)name);
      if (n)
      {
        CapeUdc h = cape_map_node_value (n);

        if (h->type == CAPE_UDC_DATETIME)
        {
          CapeDatetime* ret;

          // remove the UDC (h) from the map
          n = cape_map_extract (self->data, n);

          // get the content
          ret = h->data;
          h->data = NULL;

          // clean up
          cape_map_del_node (self->data, &n);

          return ret;
        }
      }

      return NULL;
    }
    default:
    {
      return NULL;
    }
  }
}

//-----------------------------------------------------------------------------

CapeStream cape_udc_ext_m (CapeUdc self, const CapeString name)
{
  switch (self->type)
  {
    case CAPE_UDC_NODE:
    {
      CapeMapNode n = cape_map_find (self->data, (void*)name);
      if (n)
      {
        CapeUdc h = cape_map_node_value (n);

        if (h->type == CAPE_UDC_STREAM)
        {
          CapeStream ret;

          // remove the UDC (h) from the map
          n = cape_map_extract (self->data, n);

          // get the content
          ret = h->data;
          h->data = NULL;

          // clean up
          cape_map_del_node (self->data, &n);

          return ret;
        }
      }

      return NULL;
    }
    default:
    {
      return NULL;
    }
  }
}

//-----------------------------------------------------------------------------

CapeUdc cape_udc_ext_node (CapeUdc self, const CapeString name)
{
  switch (self->type)
  {
    case CAPE_UDC_NODE:
    {
      CapeUdc h = cape_udc_get (self, name);

      if (h)
      {
        if (h->type == CAPE_UDC_NODE)
        {
          return cape_udc_ext (self, name);
        }
      }
    }
    default:
    {
      return NULL;
    }
  }
}

//-----------------------------------------------------------------------------

CapeUdc cape_udc_ext_list (CapeUdc self, const CapeString name)
{
  switch (self->type)
  {
    case CAPE_UDC_NODE:
    {
      CapeUdc h = cape_udc_get (self, name);

      if (h)
      {
        if (h->type == CAPE_UDC_LIST)
        {
          return cape_udc_ext (self, name);
        }
      }
    }
    default:
    {
      return NULL;
    }
  }
}

//-----------------------------------------------------------------------------

CapeUdc cape_udc_ext_first (CapeUdc self)
{
  switch (self->type)
  {
    case CAPE_UDC_LIST:
    {
      CapeListCursor cursor; cape_list_cursor_init (self->data, &cursor, CAPE_DIRECTION_FORW);

      if (cape_list_cursor_next (&cursor))
      {
        return cape_list_node_extract (self->data, cursor.node);
      }
      else
      {
        return NULL;
      }
    }
    case CAPE_UDC_NODE:
    {
      CapeMapCursor cursor; cape_map_cursor_init (self->data, &cursor, CAPE_DIRECTION_FORW);

      if (cape_map_cursor_next (&cursor))
      {
        CapeMapNode n = cape_map_cursor_extract (self->data, &cursor);

        // transfer ownership
        CapeUdc u = cape_map_node_mv (n);

        // releases the node memory
        cape_map_del_node (self->data, &n);

        return u;
      }
      else
      {
        return NULL;
      }
    }
    default:
    {
      return NULL;
    }
  }
}

//-----------------------------------------------------------------------------

CapeUdcCursor* cape_udc_cursor_new (CapeUdc self, int direction)
{
  CapeUdcCursor* cursor = CAPE_NEW(CapeUdcCursor);

  cursor->direction = direction;
  cursor->position = -1;
  cursor->item = NULL;

  switch (self->type)
  {
    case CAPE_UDC_NODE:
    {
      cursor->data = cape_map_cursor_create (self->data, direction);
      cursor->type = CAPE_UDC_NODE;

      break;
    }
    case CAPE_UDC_LIST:
    {
      cursor->data = cape_list_cursor_create (self->data, direction);
      cursor->type = CAPE_UDC_LIST;

      break;
    }
    default:
    {
      cursor->data = NULL;
      cursor->type = 0;

      break;
    }
  }

  return cursor;
}

//-----------------------------------------------------------------------------

void cape_udc_cursor_del (CapeUdcCursor** p_cursor)
{
  if (*p_cursor)
  {
    CapeUdcCursor* cursor = *p_cursor;

    switch (cursor->type)
    {
      case CAPE_UDC_NODE:
      {
        cape_map_cursor_destroy ((CapeMapCursor**)&(cursor->data));
        break;
      }
      case CAPE_UDC_LIST:
      {
        cape_list_cursor_destroy ((CapeListCursor**)&(cursor->data));
        break;
      }
    }

    CAPE_DEL(p_cursor, CapeUdcCursor);
  }
}

//-----------------------------------------------------------------------------

int cape_udc_cursor_next (CapeUdcCursor* cursor)
{
  if (cursor->data)
  {
    switch (cursor->type)
    {
      case CAPE_UDC_NODE:
      {
        CapeMapCursor* c = cursor->data;

        int res = cape_map_cursor_next (c);

        if (res)
        {
          cursor->position++;
          cursor->item = cape_map_node_value (c->node);
        }

        return res;
      }
      case CAPE_UDC_LIST:
      {
        CapeListCursor* c = cursor->data;

        int res = cape_list_cursor_next (c);

        if (res)
        {
          cursor->position++;
          cursor->item = cape_list_node_data (c->node);
        }

        return res;
      }
    }
  }

  return FALSE;
}

//-----------------------------------------------------------------------------

int cape_udc_cursor_prev (CapeUdcCursor* cursor)
{
  if (cursor->data)
  {
    switch (cursor->type)
    {
      case CAPE_UDC_NODE:
      {
        CapeMapCursor* c = cursor->data;

        int res = cape_map_cursor_prev (c);

        if (res)
        {
          cursor->position--;
          cursor->item = cape_map_node_value (c->node);
        }

        return res;
      }
      case CAPE_UDC_LIST:
      {
        CapeListCursor* c = cursor->data;

        int res = cape_list_cursor_prev (c);

        if (res)
        {
          cursor->position--;
          cursor->item = cape_list_node_data (c->node);
        }

        return res;
      }
    }
  }

  return FALSE;
}

//-----------------------------------------------------------------------------

void cape_udc_cursor_rm (CapeUdc self, CapeUdcCursor* cursor)
{
  CapeUdc h = cape_udc_cursor_ext (self, cursor);
  cape_udc_del (&h);
}

//-----------------------------------------------------------------------------

CapeUdc cape_udc_cursor_ext (CapeUdc self, CapeUdcCursor* cursor)
{
  switch (self->type)
  {
    case CAPE_UDC_NODE:
    {
      CapeMapNode n = cape_map_cursor_extract (self->data, cursor->data);

      if (n)
      {
        CapeUdc h = cape_map_node_mv (n);

        cape_map_del_node (self->data, &n);

        return h;
      }

      break;
    }
    case CAPE_UDC_LIST:
    {
      return cape_list_cursor_extract (self->data, cursor->data);
    }
  }

  return NULL;
}

//-----------------------------------------------------------------------------

void cape_udc_print (const CapeUdc self)
{
  switch (self->type)
  {
    case CAPE_UDC_NODE:
    {
      CapeUdcCursor* cursor = cape_udc_cursor_new (self, CAPE_DIRECTION_FORW);

      while (cape_udc_cursor_next (cursor))
      {
        cape_udc_print (cursor->item);
      }

      cape_udc_cursor_del (&cursor);

      break;
    }
    case CAPE_UDC_LIST:
    {


      break;
    }
    case CAPE_UDC_STRING:
    {
      if (self->data)
      {
        printf ("UDC [string] : %s\n", (char*)self->data);
      }
      else
      {
        printf ("UDC [string] : NULL\n");
      }

      break;
    }
    case CAPE_UDC_NUMBER:
    {
      printf ("UDC [number] : %ld\n", (number_t)(self->data));

      break;
    }
    case CAPE_UDC_DATETIME:
    {
      if (self->data)
      {
        CapeString h = cape_datetime_s__std_msec (self->data);

        printf ("UDC [datetime]: %s\n", h);

        cape_str_del (&h);
      }
      else
      {
        printf ("UDC [datetime]: NULL\n");
      }

      break;
    }
  }
}

//-----------------------------------------------------------------------------

void cape_udc_sort_list (CapeUdc self, fct_cape_udc__on_compare on_compare)
{
  switch (self->type)
  {
    case CAPE_UDC_LIST:
    {
      cape_list_sort (self->data, (fct_cape_list_onCompare)on_compare);
      break;
    }
  }
}

//-----------------------------------------------------------------------------

static int __STDCALL cape_udc_list_distinct__on_compare (void* ptr1, void* ptr2)
{
    return cape_udc_equal ((CapeUdc)ptr1, (CapeUdc)ptr2);
}

//-----------------------------------------------------------------------------

void cape_udc_list_distinct (CapeUdc self)
{
    if (!self)
    {
        return;
    }
    
    switch (self->type)
    {
        case CAPE_UDC_LIST:
        {
            cape_list_distinct (self->data, cape_udc_list_distinct__on_compare);
            break;
        }
    }
}

//-----------------------------------------------------------------------------

void cape_udc_add_n__max (CapeUdc self, const CapeString name, number_t val, number_t max_length)
{
  switch (self->type)
  {
    case CAPE_UDC_LIST:
    {
      CapeUdc h = cape_udc_new (CAPE_UDC_NUMBER, name);

      cape_udc_set_n (h, val);

      cape_list_push_max (self->data, (void*)h, max_length);

      break;
    }
  }
}

//-----------------------------------------------------------------------------

void cape_udc_add_map (CapeUdc self, CapeMap map)
{
    switch (self->type)
    {
        case CAPE_UDC_NODE:
        {
        
            break;
        }
        case CAPE_UDC_LIST:
        {
            CapeMapCursor* cursor = cape_map_cursor_new (map, CAPE_DIRECTION_FORW);
            
            while (cape_map_cursor_next (cursor))
            {
                // extract the node from the map
                CapeMapNode n = cape_map_cursor_extract (map, cursor);
                
                // transfer ownership of the object
                CapeUdc h = cape_map_node_mv (n);
                
                cape_udc_add (self, &h);
                cape_map_del_node (map, &n);
            }
            
            cape_map_cursor_del (&cursor);
            
            break;
        }
        default:
        {
            cape_log_msg (CAPE_LL_WARN, "CAPE", "UDC", "cape_udc_add_map only supports LIST and NODE");
            break;
        }
    }
}

//-----------------------------------------------------------------------------
