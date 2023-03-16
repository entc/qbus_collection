#include "qbus_route.h"
#include "qbus_engines.h"

// cape includes
#include <sys/cape_file.h>
#include <sys/cape_log.h>
#include <sys/cape_thread.h>
#include <sys/cape_mutex.h>
#include <fmt/cape_args.h>
#include <fmt/cape_json.h>
#include <fmt/cape_tokenizer.h>
#include <stc/cape_map.h>

//-----------------------------------------------------------------------------

#define QBUS_ROUTE_NODE_TYPE__SELF   1
#define QBUS_ROUTE_NODE_TYPE__LOCAL  2
#define QBUS_ROUTE_NODE_TYPE__PROXY  3

//-----------------------------------------------------------------------------

typedef struct {
  
  number_t cnt_local;
  number_t cnt_proxy;
  
} QbusRoutUpdateCtx;

//-----------------------------------------------------------------------------

struct QBusRouteConn_s
{
  CapeString module_uuid;
  QBusSubscriber load_subscriber;
  
}; typedef struct QBusRouteConn_s* QBusRouteConn;

//-----------------------------------------------------------------------------

void __STDCALL qbus_route_conn__on_emit (void* user_ptr, const CapeString subscriber_name, CapeUdc* p_data)
{
  QBusRouteConn self = user_ptr;
  
  {
    CapeUdc value = *p_data;
    
    if (value)
    {
      printf ("GOT LOAD DATA: %s\n", cape_udc_s (value, ""));
    }
  }
}

//-----------------------------------------------------------------------------

QBusRouteConn qbus_route_conn_new (const CapeString module_uuid, QBusObsvbl obsvbl)
{
  QBusRouteConn self = CAPE_NEW (struct QBusRouteConn_s);
  
  self->module_uuid = cape_str_cp (module_uuid);
  
  if (module_uuid)
  {
    self->load_subscriber = qbus_obsvbl_subscribe_uuid (obsvbl, module_uuid, "load", (void*)self, qbus_route_conn__on_emit);
  }
  
  return self;
}

//-----------------------------------------------------------------------------

void qbus_route_conn_del (QBusRouteConn* p_self)
{
  if (*p_self)
  {
    QBusRouteConn self = *p_self;
    
    if (self->load_subscriber)
    {
      
    }
    
    cape_str_del (&(self->module_uuid));
    
    CAPE_DEL (p_self, struct QBusRouteConn_s);
  }
}

//-----------------------------------------------------------------------------

void qbus_route__append_to_ptrs (QBusPvdConnection conn, CapeList user_ptrs__version1, CapeList user_ptrs__version3)
{
  switch (conn->version)
  {
    case 0:
    {
      cape_list_push_back (user_ptrs__version1, conn);
      cape_list_push_back (user_ptrs__version3, conn);        
      break;
    }
    case 1:
    {
      cape_list_push_back (user_ptrs__version1, conn);
      break;
    }
    case 3:
    {
      cape_list_push_back (user_ptrs__version3, conn);
      break;
    }
  }
}

//-----------------------------------------------------------------------------

struct QBusRouteUuidItem_s
{
  
  QBusPvdConnection conn;
  QBusSubscriber load_subscriber;
  
}; typedef struct QBusRouteUuidItem_s* QBusRouteUuidItem; 

//-----------------------------------------------------------------------------

QBusRouteUuidItem qbus_route_uuid_new (QBusObsvbl obsvbl, const CapeString module_uuid, QBusPvdConnection conn)
{
  QBusRouteUuidItem self = CAPE_NEW (struct QBusRouteUuidItem_s);

  self->load_subscriber = qbus_obsvbl_subscribe_uuid (obsvbl, module_uuid, "load", (void*)self, qbus_route_conn__on_emit);
  self->conn = conn;
  
  return self;
}

//-----------------------------------------------------------------------------

void qbus_route_uuid_del (QBusRouteUuidItem* p_self)
{
  if (*p_self)
  {
    QBusRouteUuidItem self = *p_self;
    
    qbus_obsvbl_unsubscribe (&(self->load_subscriber));
   
    CAPE_DEL (p_self, struct QBusRouteUuidItem_s);
  }
}

//-----------------------------------------------------------------------------

void qbus_route_uuid__dump (QBusRouteUuidItem self, const CapeString module_uuid)
{
  printf ("%10s |   | %36s | %p #%lu\n", "", module_uuid, self->conn, self->conn->version);  
}

//-----------------------------------------------------------------------------

struct QBusRouteNameItem_s
{
  
  CapeString uuid;
  
  QBusRouteUuidItem item;
  
  int local;
  
};

//-----------------------------------------------------------------------------

QBusRouteNameItem qbus_route_name_new (QBusObsvbl obsvbl, const CapeString module_uuid, QBusPvdConnection conn, int is_local)
{
  QBusRouteNameItem self = CAPE_NEW (struct QBusRouteNameItem_s);
  
  self->uuid = cape_str_cp (module_uuid);
  self->item = qbus_route_uuid_new (obsvbl, module_uuid, conn);
  
  self->local = is_local;
  
  return self;
};

//-----------------------------------------------------------------------------

void qbus_route_name_del (QBusRouteNameItem* p_self)
{
  if (*p_self)
  {
    QBusRouteNameItem self = *p_self;

    cape_str_del (&(self->uuid));
    qbus_route_uuid_del (&(self->item));

    CAPE_DEL (p_self, struct QBusRouteNameItem_s);
  }
}

//-----------------------------------------------------------------------------

const CapeString qbus_route_name_uuid_get (QBusRouteNameItem self)
{
  return self->uuid;
}

//-----------------------------------------------------------------------------

void qbus_route_name__append_to_nodes (QBusRouteNameItem self, QBusObsvbl obsvbl, const CapeString module_name, QBusPvdConnection conn, CapeUdc nodes)
{
  if (self->uuid)
  {
    if (cape_udc_get (nodes, self->uuid) == NULL)
    {
      CapeUdc node = cape_udc_new (CAPE_UDC_NODE, self->uuid);
      
      cape_udc_add_s_cp (node, "module", module_name);
      cape_udc_add_n (node, "type", QBUS_ROUTE_NODE_TYPE__LOCAL);
      
      {
        CapeUdc observables = qbus_obsvbl_get (obsvbl, module_name, self->uuid);
        
        cape_udc_add_name (node, &observables, "obsvbls");
      }
      
      cape_udc_add (nodes, &node);
    }
  }  
}

//-----------------------------------------------------------------------------

int qbus_route_name__is_local_connection (QBusRouteNameItem self)
{
  return self->local;
}

//-----------------------------------------------------------------------------

void qbus_route_name__append_to_ptrs (QBusRouteNameItem self, QBusPvdConnection conn_exclude, CapeList user_ptrs__version1, CapeList user_ptrs__version3)
{
  if (self->item->conn != conn_exclude)
  {
    qbus_route__append_to_ptrs (self->item->conn, user_ptrs__version1, user_ptrs__version3);
  }
}

//-----------------------------------------------------------------------------

const CapeString qbus_route_name__get_uuid (QBusRouteNameItem self)
{
  return self->uuid;
}

//-----------------------------------------------------------------------------

QBusPvdConnection qbus_route_name__get_conn (QBusRouteNameItem self)
{
  return self->item->conn;
}

//-----------------------------------------------------------------------------

void qbus_route_name__dump (QBusRouteNameItem self, const CapeString module_name)
{
  printf ("%10s | %i | %36s | %p #%lu\n", module_name, self->local, self->uuid, self->item->conn, self->item->conn->version);  
}

//-----------------------------------------------------------------------------

void qbus_route_name_dump2 (QBusRouteNameItem self, const CapeString module_name, int is_local, const CapeString data)
{
  printf ("%10s | %i | %36s | %s\n", module_name, is_local, self->uuid, data);  
}

//-----------------------------------------------------------------------------

struct QBusRouteModules_s
{
  
  CapeList modules;
  
}; typedef struct QBusRouteModules_s* QBusRouteModules;

//-----------------------------------------------------------------------------

void __STDCALL qbus_route_modules__modules__on_del (void* ptr)
{
  QBusRouteNameItem h = ptr; qbus_route_name_del (&h);
}

//-----------------------------------------------------------------------------

QBusRouteModules qbus_route_modules_new ()
{
  QBusRouteModules self = CAPE_NEW (struct QBusRouteModules_s);
  
  self->modules = cape_list_new (qbus_route_modules__modules__on_del);
  
  return self;  
};

//-----------------------------------------------------------------------------

void qbus_route_modules_del (QBusRouteModules* p_self)
{
  if (*p_self)
  {
    QBusRouteModules self = *p_self;
    
    cape_list_del (&(self->modules));
    
    CAPE_DEL (p_self, struct QBusRouteModules_s);
  }
}

//-----------------------------------------------------------------------------

QBusRouteNameItem qbus_route_modules__has_uuid (QBusRouteModules self, const CapeString module_uuid)
{
  QBusRouteNameItem ret = NULL;
  CapeListCursor* cursor = cape_list_cursor_create (self->modules, CAPE_DIRECTION_FORW);
  
  while (cape_list_cursor_next (cursor))
  {
    if (cape_str_equal (qbus_route_name__get_uuid (cape_list_node_data (cursor->node)), module_uuid))
    {
      ret = cape_list_node_data (cursor->node);
      break;
    }
  }
  
  cape_list_cursor_destroy (&cursor);
  return ret;
}

//-----------------------------------------------------------------------------

QBusRouteNameItem qbus_route_modules__add (QBusRouteModules self, QBusObsvbl obsvbl, const CapeString module_uuid, QBusPvdConnection conn, int is_local, QbusRoutUpdateCtx* rux, CapeMap modules_uuids)
{
  QBusRouteNameItem ret = NULL;
  
  if (cape_str_not_empty (module_uuid))
  {
    // check for collisions
    ret = qbus_route_modules__has_uuid (self, module_uuid); 
    
    // already added
    if (ret)
    {
      goto cleanup_and_exit;
    }
  }
  else
  {
    if (cape_list_size (self->modules) > 0)
    {
      goto cleanup_and_exit;
    }    
  }
  
  {
    ret = qbus_route_name_new (obsvbl, module_uuid, conn, is_local);
    
    cape_list_push_back (self->modules, (void*)ret);
    
    if (is_local)
    {
      rux->cnt_local++;
    }
    else
    {
      rux->cnt_proxy++;
    }
    
    if (cape_str_not_empty (module_uuid))
    {
      CapeMapNode n = cape_map_find (modules_uuids, (void*)module_uuid);
      if (n == NULL)
      {
        cape_map_insert (modules_uuids, (void*)cape_str_cp (module_uuid), ret->item);
      }
    }
  }
  
cleanup_and_exit:
  
  return ret;
}

//-----------------------------------------------------------------------------

void qbus_route_modules__append_to_nodes (QBusRouteModules self, QBusObsvbl obsvbl, const CapeString module_name, QBusPvdConnection conn, CapeUdc nodes)
{
  CapeListCursor* cursor = cape_list_cursor_create (self->modules, CAPE_DIRECTION_FORW);
  
  while (cape_list_cursor_next (cursor))
  {
    qbus_route_name__append_to_nodes (cape_list_node_data (cursor->node), obsvbl, module_name, conn, nodes);
  }
  
  cape_list_cursor_destroy (&cursor);
}

//-----------------------------------------------------------------------------

int qbus_route_modules__has_local_connections (QBusRouteModules self)
{
  int ret = FALSE;
  
  CapeListCursor* cursor = cape_list_cursor_create (self->modules, CAPE_DIRECTION_FORW);
  
  while (cape_list_cursor_next (cursor))
  {
    ret = ret || qbus_route_name__is_local_connection (cape_list_node_data (cursor->node));
  }
  
  cape_list_cursor_destroy (&cursor);
  
  return ret;
}

//-----------------------------------------------------------------------------

void qbus_route_modules__append_to_ptrs (QBusRouteModules self, QBusPvdConnection conn_exclude, CapeList user_ptrs__version1, CapeList user_ptrs__version3)
{
  CapeListCursor* cursor = cape_list_cursor_create (self->modules, CAPE_DIRECTION_FORW);
  
  while (cape_list_cursor_next (cursor))
  {
    qbus_route_name__append_to_ptrs (cape_list_node_data (cursor->node), conn_exclude, user_ptrs__version1, user_ptrs__version3);
  }
  
  cape_list_cursor_destroy (&cursor);
}


//-----------------------------------------------------------------------------

void qbus_route_modules__get_routings (QBusRouteModules self, CapeMap routings, CapeMap user_ptrs)
{
  /*
  if (self->local_conns)
  {
    CapeMapCursor* cursor = cape_map_cursor_create (routings, CAPE_DIRECTION_FORW);
    
    while (cape_map_cursor_next (cursor))
    {
      const CapeString uuid = cape_map_node_key (cursor->node);
      
      QBusPvdConnection conn = qbus_route_node__seek (self->local_conns, uuid);
      if (conn)
      {
        qbus_route_node__get_routings__append (uuid, user_ptrs, conn);
      }
    }
    
    cape_map_cursor_destroy (&cursor);
  }
  
  */
}

//-----------------------------------------------------------------------------

void qbus_route_modules__rm_conn (QBusRouteModules self, QBusObsvbl obsvbl, QBusPvdConnection conn_rm, int keep_local, CapeMap module_uuids)
{
  CapeListCursor* cursor = cape_list_cursor_create (self->modules, CAPE_DIRECTION_FORW);
  
  while (cape_list_cursor_next (cursor))
  {
    QBusRouteNameItem name_item = cape_list_node_data (cursor->node);
    
    if (qbus_route_name__get_conn (name_item) == conn_rm && (keep_local == FALSE || qbus_route_name__is_local_connection (name_item) == FALSE))
    {
      if (name_item->uuid)
      {
        CapeMapNode n = cape_map_find (module_uuids, (void*)name_item->uuid);
        if (n)
        {
          cape_map_erase (module_uuids, n);
        }
      }
      
      // remove all emitter entries which have a link to this name item
      qbus_obsvbl_rm (obsvbl, name_item);
      
      cape_list_cursor_erase (self->modules, cursor);
    }
  }
  
  cape_list_cursor_destroy (&cursor);
}
  
//-----------------------------------------------------------------------------

void qbus_route_modules__dump (QBusRouteModules self, const CapeString module_name)
{
  CapeListCursor* cursor = cape_list_cursor_create (self->modules, CAPE_DIRECTION_FORW);
  
  while (cape_list_cursor_next (cursor))
  {
    qbus_route_name__dump (cape_list_node_data (cursor->node), module_name);
  }
  
  cape_list_cursor_destroy (&cursor);
}

//-----------------------------------------------------------------------------

struct QBusRoute_s
{
  QBusEngines engines;        // reference
  QBusObsvbl obsvbl;          // reference
  
  CapeString uuid;            // the unique id of this module
  CapeString name;            // the name of the module (multiple modules might have the same name)

  CapeMap modules_uuids;      // list of all connected modules sorted by unique uuid
  CapeMap modules_names;      // list of all connected modules by name
  
  CapeMutex mutex;            // to ensure thread safety
};

//-----------------------------------------------------------------------------

void __STDCALL qbus_route__modules_uuids__on_del (void* key, void* val)
{
  {
    CapeString h = key; cape_str_del (&h);
  }
  {
    // this is only a reference
  }
}

//-----------------------------------------------------------------------------

void __STDCALL qbus_route__modules_names__on_del (void* key, void* val)
{
  {
    CapeString h = key; cape_str_del (&h);
  }
  {
    QBusRouteModules h = val; qbus_route_modules_del (&h);
  }
}

//-----------------------------------------------------------------------------

QBusRoute qbus_route_new (const CapeString name, QBusEngines engines)
{
  QBusRoute self = CAPE_NEW (struct QBusRoute_s);

  self->engines = engines;
  self->obsvbl = NULL;
  
  self->uuid = cape_str_uuid ();
  self->name = cape_str_cp (name);
  
  cape_str_to_upper (self->name);
  
  cape_log_fmt (CAPE_LL_INFO, "QBUS", "route", "started new qbus node with = %s as %s", self->uuid, self->name);
  
  self->modules_uuids = cape_map_new (NULL, qbus_route__modules_uuids__on_del, NULL);
  self->modules_names = cape_map_new (NULL, qbus_route__modules_names__on_del, NULL);
  
  self->mutex = cape_mutex_new ();
  
  return self;
}

//-----------------------------------------------------------------------------

void qbus_route_del (QBusRoute* p_self)
{
  if (*p_self)
  {
    QBusRoute self = *p_self;
    
    cape_mutex_del (&(self->mutex));
    
    cape_str_del (&(self->name));
    cape_str_del (&(self->uuid));
    
    cape_map_del (&(self->modules_names));
    cape_map_del (&(self->modules_uuids));
    
    CAPE_DEL (p_self, struct QBusRoute_s);
  }
}

//-----------------------------------------------------------------------------

void qbus_route_set (QBusRoute self, QBusObsvbl obsvbl)
{
  self->obsvbl = obsvbl;
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

CapeUdc qbus_route_node_get (QBusRoute self, int as_node, QBusPvdConnection conn_not_in_list)
{
  if (as_node)
  {
    CapeUdc nodes = cape_udc_new (CAPE_UDC_NODE, NULL);

    // always send self node
    {
      CapeUdc node = cape_udc_new (CAPE_UDC_NODE, self->uuid);
      
      cape_udc_add_s_cp (node, "module", self->name);
      cape_udc_add_n (node, "type", QBUS_ROUTE_NODE_TYPE__SELF);
      
      {
        CapeUdc observables = qbus_obsvbl_get (self->obsvbl, self->name, self->uuid);
        
        cape_udc_add_name (node, &observables, "obsvbls");
      }
      
      cape_udc_add (nodes, &node);
    }
    
    // send all other nodes
    {
      CapeMapCursor* cursor = cape_map_cursor_create (self->modules_names, CAPE_DIRECTION_FORW);
      
      while (cape_map_cursor_next (cursor))
      {
        qbus_route_modules__append_to_nodes (cape_map_node_value (cursor->node), self->obsvbl, cape_map_node_key (cursor->node), conn_not_in_list, nodes);
      }
      
      cape_map_cursor_destroy (&cursor);
    }
    
    return nodes;
  }
  else
  {
    CapeUdc nodes = cape_udc_new (CAPE_UDC_LIST, NULL);
    
    CapeMapCursor* cursor = cape_map_cursor_create (self->modules_names, CAPE_DIRECTION_FORW);
    
    while (cape_map_cursor_next (cursor))
    {
      if (qbus_route_modules__has_local_connections (cape_map_node_value (cursor->node)))
      {
        cape_udc_add_s_cp (nodes, NULL, cape_map_node_key (cursor->node));
      }
    }
    
    cape_map_cursor_destroy (&cursor);

    return nodes;
  }
}

//-----------------------------------------------------------------------------

QBusRouteNameItem qbus_route__seek_or_create_node (QBusRoute self, const CapeString module_name, const CapeString module_uuid, QBusPvdConnection conn, int is_local, QbusRoutUpdateCtx* rux)
{
  QBusRouteNameItem ret = NULL;
  
  // local objects
  CapeString module = cape_str_cp (module_name);

  cape_str_to_upper (module);
  
  {
    QBusRouteModules modules;
    
    CapeMapNode n = cape_map_find (self->modules_names, (void*)module_name);
    if (n)
    {
      modules = cape_map_node_value (n);
    }
    else
    {
      modules = qbus_route_modules_new ();
      cape_map_insert (self->modules_names, (void*)cape_str_mv (&module), (void*)modules);
    }

    ret = qbus_route_modules__add (modules, self->obsvbl, module_uuid, conn, is_local, rux, self->modules_uuids);
  }
    
  cape_str_del (&module);
  
  return ret;
}

//-----------------------------------------------------------------------------

QBusRouteNameItem qbus_route__add_local_node (QBusRoute self, const CapeString module_name, const CapeString module_uuid, QBusPvdConnection conn, QbusRoutUpdateCtx* rux)
{
  return qbus_route__seek_or_create_node (self, module_name, module_uuid, conn, TRUE, rux);
}

//-----------------------------------------------------------------------------

QBusRouteNameItem qbus_route__add_proxy_node (QBusRoute self, const CapeString module_name, const CapeString module_uuid, QBusPvdConnection conn, QbusRoutUpdateCtx* rux)
{
  return qbus_route__seek_or_create_node (self, module_name, module_uuid, conn, FALSE, rux);
}

//-----------------------------------------------------------------------------

int qbus_route__is_module_name_valid (QBusRoute self, const CapeString remote_module)
{
  int ret = FALSE;
  
  if (cape_str_not_empty (remote_module))
  {
    CapeString h = cape_str_cp (remote_module);
    
    cape_str_to_upper (h);
    
    ret = !cape_str_equal (self->name, h);
    
    cape_str_del (&h);
  }
  
  return ret;
}

//-----------------------------------------------------------------------------

void qbus_route_add_nodes__list (QBusRoute self, CapeUdc nodes, QBusPvdConnection conn, QbusRoutUpdateCtx* rux)
{
  CapeUdcCursor* cursor = cape_udc_cursor_new (nodes, CAPE_DIRECTION_FORW);
  
  while (cape_udc_cursor_next (cursor))
  {
    switch (cape_udc_type (cursor->item))
    {
      case CAPE_UDC_STRING:
      {
        const CapeString remote_module = cape_udc_s (cursor->item, NULL);
        
        if (qbus_route__is_module_name_valid (self, remote_module))
        {
          qbus_route__add_proxy_node (self, remote_module, NULL, conn, rux);
        }
        
        break;
      }
      case CAPE_UDC_NODE:
      {
        const CapeString remote_module = cape_udc_get_s (cursor->item, "module", NULL);

        if (qbus_route__is_module_name_valid (self, remote_module))
        {
          qbus_route__add_proxy_node (self, remote_module, NULL, conn, rux);
        }

        break;
      }
    }
  }
  
  cape_udc_cursor_del (&cursor);
}

//-----------------------------------------------------------------------------

typedef struct {
  
  CapeUdc obsvbls;
  QBusRouteNameItem name_item;
  
} QBusRouteAddNodesItem;

//-----------------------------------------------------------------------------

void __STDCALL qbus_route_add__add_nodes_items__on_del (void* ptr)
{
  QBusRouteAddNodesItem* ani = ptr; CAPE_DEL (&ani, QBusRouteAddNodesItem);
}

//-----------------------------------------------------------------------------

void qbus_route_add_nodes__node (QBusRoute self, CapeUdc nodes, QBusPvdConnection conn, QbusRoutUpdateCtx* rux, QBusRouteNameItem name_local_item)
{
  // store the observable set into this list
  // -> this is needed because the subscribers must be added first
  CapeList add_nodes_items = cape_list_new (qbus_route_add__add_nodes_items__on_del);
  
  // add to route and subscribers
  {
    CapeUdcCursor* cursor = cape_udc_cursor_new (nodes, CAPE_DIRECTION_FORW);
    
    while (cape_udc_cursor_next (cursor))
    {
      switch (cape_udc_type (cursor->item))
      {        
        case CAPE_UDC_NODE:
        {
          switch (cape_udc_get_n (cursor->item, "type", 0))
          {
            case QBUS_ROUTE_NODE_TYPE__SELF:
            {
              const CapeString remote_module = cape_udc_get_s (cursor->item, "module", NULL);
              
              if (qbus_route__is_module_name_valid (self, remote_module))
              {
                QBusRouteAddNodesItem* ani = CAPE_NEW (QBusRouteAddNodesItem);
                
                ani->obsvbls = cape_udc_get (cursor->item, "obsvbls");
                ani->name_item = name_local_item;

                cape_list_push_back (add_nodes_items, ani);
              }
              
              break;
            }
            case QBUS_ROUTE_NODE_TYPE__LOCAL:
            {
              const CapeString remote_module = cape_udc_get_s (cursor->item, "module", NULL);
              
              if (qbus_route__is_module_name_valid (self, remote_module))
              {
                QBusRouteAddNodesItem* ani = CAPE_NEW (QBusRouteAddNodesItem);
                
                ani->obsvbls = cape_udc_get (cursor->item, "obsvbls");
                ani->name_item = qbus_route__add_proxy_node (self, remote_module, cape_udc_name (cursor->item), conn, rux);
                
                cape_list_push_back (add_nodes_items, ani);
              }
              
              break;
            }
            case QBUS_ROUTE_NODE_TYPE__PROXY:
            {
              
              break;
            }
          }
          
          break;
        }
      }
    }
    
    cape_udc_cursor_del (&cursor);
  }
  
  // add the emitters
  {
    CapeListCursor* cursor = cape_list_cursor_create (add_nodes_items, CAPE_DIRECTION_FORW);
    
    while (cape_list_cursor_next (cursor))
    {
      QBusRouteAddNodesItem* ani = cape_list_node_data (cursor->node);
      
      qbus_obsvbl_set (self->obsvbl, ani->obsvbls, ani->name_item);
    }
    
    cape_list_cursor_destroy (&cursor);
  }
  
  cape_list_del (&add_nodes_items);
}

//-----------------------------------------------------------------------------

void qbus_route_add_nodes (QBusRoute self, const CapeString module_name, const CapeString sender_uuid, QBusPvdConnection conn, CapeUdc* p_nodes)
{
  CapeUdc nodes = *p_nodes;

  QbusRoutUpdateCtx rux;
  
  QBusRouteNameItem name_local_item = NULL;
  
  rux.cnt_local = 0;
  rux.cnt_proxy = 0;
  
  {
    CapeString h = cape_json_to_s (nodes);
    
    printf ("NODES: %s\n", h);
    
    cape_str_del (&h);
  }
  
  cape_mutex_lock (self->mutex);
  
  if (qbus_route__is_module_name_valid (self, module_name))
  {
    name_local_item = qbus_route__add_local_node (self, module_name, sender_uuid, conn, &rux);
  }
  
  // clear all nodes which belong to the uuid
  {
    CapeMapCursor* cursor = cape_map_cursor_create (self->modules_names, CAPE_DIRECTION_FORW);
    
    while (cape_map_cursor_next (cursor))
    {
      qbus_route_modules__rm_conn (cape_map_node_value (cursor->node), self->obsvbl, conn, TRUE, self->modules_uuids);
    }
    
    cape_map_cursor_destroy (&cursor);
  }
  
  if (nodes)
  {
    switch (cape_udc_type (nodes))
    {
      case CAPE_UDC_LIST:
      {
        // set the version
        if (conn->version < 1)
        {
          conn->version = 1;
          rux.cnt_local++;
        }
        
        qbus_route_add_nodes__list (self, nodes, conn, &rux);
        break;
      }
      case CAPE_UDC_NODE:
      {
        // set the version
        if (conn->version < 3)
        {
          conn->version = 3;
          rux.cnt_local++;
        }
        
        qbus_route_add_nodes__node (self, nodes, conn, &rux, name_local_item);
        break;
      }
    }
  }

  cape_mutex_unlock (self->mutex);
  
  // tell the others the new nodes
  //  qbus_route_send_updates (self, conn);
  cape_udc_del (p_nodes);
  
  printf ("check observables\n");
  
  qbus_route_dump (self);
  
  if (rux.cnt_local)
  {
    printf ("send observables\n");

    qbus_route_send_update (self, NULL, conn);
  }

  printf ("send update\n");
  
  // tell all others our updates
  qbus_route_send_update (self, conn, NULL);
  
}

//-----------------------------------------------------------------------------

void qbus_route_rm (QBusRoute self, QBusPvdConnection conn)
{
  CapeMapCursor* cursor = cape_map_cursor_create (self->modules_names, CAPE_DIRECTION_FORW);
  
  while (cape_map_cursor_next (cursor))
  {
    qbus_route_modules__rm_conn (cape_map_node_value (cursor->node), self->obsvbl, conn, FALSE, self->modules_uuids);
  }
  
  cape_map_cursor_destroy (&cursor);
  
  qbus_route_dump (self);

  // tell all others our updates
  qbus_route_send_update (self, conn, NULL);  
}

//-----------------------------------------------------------------------------

void qbus_route_frame_nodes_add (QBusRoute self, QBusFrame frame, int as_node, QBusPvdConnection conn_not_in_list)
{
  CapeUdc route_nodes = qbus_route_node_get (self, as_node, conn_not_in_list);
  
  {
    CapeString h = cape_json_to_s (route_nodes);
    
    printf ("ROUTE NODES [%i]: %s\n", as_node, h);
    
    cape_str_del (&h);
  }
  
  if (route_nodes)
  {
    // set the payload frame
    qbus_frame_set_udc (frame, QBUS_MTYPE_JSON, &route_nodes);
  }
}

//-----------------------------------------------------------------------------

void qbus_route_send_update (QBusRoute self, QBusPvdConnection conn_not_in_list, QBusPvdConnection conn_single)
{
  CapeList user_ptrs__version1 = cape_list_new (NULL);
  CapeList user_ptrs__version3 = cape_list_new (NULL);
  
  if (conn_single)
  {
    qbus_route__append_to_ptrs (conn_single, user_ptrs__version1, user_ptrs__version3);
  }
  else
  {
    qbus_route_get__conn (self, conn_not_in_list, user_ptrs__version1, user_ptrs__version3);
  }
    
  if (cape_list_size (user_ptrs__version1) > 0)
  {
    // send old version as list
    {
      QBusFrame frame = qbus_frame_new ();
      
      // CH01: replace self->name with self->uuid and add name as module
      qbus_frame_set (frame, QBUS_FRAME_TYPE_ROUTE_UPD, NULL, self->name, NULL, self->uuid);
      
      // create an UDC structure of all nodes
      qbus_route_frame_nodes_add (self, frame, FALSE, conn_not_in_list);
      
      // send the frame
      qbus_engines__broadcast (self->engines, frame, user_ptrs__version1);
      
      qbus_frame_del (&frame);
    }
  }
  
  if (cape_list_size (user_ptrs__version3) > 0)
  {
    // send new version as node
    {
      QBusFrame frame = qbus_frame_new ();
      
      // CH01: replace self->name with self->uuid and add name as module
      qbus_frame_set (frame, QBUS_FRAME_TYPE_ROUTE_UPD, NULL, self->name, NULL, self->uuid);
      
      // create an UDC structure of all nodes
      qbus_route_frame_nodes_add (self, frame, TRUE, conn_not_in_list);
      
      // send the frame
      qbus_engines__broadcast (self->engines, frame, user_ptrs__version3);
      
      qbus_frame_del (&frame);
    }
  }
  
  cape_list_del (&user_ptrs__version1);
  cape_list_del (&user_ptrs__version3);
}

//-----------------------------------------------------------------------------

void qbus_route_dump (QBusRoute self)
{
  
  printf ("-----------+---+--------------------------------------+--------------------------------------\n");
  printf ("      NAME | L |                                 UUID | DATA\n");
  printf ("-----------+---+--------------------------------------+--------------------------------------\n");

  {
    CapeMapCursor* cursor = cape_map_cursor_create (self->modules_names, CAPE_DIRECTION_FORW);

    while (cape_map_cursor_next (cursor))
    {
      qbus_route_modules__dump (cape_map_node_value (cursor->node), (CapeString)cape_map_node_key (cursor->node));
    }
    
    cape_map_cursor_destroy (&cursor);
  }

  printf ("-----------+---+--------------------------------------+--------------------------------------\n");
  
  {
    CapeMapCursor* cursor = cape_map_cursor_create (self->modules_uuids, CAPE_DIRECTION_FORW);
    
    while (cape_map_cursor_next (cursor))
    {
      qbus_route_uuid__dump (cape_map_node_value (cursor->node), (CapeString)cape_map_node_key (cursor->node));
    }
    
    cape_map_cursor_destroy (&cursor);
  }
  
  printf ("-----------+---+--------------------------------------+--------------------------------------\n");
  
  qbus_obsvbl_dump (self->obsvbl);
  
  printf ("-----------+---+--------------------------------------+--------------------------------------\n");
}

//-----------------------------------------------------------------------------

void qbus_route_get__conn (QBusRoute self, QBusPvdConnection conn, CapeList user_ptrs__version1, CapeList user_ptrs__version3)
{
  CapeMapCursor* cursor = cape_map_cursor_create (self->modules_names, CAPE_DIRECTION_FORW);
  
  while (cape_map_cursor_next (cursor))
  {
    qbus_route_modules__append_to_ptrs (cape_map_node_value (cursor->node), conn, user_ptrs__version1, user_ptrs__version3);      
  }
  
  cape_map_cursor_destroy (&cursor);
}

//-----------------------------------------------------------------------------

void __STDCALL qbus_route__routings__on_del (void* key, void* val)
{
  {
    CapeString h = key; cape_str_del (&h);
  }
}

//-----------------------------------------------------------------------------

CapeMap qbus_route_get__routings (QBusRoute self, CapeMap routings)
{
  CapeMap user_ptrs = cape_map_new (NULL, qbus_route__routings__on_del, NULL);

  {
    CapeMapCursor* cursor = cape_map_cursor_create (self->modules_names, CAPE_DIRECTION_FORW);
    
    while (cape_map_cursor_next (cursor))
    {
      qbus_route_modules__get_routings (cape_map_node_value (cursor->node), routings, user_ptrs);      
    }
    
    cape_map_cursor_destroy (&cursor);
  }
  
  return user_ptrs;
}

//-----------------------------------------------------------------------------
