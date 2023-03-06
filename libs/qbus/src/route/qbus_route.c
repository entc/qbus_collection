#include "qbus_route.h"

// cape includes
#include <sys/cape_file.h>
#include <sys/cape_log.h>
#include <sys/cape_thread.h>
#include <fmt/cape_args.h>
#include <fmt/cape_json.h>
#include <fmt/cape_tokenizer.h>
#include <stc/cape_map.h>

//-----------------------------------------------------------------------------

struct QBusRouteNode_s
{
  CapeMap local_conns;           // a list of direct connections
  CapeMap proxy_conns;           // via a proxy connection
  
}; typedef struct QBusRouteNode_s* QBusRouteNode;

//-----------------------------------------------------------------------------

void __STDCALL qbus_route__items__on_del (void* key, void* val)
{
  {
    CapeString h = key; cape_str_del (&h);
  }
  {
    
  }
}

//-----------------------------------------------------------------------------

QBusRouteNode qbus_route_node_new ()
{
  QBusRouteNode self = CAPE_NEW (struct QBusRouteNode_s);
  
  self->local_conns = NULL;
  self->proxy_conns = NULL;
  
  return self;
}

//-----------------------------------------------------------------------------

void qbus_route_node_del (QBusRouteNode* p_self)
{
  if (*p_self)
  {
    QBusRouteNode self = *p_self;
    
    cape_map_del (&(self->local_conns));
    cape_map_del (&(self->proxy_conns));
    
    CAPE_DEL (p_self, struct QBusRouteNode_s);
  }
}

//-----------------------------------------------------------------------------

void qbus_route_node__add_local_item (QBusRouteNode self, const CapeString module_uuid, void* user_ptr)
{
  if (self->local_conns == NULL)
  {
    self->local_conns = cape_map_new (NULL, qbus_route__items__on_del, NULL);

    cape_map_insert (self->local_conns, (void*)cape_str_cp (module_uuid), user_ptr);
  }
  else
  {
    CapeMapNode n = cape_map_find (self->local_conns, (void*)module_uuid);
    if (n == NULL)
    {
      cape_map_insert (self->local_conns, (void*)cape_str_cp (module_uuid), user_ptr);
    }
  }  
}

//-----------------------------------------------------------------------------

void qbus_route_node__add_proxy_item (QBusRouteNode self, const CapeString module_uuid, void* user_ptr)
{
  if (self->proxy_conns == NULL)
  {
    self->proxy_conns = cape_map_new (NULL, qbus_route__items__on_del, NULL);
    
    cape_map_insert (self->proxy_conns, (void*)cape_str_cp (module_uuid), user_ptr);
  }
  else
  {
    CapeMapNode n = cape_map_find (self->proxy_conns, (void*)module_uuid);
    if (n == NULL)
    {
      cape_map_insert (self->proxy_conns, (void*)cape_str_cp (module_uuid), user_ptr);
    }
  }  
}

//-----------------------------------------------------------------------------

void qbus_route_node__dump (QBusRouteNode self)
{
  if (self->local_conns)
  {
    CapeMapCursor* cursor = cape_map_cursor_create (self->local_conns, CAPE_DIRECTION_FORW);
    
    while (cape_map_cursor_next (cursor))
    {
      printf ("ooo %s -> %p\n", (CapeString)cape_map_node_key (cursor->node), cape_map_node_value (cursor->node));
    }
    
    cape_map_cursor_destroy (&cursor);
  }

  if (self->proxy_conns)
  {
    CapeMapCursor* cursor = cape_map_cursor_create (self->proxy_conns, CAPE_DIRECTION_FORW);
    
    while (cape_map_cursor_next (cursor))
    {
      printf (">>> %s -> %p\n", (CapeString)cape_map_node_key (cursor->node), cape_map_node_value (cursor->node));
    }
    
    cape_map_cursor_destroy (&cursor);
  }
}

//-----------------------------------------------------------------------------

struct QBusRoute_s
{
  CapeString uuid;            // the unique id of this module
  CapeString name;            // the name of the module (multiple modules might have the same name)

  CapeMap nodes;
};

//-----------------------------------------------------------------------------

void __STDCALL qbus_route__nodes__on_del (void* key, void* val)
{
  {
    CapeString h = key; cape_str_del (&h);
  }
  {
    QBusRouteNode h = val; qbus_route_node_del (&h);
  }
}

//-----------------------------------------------------------------------------

QBusRoute qbus_route_new (const CapeString name)
{
  QBusRoute self = CAPE_NEW (struct QBusRoute_s);

  self->uuid = cape_str_uuid ();
  self->name = cape_str_cp (name);
  
  self->nodes = cape_map_new (NULL, qbus_route__nodes__on_del, NULL);
  
  return self;
}

//-----------------------------------------------------------------------------

void qbus_route_del (QBusRoute* p_self)
{
  if (*p_self)
  {
    QBusRoute self = *p_self;
    
    cape_map_del (&(self->nodes));
    
    cape_str_del (&(self->name));
    cape_str_del (&(self->uuid));
    
    CAPE_DEL (p_self, struct QBusRoute_s);
  }
}

//-----------------------------------------------------------------------------

const CapeString qbus_route_uuid_get (QBusRoute self)
{
  return self->uuid;
}

//-----------------------------------------------------------------------------

const CapeString qbus_route_name_get (QBusRoute self)
{
  return self->name;
}

//-----------------------------------------------------------------------------

CapeUdc qbus_route_node_get (QBusRoute self)
{
  CapeUdc nodes = cape_udc_new (CAPE_UDC_LIST, NULL);
  
  CapeMapCursor* cursor = cape_map_cursor_create (self->nodes, CAPE_DIRECTION_FORW);
  
  while (cape_map_cursor_next (cursor))
  {
    cape_udc_add_s_cp (nodes, NULL, cape_map_node_key (cursor->node));
  }
  
  cape_map_cursor_destroy (&cursor);

  return nodes;
}

//-----------------------------------------------------------------------------

QBusRouteNode qbus_route__seek_or_create_node (QBusRoute self, const CapeString module_name)
{
  QBusRouteNode ret;
  
  // local objects
  CapeString module = cape_str_cp (module_name);

  cape_str_to_upper (module);
  
  {
    CapeMapNode n = cape_map_find (self->nodes, (void*)module_name);
    if (n)
    {
      ret = cape_map_node_value (n);
    }
    else
    {
      ret = qbus_route_node_new ();
      cape_map_insert (self->nodes, (void*)cape_str_mv (&module), (void*)ret);
    }
  }
    
  cape_str_del (&module);
  
  return ret;
}

//-----------------------------------------------------------------------------

void qbus_route__add_local_node (QBusRoute self, const CapeString module_name, const CapeString module_uuid, void* user_ptr)
{
  QBusRouteNode rn = qbus_route__seek_or_create_node (self, module_name);
  
  qbus_route_node__add_local_item (rn, module_uuid, user_ptr);
}

//-----------------------------------------------------------------------------

void qbus_route__add_proxy_node (QBusRoute self, const CapeString module_name, const CapeString module_uuid, void* user_ptr)
{
  QBusRouteNode rn = qbus_route__seek_or_create_node (self, module_name);

  qbus_route_node__add_proxy_item (rn, module_uuid, user_ptr);
}

//-----------------------------------------------------------------------------

void qbus_route_add_nodes (QBusRoute self, const CapeString module_name, const CapeString module_uuid, void* user_ptr, CapeUdc* p_nodes)
{
  CapeUdc nodes = *p_nodes;
  
  qbus_route__add_local_node (self, module_name, module_uuid, user_ptr);
  
  if (nodes)
  {
    CapeUdcCursor* cursor = cape_udc_cursor_new (nodes, CAPE_DIRECTION_FORW);
    
    while (cape_udc_cursor_next (cursor))
    {
      switch (cape_udc_type (cursor->item))
      {
        case CAPE_UDC_STRING:
        {
          qbus_route__add_proxy_node (self, cape_udc_s (cursor->item, NULL), module_uuid, user_ptr);
        
          break;
        }
        case CAPE_UDC_NODE:
        {
          
         
          break;
        }
      }
    }
    
    cape_udc_cursor_del (&cursor);
  }
  
  // tell the others the new nodes
  //  qbus_route_send_updates (self, conn);
  cape_udc_del (p_nodes);
  
  qbus_route_dump (self);
}

//-----------------------------------------------------------------------------

void qbus_route_dump (QBusRoute self)
{
  CapeMapCursor* cursor = cape_map_cursor_create (self->nodes, CAPE_DIRECTION_FORW);
  
  while (cape_map_cursor_next (cursor))
  {
    QBusRouteNode node = cape_map_node_value (cursor->node);
    
    printf ("-- %s ------\n", (CapeString)cape_map_node_key (cursor->node));
    
    qbus_route_node__dump (node);
  }
  
  cape_map_cursor_destroy (&cursor);
}

//-----------------------------------------------------------------------------
