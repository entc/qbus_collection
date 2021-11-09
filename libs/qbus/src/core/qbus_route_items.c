#include "qbus_route_items.h"

// cape includes
#include <sys/cape_types.h>
#include <sys/cape_mutex.h>
#include <stc/cape_map.h>
#include <fmt/cape_json.h>
#include <sys/cape_log.h>

//-----------------------------------------------------------------------------

struct QBusRouteDirectConn_s
{
  QBusConnection conn;
  number_t rr_current;
  
}; typedef struct QBusRouteDirectConn_s* QBusRouteDirectConn;

//-----------------------------------------------------------------------------

QBusRouteDirectConn qbus_route_direct_conn_new (QBusConnection conn, number_t rr_context)
{
  QBusRouteDirectConn self = CAPE_NEW (struct QBusRouteDirectConn_s);
  
  self->conn = conn;
  self->rr_current = rr_context;
  
  return self;
}

//-----------------------------------------------------------------------------

void qbus_route_direct_conn_del (QBusRouteDirectConn* p_self)
{
  if (*p_self)
  {
    QBusRouteDirectConn self = *p_self;
    
    CAPE_DEL (p_self, struct QBusRouteDirectConn_s);
  }
}

//-----------------------------------------------------------------------------

struct QBusRouteDirectContext_s
{
  // store the connection by uuid
  CapeMap connections;
  
  // store the connection if there is no uuid
  // -> old version of QBUS protocol and will be depricated soon
  QBusRouteDirectConn rdn;
  
  number_t rr_context;
  
}; typedef struct QBusRouteDirectContext_s* QBusRouteDirectContext;

//-----------------------------------------------------------------------------

void __STDCALL qbus_route_direct_context__connections__on_del (void* key, void* val)
{
  {
    CapeString h = key; cape_str_del (&h);
  }
  {
    QBusRouteDirectConn h = val; qbus_route_direct_conn_del (&h);
  }
}

//-----------------------------------------------------------------------------

QBusRouteDirectContext qbus_route_direct_context_new (void)
{
  QBusRouteDirectContext self = CAPE_NEW (struct QBusRouteDirectContext_s);
  
  self->connections = cape_map_new (NULL, qbus_route_direct_context__connections__on_del, NULL);
  self->rdn = NULL;
  self->rr_context = 0;
  
  return self;
}

//-----------------------------------------------------------------------------

void qbus_route_direct_context_del (QBusRouteDirectContext* p_self)
{
  if (*p_self)
  {
    QBusRouteDirectContext self = *p_self;
    
    cape_map_del (&(self->connections));
    qbus_route_direct_conn_del (&(self->rdn));
    
    CAPE_DEL (p_self, struct QBusRouteDirectContext_s);
  }
}

//-----------------------------------------------------------------------------

void qbus_route_direct_context_add (QBusRouteDirectContext self, const CapeString module_uuid, QBusConnection conn)
{
  if (module_uuid)
  {
    QBusRouteDirectConn rdn = qbus_route_direct_conn_new (conn, self->rr_context);
    
    cape_log_fmt (CAPE_LL_TRACE, "QBUS", "route context", "add connection to pool for %s", module_uuid);
    
    // lets have another map to sort out all uuids
    cape_map_insert (self->connections, (void*)cape_str_cp (module_uuid), (void*)rdn);
  }
  else
  {
    if (self->rdn)
    {
      cape_log_msg (CAPE_LL_ERROR, "QBUS", "route context", "doublicate module names are not allowed for none uuid modules");
    }
    else
    {
      self->rdn = qbus_route_direct_conn_new (conn, self->rr_context);
    }
  }
}

//-----------------------------------------------------------------------------

QBusRouteDirectConn qbus_route_direct_context__round_robin__next (QBusRouteDirectContext self)
{
  QBusRouteDirectConn ret = NULL;
  
  // local objects
  CapeMapCursor* cursor = NULL;
  
  if (self->rdn)
  {
    QBusRouteDirectConn rdn = self->rdn;
    
    if (rdn->rr_current < self->rr_context)
    {
      rdn->rr_current = self->rr_context;
      
      //cape_log_fmt (CAPE_LL_TRACE, "QBUS", "direct route", "found a connection [%i] (old version)", rdn->rr_current);

      ret = rdn;
      goto exit_and_cleanup;
    }
  }
  
  cursor = cape_map_cursor_create (self->connections, CAPE_DIRECTION_FORW);

  while (cape_map_cursor_next (cursor))
  {
    QBusRouteDirectConn rdn = cape_map_node_value (cursor->node);

    if (rdn->rr_current < self->rr_context)
    {
      rdn->rr_current = self->rr_context;

      //cape_log_fmt (CAPE_LL_TRACE, "QBUS", "direct route", "found a connection [%i] in the pool = %s", rdn->rr_current, cape_map_node_key (cursor->node));

      ret = rdn;
      goto exit_and_cleanup;
    }
  }
  
exit_and_cleanup:
  
  cape_map_cursor_destroy (&cursor);
  return ret;
}

//-----------------------------------------------------------------------------

QBusRouteDirectConn qbus_route_direct_context__round_robin (QBusRouteDirectContext self)
{
  QBusRouteDirectConn ret = NULL;
  
  ret = qbus_route_direct_context__round_robin__next (self);

  if (ret == NULL)
  {
    self->rr_context++;

    ret = qbus_route_direct_context__round_robin__next (self);
  }

  return ret;
}

//-----------------------------------------------------------------------------

QBusConnection qbus_route_direct_context_get (QBusRouteDirectContext self)
{
  QBusConnection ret = NULL;
  
  QBusRouteDirectConn rdn = qbus_route_direct_context__round_robin (self);
  if (rdn)
  {
    ret = rdn->conn;
  }

  return ret;
}

//-----------------------------------------------------------------------------

QBusConnection qbus_route_direct_context_rm (QBusRouteDirectContext self, const CapeString module_uuid)
{
  QBusConnection ret = NULL;

  if (cape_str_empty (module_uuid))
  {
    if (self->rdn)
    {
      QBusRouteDirectConn rdn = self->rdn;

      ret = rdn->conn;
      
      qbus_route_direct_conn_del (&(self->rdn));
    }
  }
  else
  {
    CapeMapNode n = cape_map_find (self->connections, (void*)module_uuid);
    
    if (n)
    {
      QBusRouteDirectConn rdn = cape_map_node_value (n);

      ret = rdn->conn;
      
      // remove the connection
      cape_map_erase (self->connections, n);
    }
  }
  
  return ret;
}

//-----------------------------------------------------------------------------

int qbus_route_direct_context_is_empty (QBusRouteDirectContext self)
{
  return (cape_map_size (self->connections) == 0) && (self->rdn == NULL);
}

//-----------------------------------------------------------------------------

void qbus_route_direct_context__internal__append (CapeList conns, QBusConnection conn, QBusConnection exception)
{
  if (conn != exception)
  {
    cape_list_push_back (conns, (void*)conn);
  }
}

//-----------------------------------------------------------------------------

void qbus_route_direct_context__append_connections (QBusRouteDirectContext self, CapeList conns, QBusConnection exception)
{
  // local objects
  CapeMapCursor* cursor = cape_map_cursor_create (self->connections, CAPE_DIRECTION_FORW);
  
  // check for the old version
  if (self->rdn)
  {
    QBusRouteDirectConn rdn = self->rdn;

    qbus_route_direct_context__internal__append (conns, rdn->conn, exception);
  }
  
  // append all connection to conns
  while (cape_map_cursor_next (cursor))
  {
    QBusRouteDirectConn rdn = cape_map_node_value (cursor->node);

    qbus_route_direct_context__internal__append (conns, rdn->conn, exception);
  }
  
  cape_map_cursor_destroy (&cursor);
}

//-----------------------------------------------------------------------------

struct QBusRouteItems_s
{
  CapeMutex mutex;
  
  CapeMap routes_direct;
  CapeMap routes_node;
};

//-----------------------------------------------------------------------------

void __STDCALL qbus_route_routes_direct_del (void* key, void* val)
{
  {
    CapeString h = key; cape_str_del (&h);
  }
  {
    QBusRouteDirectContext h = val; qbus_route_direct_context_del (&h);
  }
}

//-----------------------------------------------------------------------------

void __STDCALL qbus_route_routes_nodes_del (void* key, void* val)
{
  {
    CapeString h = key; cape_str_del (&h);
  }
}

//-----------------------------------------------------------------------------

QBusRouteItems qbus_route_items_new (void)
{
  QBusRouteItems self = CAPE_NEW (struct QBusRouteItems_s);
  
  self->routes_direct = cape_map_new (NULL, qbus_route_routes_direct_del, NULL);
  self->routes_node = cape_map_new (NULL, qbus_route_routes_nodes_del, NULL);
  
  self->mutex = cape_mutex_new ();

  return self;  
}

//-----------------------------------------------------------------------------

void qbus_route_items_del (QBusRouteItems* p_self)
{
  QBusRouteItems self = *p_self;
  
  cape_map_del (&(self->routes_direct));
  cape_map_del (&(self->routes_node));

  cape_mutex_del (&(self->mutex));

  CAPE_DEL (p_self, struct QBusRouteItems_s);  
}

//-----------------------------------------------------------------------------

void qbus_route_items_node_add (QBusRouteItems self, QBusConnection conn, CapeUdc item)
{
  switch (cape_udc_type (item))
  {
    case CAPE_UDC_STRING:
    {
      const CapeString remote_module = cape_udc_s (item, NULL);
      if (remote_module)
      {
        CapeString h = cape_str_cp (remote_module);
        
        cape_str_to_upper (h);
        
        {
          CapeMapNode n = cape_map_find (self->routes_node, (void*)h);
          if (n == NULL)
          {
            //printf ("ROUTE NODES: insert %s -> %p\n", h, conn);
            
            cape_map_insert (self->routes_node, (void*)h, (void*)conn);
          }
          else
          {
            cape_str_del (&h);
          }
        }
      }
    }
  }
}

//-----------------------------------------------------------------------------

void qbus_route_items_nodes_add (QBusRouteItems self, QBusConnection conn, CapeUdc nodes)
{
  switch (cape_udc_type (nodes))
  {
    case CAPE_UDC_LIST:
    {
      CapeUdcCursor* cursor = cape_udc_cursor_new (nodes, CAPE_DIRECTION_FORW);

      while (cape_udc_cursor_next (cursor))
      {
        qbus_route_items_node_add (self, conn, cursor->item);
      }
      
      cape_udc_cursor_del (&cursor);
      
      break;
    }
  }
}

//-----------------------------------------------------------------------------

void qbus_route_items_nodes_remove_all (QBusRouteItems self, QBusConnection conn)
{
  CapeMapCursor* cursor = cape_map_cursor_create (self->routes_node, CAPE_DIRECTION_FORW);
  
  while (cape_map_cursor_next (cursor))
  {
    QBusConnection conn_remote = cape_map_node_value (cursor->node);
    
    if (conn_remote == conn)
    {
      //printf ("ROUTE NODES: remove %s\n", (CapeString)cape_map_node_key (cursor->node));
      
      cape_map_cursor_erase (self->routes_node, cursor);      
    }
  }
  
  cape_map_cursor_destroy (&cursor);
}

//-----------------------------------------------------------------------------

void qbus_route_items_add (QBusRouteItems self, const CapeString module_name, const CapeString module_uuid, QBusConnection conn, CapeUdc* p_nodes)
{
  cape_mutex_lock (self->mutex);
  
  {
    CapeString module = cape_str_cp (module_name);
    
    cape_str_to_upper (module);
    
    CapeMapNode n = cape_map_find (self->routes_direct, (void*)module);
    if (n)
    {
      QBusRouteDirectContext rdc = cape_map_node_value (n);
      
      qbus_route_direct_context_add (rdc, module_uuid, conn);
    }
    else
    {
      QBusRouteDirectContext rdc = qbus_route_direct_context_new ();
      
      qbus_route_direct_context_add (rdc, module_uuid, conn);

      cape_map_insert (self->routes_direct, (void*)module, (void*)rdc);
    }
    
    // in the old version the module_uuid is NULL
    qbus_connection_set (conn, module, module_uuid);
  }
  
exit_and_cleanup:
  
  if (*p_nodes)
  {
    qbus_route_items_nodes_add (self, conn, *p_nodes);
    
    cape_udc_del (p_nodes);    
  }
  
  cape_mutex_unlock (self->mutex);
}

//-----------------------------------------------------------------------------

void qbus_route_items_update (QBusRouteItems self, QBusConnection conn, CapeUdc* p_nodes)
{
  cape_mutex_lock (self->mutex);
  
  // direct
  qbus_route_items_nodes_remove_all (self, conn);
  
  if (*p_nodes)
  {
    qbus_route_items_nodes_add (self, conn, *p_nodes);
  }
  
  cape_mutex_unlock (self->mutex);
  cape_udc_del (p_nodes);
}

//-----------------------------------------------------------------------------

QBusConnection qbus_route_items_get (QBusRouteItems self, const CapeString module_name, const CapeString module_uuid)
{
  QBusConnection ret = NULL;
  
  // convert module name into capital letters
  CapeString module = cape_str_cp (module_name);
  cape_str_to_upper (module);
  
  cape_mutex_lock (self->mutex);

  // direct
  {
    CapeMapNode n = cape_map_find (self->routes_direct, (void*)module);
    if (n)
    {
      QBusRouteDirectContext rdc = cape_map_node_value (n);
      
      ret = qbus_route_direct_context_get (rdc);
      if (ret)
      {
        goto exit_and_cleanup;
      }
    }
  }
  
  // node
  {
    CapeMapNode n = cape_map_find (self->routes_node, (void*)module);    
    if (n)
    {
      ret = cape_map_node_value (n);
      goto exit_and_cleanup;
    }
  }
  
exit_and_cleanup:

  cape_str_del (&module);
  cape_mutex_unlock (self->mutex);

  return ret;
}

//-----------------------------------------------------------------------------

void qbus_route_items_rm (QBusRouteItems self, const CapeString module_name, const CapeString module_uuid)
{
  cape_mutex_lock (self->mutex);
  
  {
    CapeMapNode n = cape_map_find (self->routes_direct, (void*)module_name);
    
    if (n)
    {
      QBusRouteDirectContext rdc = cape_map_node_value (n);

      QBusConnection conn = qbus_route_direct_context_rm (rdc, module_uuid);
      
      if (conn)
      {
        // remove all indirect nodes with this connection
        qbus_route_items_nodes_remove_all (self, conn);
      }

      //printf ("qbus connection removed: %s\n", module);
      
      if (qbus_route_direct_context_is_empty (rdc))
      {
        // remove the connection
        cape_map_erase (self->routes_direct, n);
      }
    }
  }
  
  cape_mutex_unlock (self->mutex);
}

//-----------------------------------------------------------------------------

CapeUdc qbus_route_items_nodes (QBusRouteItems self)
{
  CapeUdc nodes = cape_udc_new (CAPE_UDC_LIST, NULL);
 
  cape_mutex_lock (self->mutex);

  // iterate through all direct connections
  {
    CapeMapCursor* cursor = cape_map_cursor_create (self->routes_direct, CAPE_DIRECTION_FORW);
    
    while (cape_map_cursor_next (cursor))
    {
      cape_udc_add_s_cp (nodes, NULL, cape_map_node_key (cursor->node));
    }
    
    cape_map_cursor_destroy (&cursor);
  }

  // iterate through all node connections
  {
    CapeMapCursor* cursor = cape_map_cursor_create (self->routes_node, CAPE_DIRECTION_FORW);
    
    while (cape_map_cursor_next (cursor))
    {
      cape_udc_add_s_cp (nodes, NULL, cape_map_node_key (cursor->node));      
    }
    
    cape_map_cursor_destroy (&cursor);
  }
  
  cape_mutex_unlock (self->mutex);

  return nodes;
}

//-----------------------------------------------------------------------------

CapeList qbus_route_items_conns (QBusRouteItems self, QBusConnection exception)
{
  CapeList conns = cape_list_new (NULL);
  
  cape_mutex_lock (self->mutex);
  
  // iterate through all direct connections
  {
    CapeMapCursor* cursor = cape_map_cursor_create (self->routes_direct, CAPE_DIRECTION_FORW);
    
    while (cape_map_cursor_next (cursor))
    {
      QBusRouteDirectContext rdc = cape_map_node_value (cursor->node);

      qbus_route_direct_context__append_connections (rdc, conns, exception);
    }
    
    cape_map_cursor_destroy (&cursor);
  }
  
  cape_mutex_unlock (self->mutex);
  
  return conns;
}

//-----------------------------------------------------------------------------

