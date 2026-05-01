#include "qbus_router.h"

// cape includes
#include <stc/cape_str.h>
#include <stc/cape_map.h>
#include <sys/cape_log.h>
#include <sys/cape_file.h>
#include <sys/cape_dl.h>
#include <fmt/cape_json.h>

//-----------------------------------------------------------------------------

struct QBusRouteItem_s
{
    CapeString cid;
    CapeUdc methods;
    
}; typedef struct QBusRouteItem_s* QBusRouteItem;

//-----------------------------------------------------------------------------

QBusRouteItem qbus_router_item_new (const CapeString cid, CapeUdc* p_methods, const CapeString name)
{
    QBusRouteItem self = CAPE_NEW (struct QBusRouteItem_s);
  
    self->cid = cape_str_cp (cid);
    self->methods = cape_udc_mv (p_methods);
  
    {
        number_t nmb_methods = 0;
        
        if (self->methods)
        {
            nmb_methods = cape_udc_size (self->methods);
        }

        cape_log_fmt (CAPE_LL_TRACE, "QBUS", "module ADD", "cid = %s, methods = %03li, name = %s", cid, nmb_methods, name);
    }
    
    return self;
}

//-----------------------------------------------------------------------------

int qbus_router_item_is_cid (QBusRouteItem self, const CapeString cid)
{
    return cape_str_equal (cid, self->cid);
}

//-----------------------------------------------------------------------------

const CapeString qbus_router_item_cid (QBusRouteItem self)
{
    return self->cid;
}

//-----------------------------------------------------------------------------

void qbus_router_item_del (QBusRouteItem* p_self)
{
    if (*p_self)
    {
        QBusRouteItem self = *p_self;

        cape_str_del (&(self->cid));
        cape_udc_del (&(self->methods));
      
        CAPE_DEL (p_self, struct QBusRouteItem_s);
    }
}

//-----------------------------------------------------------------------------

struct QBusRouter_s
{
  CapeMap routes;
};

//-----------------------------------------------------------------------------

void __STDCALL qbus_router__route_item__on_del (void* ptr)
{
    QBusRouteItem h = ptr; qbus_router_item_del (&h);
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
        printf ("ITEM: name = %s, cid = %s\n", name, qbus_router_item_cid (cape_list_node_data (cursor->node)));
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

void qbus_router_add (QBusRouter self, const CapeString cid, const CapeString name, CapeUdc* p_methods)
{
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
    
    // add new item
    {
        cape_list_push_back (cids, (void*)qbus_router_item_new (cid, p_methods, name));
    }

    //qbus_router_dump (self);
}

//-----------------------------------------------------------------------------

void qbus_router_rm (QBusRouter self, const CapeString cid, const CapeString name)
{
    cape_log_fmt (CAPE_LL_TRACE, "QBUS", "module REMOVE", "connection change detected, cid = %s, name = %s", cid, name);

    CapeMapNode n = cape_map_find (self->routes, (void*)name);
    if (n)
    {
        CapeList cids = cape_map_node_value (n);
        
        {
            CapeListCursor* cursor = cape_list_cursor_new (cids, CAPE_DIRECTION_FORW);
            
            while (cape_list_cursor_next (cursor))
            {
                if (qbus_router_item_is_cid (cape_list_node_data (cursor->node), cid))
                {
                    cape_list_cursor_erase (cids, cursor);
                }
            }
            
            cape_list_cursor_del (&cursor);
        }
        
        if (cape_list_size (cids) == 0)
        {
            
        }
    }

    //qbus_router_dump (self);
}

//-----------------------------------------------------------------------------

const CapeString qbus_router_get__first_cid (CapeList cids)
{
    const CapeString ret = NULL;
    
    CapeListCursor* cursor = cape_list_cursor_new (cids, CAPE_DIRECTION_FORW);

    if (cape_list_cursor_next (cursor))
    {
        ret = qbus_router_item_cid (cape_list_node_data (cursor->node));
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
        // TODO: apply a more complex algorithm here
        ret = qbus_router_get__first_cid (cape_map_node_value (n));
    }
    
    return ret;
}

//-----------------------------------------------------------------------------

void qbus_router_modules__add_cids (CapeUdc cids_node, CapeList cids)
{
    CapeListCursor* cursor = cape_list_cursor_new (cids, CAPE_DIRECTION_FORW);

    while (cape_list_cursor_next (cursor))
    {
        cape_udc_add_s_cp (cids_node, NULL, qbus_router_item_cid (cape_list_node_data (cursor->node)));
    }

    cape_list_cursor_del (&cursor);
}

//-----------------------------------------------------------------------------

CapeUdc qbus_router_modules (QBusRouter self)
{
    CapeUdc ret = cape_udc_new (CAPE_UDC_LIST, NULL);
    
    CapeMapCursor* cursor = cape_map_cursor_new (self->routes, CAPE_DIRECTION_FORW);
    
    while (cape_map_cursor_next (cursor))
    {
        CapeUdc list_node = cape_udc_new (CAPE_UDC_NODE, NULL);
        
        cape_udc_add_s_cp (list_node, "name", cape_map_node_key (cursor->node));
        
        qbus_router_modules__add_cids (cape_udc_add_list (list_node, "cids"), cape_map_node_value (cursor->node));
        
        cape_udc_add (ret, &list_node);
    }
    
    cape_map_cursor_del (&cursor);
    
    return ret;
}

//-----------------------------------------------------------------------------

void qbus_router_list__add_methods (CapeUdc methods_list)
{
    
}

//-----------------------------------------------------------------------------

CapeUdc qbus_router_methods (QBusRouter self, const CapeString cid)
{
    CapeUdc ret = cape_udc_new (CAPE_UDC_LIST, NULL);


    
    return ret;
}

//-----------------------------------------------------------------------------
