#include "cape_udc.h"

// cape includes
#include "sys/cape_types.h"
#include "stc/cape_map.h"

//-----------------------------------------------------------------------------

#include <stdlib.h>
#include <stdio.h>

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

CapeUdc cape_udc_new (u_t type, const CapeString name)
{
  CapeUdc self = CAPE_NEW(struct CapeUdc_s);
  
  self->type = type;
  self->data = NULL;
  
  self->name = cape_str_cp (name);
  
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
  }
  
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
  
  cape_str_del (&(self->name));
  
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
  }
  
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
  
  if (self)
  {
    // copy the base type
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
        clone->data = cape_str_cp (self->data);
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
        // allocate memory
        clone->data = CAPE_NEW (double);
        
        // copy the value
        *(double*)(clone->data) = *(double*)(self->data);
        break;
      }
      case CAPE_UDC_DATETIME:
      {
        // allocate memory
        clone->data = cape_datetime_cp (self->data);
        break;
      }
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

void cape_udc_merge_cp__item__node (CapeUdc origin, const CapeUdc other)
{
  if (origin->type == other->type)
  {
    CapeUdcCursor* cursor = cape_udc_cursor_new (other, CAPE_DIRECTION_FORW);
    
    while (cape_udc_cursor_next (cursor))
    {
      CapeUdc h1 = cursor->item;
      CapeUdc h2 = cape_udc_get (origin, h1->name);
      
      if (h2)
      {
        // we found this node in our self node
        cape_udc_merge_cp (h2, h1);
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
    }
  }
}

//-----------------------------------------------------------------------------

const CapeString  cape_udc_name (const CapeUdc self)
{
  return self->name;
}

//-----------------------------------------------------------------------------

u_t cape_udc_type (const CapeUdc self)
{
  return self->type;
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
        h = cape_map_node_value (n);
        
        cape_map_node_del (&n);
        
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

void cape_udc_set_s_cp (CapeUdc self, const CapeString val)
{
  switch (self->type)
  {
    case CAPE_UDC_STRING:
    {
      cape_str_replace_cp ((CapeString*)&(self->data), val);
    }
  }   
}

//-----------------------------------------------------------------------------

void cape_udc_set_s_mv (CapeUdc self, CapeString* p_val)
{
  switch (self->type)
  {
    case CAPE_UDC_STRING:
    {
      cape_str_replace_mv ((CapeString*)&(self->data), p_val);
    }
  }   
}

//-----------------------------------------------------------------------------

void cape_udc_set_n (CapeUdc self, number_t val)
{
  switch (self->type)
  {
    case CAPE_UDC_NUMBER:
    {
      self->data = (void*)val;
    }
  }  
}

//-----------------------------------------------------------------------------

void cape_udc_set_f (CapeUdc self, double val)
{
  switch (self->type)
  {
    case CAPE_UDC_FLOAT:
    {
      double* h = self->data;
      
      *h = val;
    }
  }  
}

//-----------------------------------------------------------------------------

void cape_udc_set_b (CapeUdc self, int val)
{
  switch (self->type)
  {
    case CAPE_UDC_BOOL:
    {
      self->data = val ? (void*)1 : NULL;
      break;
    }
  }  
}

//-----------------------------------------------------------------------------

void cape_udc_set_d (CapeUdc self, const CapeDatetime* val)
{
  switch (self->type)
  {
    case CAPE_UDC_DATETIME:
    {
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
      
      break;
    }
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
      char * pEnd;
      long h = strtol (self->data, &pEnd, 10);
      
      if (pEnd)
      {
        return h;
      }
      else
      {
        return alt;
      }      
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
          cape_udc_del (&h);
          cape_map_node_del (&n);
          
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
        
        CapeUdc u = cape_map_node_value (n);

        // releases the node memory
        cape_map_node_del (&n);
        
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
        CapeUdc h = cape_map_node_value (n);
        
        cape_map_node_del (&n);
        
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
        CapeString h = cape_datetime_s__std (self->data);
        
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
