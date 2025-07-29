#include "qbus_router.h"

// cape includes
#include <stc/cape_str.h>
#include <stc/cape_map.h>
#include <sys/cape_log.h>
#include <sys/cape_file.h>
#include <sys/cape_dl.h>

//-----------------------------------------------------------------------------

struct QBusRouter_s
{
  CapeMap routes;
  
  
  
};

//-----------------------------------------------------------------------------

void __STDCALL qbus_router__route_item__on_del (void* ptr)
{
  CapeString h = ptr; cape_str_del (&h);
}

//-----------------------------------------------------------------------------

void __STDCALL qbus_router__routes__on_del (void* key, void* val)
{
  {
    CapeString h = key; cape_str_del (&h);
  }
  {
    CapeList h = val; cape_list_del (&h);
  }
}

//-----------------------------------------------------------------------------

QBusRouter qbus_router_new ()
{
  QBusRouter self = CAPE_NEW (struct QBusRouter_s);
  
  self->routes = cape_map_new (cape_map__compare__s, qbus_router__routes__on_del, NULL);
  
  return self;
}

//-----------------------------------------------------------------------------

void qbus_router_del (QBusRouter* p_self)
{
  if (*p_self)
  {
    QBusRouter self = *p_self;
    
    cape_map_del (&(self->routes));
    
    CAPE_DEL (p_self, struct QBusRouter_s);
  }
}

//-----------------------------------------------------------------------------

void qbus_router_dump__cids (QBusRouter self, const CapeString name, CapeList cids)
{
  CapeListCursor* cursor = cape_list_cursor_new (cids, CAPE_DIRECTION_FORW);

  printf ("-----------------------------------------------------\n");

  while (cape_list_cursor_next (cursor))
  {
    printf ("ITEM: name = %s, cid = %s\n", name, (char*)cape_list_node_data (cursor->node));
  }

  cape_list_cursor_del (&cursor);
}

//-----------------------------------------------------------------------------

void qbus_router_dump (QBusRouter self)
{
  CapeMapCursor* cursor = cape_map_cursor_new (self->routes, CAPE_DIRECTION_FORW);
  
  printf ("-----------------------------------------------------\n");

  while (cape_map_cursor_next (cursor))
  {
    qbus_router_dump__cids (self, cape_map_node_key (cursor->node), cape_map_node_value (cursor->node));
  }

  printf ("-----------------------------------------------------\n");

  cape_map_cursor_del (&cursor);
}

//-----------------------------------------------------------------------------

void qbus_router_add (QBusRouter self, const CapeString cid, const CapeString name)
{
  cape_log_fmt (CAPE_LL_TRACE, "QBUS", "on con", "connection change [ADD] detected, cid = %s, name = %s", cid, name);
  
  CapeList cids;
  
  CapeMapNode n = cape_map_find (self->routes, (void*)name);
  
  if (n)
  {
    cids = cape_map_node_value (n);
  }
  else
  {
    cids = cape_list_new (qbus_router__route_item__on_del);
    
    // use uppercase module names only
    {
      CapeString module_name = cape_str_cp (name);
      
      cape_str_to_upper (module_name);
      
      cape_map_insert (self->routes, (void*)module_name, cids);
    }
  }

  cape_list_push_back (cids, (void*)cape_str_cp (cid));
  
  qbus_router_dump (self);
}

//-----------------------------------------------------------------------------

void qbus_router_rm (QBusRouter self, const CapeString cid, const CapeString name)
{
  //cape_log_fmt (CAPE_LL_TRACE, "QBUS", "on con", "connection change [RM] detected, cid = %s, name = %s", cid, name);

  CapeMapNode n = cape_map_find (self->routes, (void*)name);
  if (n)
  {
    CapeList cids = cape_map_node_value (n);
    
    {
      CapeListCursor* cursor = cape_list_cursor_new (cids, CAPE_DIRECTION_FORW);
      
      while (cape_list_cursor_next (cursor))
      {
        const CapeString cid_from_list = cape_list_node_data (cursor->node);
        
        if (cape_str_equal (cid, cid_from_list))
        {
          cape_list_cursor_erase (cids, cursor);
        }
      }
      
      cape_list_cursor_del (&cursor);
    }
  }

  qbus_router_dump (self);
}

//-----------------------------------------------------------------------------

const CapeString qbus_router_get__from_cids (CapeList cids)
{
  const CapeString ret = NULL;
  
  CapeListCursor* cursor = cape_list_cursor_new (cids, CAPE_DIRECTION_FORW);

  if (cape_list_cursor_next (cursor))
  {
    ret = cape_list_node_data (cursor->node);
  }
  
  cape_list_cursor_del (&cursor);
  
  return ret;
}

//-----------------------------------------------------------------------------

const CapeString qbus_router_get (QBusRouter self, const CapeString name)
{
  const CapeString ret = NULL;
  
  CapeMapNode n = cape_map_find (self->routes, (void*)name);
  if (n)
  {
    ret = qbus_router_get__from_cids (cape_map_node_value (n));
  }
  
  return ret;
}

//-----------------------------------------------------------------------------

CapeUdc qbus_router_list (QBusRouter self)
{
  
}

//-----------------------------------------------------------------------------
