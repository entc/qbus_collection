#include "qbus_route.h"
#include "qbus_engines.h"

// cape includes
#include <sys/cape_file.h>
#include <sys/cape_log.h>
#include <sys/cape_thread.h>
#include <fmt/cape_args.h>
#include <fmt/cape_json.h>
#include <fmt/cape_tokenizer.h>
#include <stc/cape_map.h>

//-----------------------------------------------------------------------------

#define QBUS_ROUTE_NODE_TYPE__SELF   1
#define QBUS_ROUTE_NODE_TYPE__LOCAL  2
#define QBUS_ROUTE_NODE_TYPE__PROXY  3

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

struct QBusRouteNode_s
{
  CapeMap local_conns;           // a list of direct connections
  CapeMap proxy_conns;           // via a proxy connection
  
}; typedef struct QBusRouteNode_s* QBusRouteNode;

//-----------------------------------------------------------------------------

void __STDCALL qbus_route__items__on_del (void* key, void* val)
{
  {

  }
  {
    QBusRouteConn h = val; qbus_route_conn_del (&h);
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

void qbus_route_node__add_local_item (QBusRouteNode self, QBusObsvbl obsvbl, const CapeString module_uuid, QBusPvdConnection conn)
{
  if (self->local_conns == NULL)
  {
    self->local_conns = cape_map_new (cape_map__compare__n, qbus_route__items__on_del, NULL);
    
    cape_map_insert (self->local_conns, conn, (void*)qbus_route_conn_new (module_uuid, obsvbl));
  }
  else
  {
    CapeMapNode n = cape_map_find (self->local_conns, (void*)conn);
    if (n == NULL)
    {
      cape_map_insert (self->local_conns, conn, (void*)qbus_route_conn_new (module_uuid, obsvbl));
    }
  }
}

//-----------------------------------------------------------------------------

void qbus_route_node__add_proxy_item (QBusRouteNode self, QBusObsvbl obsvbl, const CapeString module_uuid, QBusPvdConnection conn)
{
  if (self->proxy_conns == NULL)
  {
    self->proxy_conns = cape_map_new (NULL, qbus_route__items__on_del, NULL);
    
    cape_map_insert (self->proxy_conns, (void*)conn, (void*)qbus_route_conn_new (module_uuid, obsvbl));
  }
  else
  {
    CapeMapNode n = cape_map_find (self->proxy_conns, (void*)conn);
    if (n == NULL)
    {
      cape_map_insert (self->proxy_conns, (void*)conn, (void*)qbus_route_conn_new (module_uuid, obsvbl));
    }
  }  
}

//-----------------------------------------------------------------------------

void qbus_route_node__proxy_conns__clr (QBusRouteNode self, QBusPvdConnection conn)
{
  CapeMapNode n = cape_map_find (self->proxy_conns, (void*)conn);
  if (n)
  {
    cape_map_erase (self->proxy_conns, n);
  }
}

//-----------------------------------------------------------------------------

int qbus_route_node__clr_proxy (QBusRouteNode self, QBusPvdConnection conn)
{
  int remove_node = FALSE;
  
  if (self->local_conns)
  {
    remove_node = (cape_map_size (self->local_conns) == 0);
  }
  else
  {
    remove_node = TRUE;
  }
  
  if (self->proxy_conns)
  {
    qbus_route_node__proxy_conns__clr (self, conn);
    
    remove_node = remove_node && (cape_map_size (self->proxy_conns) == 0);
  }
  else
  {
    remove_node = remove_node && TRUE;
  }
  
  return remove_node;
}

//-----------------------------------------------------------------------------

int qbus_route_node__rm (QBusRouteNode self, QBusPvdConnection conn)
{
  int remove_node = FALSE;
  
  if (self->local_conns)
  {
    CapeMapNode n = cape_map_find (self->local_conns, (void*)conn);
    if (n)
    {
      cape_map_erase (self->local_conns, n);
    }
    
    remove_node = (cape_map_size (self->local_conns) == 0);    
  }
  else
  {
    remove_node = TRUE;
  }
  
  if (self->proxy_conns)
  {
    qbus_route_node__proxy_conns__clr (self, conn);
    
    remove_node = remove_node && (cape_map_size (self->proxy_conns) == 0);    
  }
  else
  {
    remove_node = remove_node && TRUE;
  }
  
  return remove_node;
}

//-----------------------------------------------------------------------------

void qbus_route_node__dump (QBusRouteNode self)
{
  number_t cnt = 0;
  
  if (self->local_conns)
  {
    CapeMapCursor* cursor = cape_map_cursor_create (self->local_conns, CAPE_DIRECTION_FORW);
    
    while (cape_map_cursor_next (cursor))
    {
      QBusRouteConn conn = cape_map_node_value (cursor->node);
      
      if (cnt)
      {
        printf ("%10s    +-- ", "");
      }
      
      printf ("ooo %p -> %s\n", (CapeString)cape_map_node_key (cursor->node), conn->module_uuid);
      
      cnt++;
    }
    
    cape_map_cursor_destroy (&cursor);
  }

  if (self->proxy_conns)
  {
    CapeMapCursor* cursor = cape_map_cursor_create (self->proxy_conns, CAPE_DIRECTION_FORW);
    
    while (cape_map_cursor_next (cursor))
    {
      QBusRouteConn conn = cape_map_node_value (cursor->node);

      if (cnt)
      {
        printf ("%10s    +-- ", "");
      }
      
      printf (">>> %p -> %s\n", (CapeString)cape_map_node_key (cursor->node), conn->module_uuid);

      cnt++;
    }
    
    cape_map_cursor_destroy (&cursor);
  }
}

//-----------------------------------------------------------------------------

int qbus_route_node__has_local_connections (QBusRouteNode self)
{
  int ret = FALSE;
  
  if (self->local_conns)
  {
    ret = (cape_map_size (self->local_conns) > 0);
  }
    
  return ret;
}

//-----------------------------------------------------------------------------

void qbus_route_node__get_local_connections (QBusRouteNode self, CapeList user_ptrs, QBusPvdConnection conn_excluded)
{
  if (self->local_conns)
  {
    CapeMapCursor* cursor = cape_map_cursor_create (self->local_conns, CAPE_DIRECTION_FORW);
    
    while (cape_map_cursor_next (cursor))
    {
      QBusPvdConnection conn = cape_map_node_key (cursor->node);
      
      if (conn != conn_excluded)
      {
        cape_list_push_back (user_ptrs, conn);
      }
    }
    
    cape_map_cursor_destroy (&cursor);
  }
}

//-----------------------------------------------------------------------------

void qbus_route_node__get_routings__append (const CapeString uuid, CapeMap user_ptrs, QBusPvdConnection conn)
{
  CapeMapNode n = cape_map_find (user_ptrs, (void*)uuid);
  if (n == NULL)
  {
    cape_map_insert (user_ptrs, (void*)conn, (void*)cape_str_cp (uuid));
  }
}

//-----------------------------------------------------------------------------

QBusPvdConnection qbus_route_node__seek (CapeMap conns, const CapeString uuid)
{
  QBusPvdConnection ret = NULL;
  
  CapeMapCursor* cursor = cape_map_cursor_create (conns, CAPE_DIRECTION_FORW);
  
  while (cape_map_cursor_next (cursor))
  {
    QBusRouteConn conn = cape_map_node_value (cursor->node);
    
    if (cape_str_equal (conn->module_uuid, uuid))
    {
      
      ret = cape_map_node_key (cursor->node);


      printf ("found conn: %p\n", ret);
      break;
    }
  }
  
  cape_map_cursor_destroy (&cursor);

  return ret;
}

//-----------------------------------------------------------------------------

void qbus_route_node__get_routings (QBusRouteNode self, CapeMap user_ptrs, CapeMap routings)
{
  printf ("get routings\n");
  
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
  
  if (self->proxy_conns)
  {
    CapeMapCursor* cursor = cape_map_cursor_create (routings, CAPE_DIRECTION_FORW);
    
    while (cape_map_cursor_next (cursor))
    {
      const CapeString uuid = cape_map_node_key (cursor->node);
      
      QBusPvdConnection conn = qbus_route_node__seek (self->proxy_conns, uuid);
      if (conn)
      {
        qbus_route_node__get_routings__append (uuid, user_ptrs, conn);
      }
    }

    cape_map_cursor_destroy (&cursor);
  }  
}

//-----------------------------------------------------------------------------

void qbus_route_node__append_to_nodes (QBusRouteNode self, QBusObsvbl obsvbl, CapeUdc nodes, const CapeString module_name)
{
  if (self->local_conns)
  {
    CapeMapCursor* cursor = cape_map_cursor_create (self->local_conns, CAPE_DIRECTION_FORW);
    
    while (cape_map_cursor_next (cursor))
    {
      QBusRouteConn conn = cape_map_node_value (cursor->node);

      if (conn->module_uuid)
      {
        CapeUdc node = cape_udc_new (CAPE_UDC_NODE, conn->module_uuid);
        
        cape_udc_add_s_cp (node, "module", module_name);
        cape_udc_add_n (node, "type", QBUS_ROUTE_NODE_TYPE__LOCAL);

        {
          CapeUdc observables = qbus_obsvbl_get (obsvbl, module_name, conn->module_uuid);
          
          cape_udc_add_name (node, &observables, "obsvbls");
        }
        
        cape_udc_add (nodes, &node);
      }
    }
    
    cape_map_cursor_destroy (&cursor);
  }
  
  if (self->proxy_conns)
  {
    CapeMapCursor* cursor = cape_map_cursor_create (self->proxy_conns, CAPE_DIRECTION_FORW);
    
    while (cape_map_cursor_next (cursor))
    {
      QBusRouteConn conn = cape_map_node_value (cursor->node);

      if (conn->module_uuid)
      {
        CapeUdc node = cape_udc_new (CAPE_UDC_NODE, conn->module_uuid);
        
        cape_udc_add_s_cp (node, "module", module_name);
        cape_udc_add_n (node, "type", QBUS_ROUTE_NODE_TYPE__PROXY);
        
        {
          CapeUdc observables = qbus_obsvbl_get (obsvbl, module_name, conn->module_uuid);
          
          cape_udc_add_name (node, &observables, "obsvbls");
        }
        
        cape_udc_add (nodes, &node);
      }
    }
    
    cape_map_cursor_destroy (&cursor);
  }
}

//-----------------------------------------------------------------------------

struct QBusRoute_s
{
  QBusEngines engines;        // reference
  QBusObsvbl obsvbl;          // reference
  
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

QBusRoute qbus_route_new (const CapeString name, QBusEngines engines)
{
  QBusRoute self = CAPE_NEW (struct QBusRoute_s);

  self->engines = engines;
  self->obsvbl = NULL;
  
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

CapeUdc qbus_route_node_get (QBusRoute self, int as_node)
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
      CapeMapCursor* cursor = cape_map_cursor_create (self->nodes, CAPE_DIRECTION_FORW);
      
      while (cape_map_cursor_next (cursor))
      {
        QBusRouteNode rn = cape_map_node_value (cursor->node);
        
        qbus_route_node__append_to_nodes (rn, self->obsvbl, nodes, cape_map_node_key (cursor->node));
      }
      
      cape_map_cursor_destroy (&cursor);
    }
    
    return nodes;
  }
  else
  {
    CapeUdc nodes = cape_udc_new (CAPE_UDC_LIST, NULL);
    
    CapeMapCursor* cursor = cape_map_cursor_create (self->nodes, CAPE_DIRECTION_FORW);
    
    while (cape_map_cursor_next (cursor))
    {
      QBusRouteNode rn = cape_map_node_value (cursor->node);
      
      if (qbus_route_node__has_local_connections (rn))
      {
        cape_udc_add_s_cp (nodes, NULL, cape_map_node_key (cursor->node));
      }
    }
    
    cape_map_cursor_destroy (&cursor);

    return nodes;
  }
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

void qbus_route__add_local_node (QBusRoute self, const CapeString module_name, const CapeString module_uuid, QBusPvdConnection conn)
{
  QBusRouteNode rn = qbus_route__seek_or_create_node (self, module_name);
  
  qbus_route_node__add_local_item (rn, self->obsvbl, module_uuid, conn);
}

//-----------------------------------------------------------------------------

void qbus_route__add_proxy_node (QBusRoute self, const CapeString module_name, const CapeString module_uuid, QBusPvdConnection conn)
{
  QBusRouteNode rn = qbus_route__seek_or_create_node (self, module_name);

  qbus_route_node__add_proxy_item (rn, self->obsvbl, module_uuid, conn);
}

//-----------------------------------------------------------------------------

void qbus_route_add_nodes__list (QBusRoute self, CapeUdc nodes, QBusPvdConnection conn)
{
  CapeUdcCursor* cursor = cape_udc_cursor_new (nodes, CAPE_DIRECTION_FORW);
  
  while (cape_udc_cursor_next (cursor))
  {
    switch (cape_udc_type (cursor->item))
    {
      case CAPE_UDC_STRING:
      {
        qbus_route__add_proxy_node (self, cape_udc_s (cursor->item, NULL), NULL, conn);
        
        break;
      }
      case CAPE_UDC_NODE:
      {
        const CapeString remote_module = cape_udc_get_s (cursor->item, "module", NULL);
        if (remote_module)
        {
          qbus_route__add_proxy_node (self, remote_module, NULL, conn);
        }

        break;
      }
    }
  }
  
  cape_udc_cursor_del (&cursor);
}

//-----------------------------------------------------------------------------

void qbus_route_add_nodes__node (QBusRoute self, CapeUdc nodes, QBusPvdConnection conn)
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
            if (remote_module)
            {
              qbus_obsvbl_set (self->obsvbl, remote_module, cape_udc_name (cursor->item), cape_udc_get (cursor->item, "obsvbls"));
            }
            
            break;
          }
          case QBUS_ROUTE_NODE_TYPE__LOCAL:
          {
            const CapeString remote_module = cape_udc_get_s (cursor->item, "module", NULL);
            if (remote_module)
            {
              qbus_route__add_proxy_node (self, remote_module, cape_udc_name (cursor->item), conn);

              // check for observables
              {
                qbus_obsvbl_set (self->obsvbl, remote_module, cape_udc_name (cursor->item), cape_udc_get (cursor->item, "obsvbls"));
              }
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

//-----------------------------------------------------------------------------

void qbus_route_add_nodes (QBusRoute self, const CapeString module_name, const CapeString sender_uuid, QBusPvdConnection conn, CapeUdc* p_nodes)
{
  CapeUdc nodes = *p_nodes;

  qbus_route__add_local_node (self, module_name, sender_uuid, conn);
  
  // clear all nodes which belong to the uuid
  {
    CapeMapCursor* cursor = cape_map_cursor_create (self->nodes, CAPE_DIRECTION_FORW);
    
    while (cape_map_cursor_next (cursor))
    {
      QBusRouteNode rn = cape_map_node_value (cursor->node);
      
      if (qbus_route_node__clr_proxy (rn, conn))
      {
        cape_map_cursor_erase (self->nodes, cursor);      
      }
    }
    
    cape_map_cursor_destroy (&cursor);
  }
  
  if (nodes)
  {
    switch (cape_udc_type (nodes))
    {
      case CAPE_UDC_LIST:
      {
        qbus_route_add_nodes__list (self, nodes, conn);
        break;
      }
      case CAPE_UDC_NODE:
      {
        qbus_route_add_nodes__node (self, nodes, conn);
        break;
      }
    }
  }
  
  // tell the others the new nodes
  //  qbus_route_send_updates (self, conn);
  cape_udc_del (p_nodes);
  
  // tell all others our updates
  qbus_route_send_update (self, conn);
  
  qbus_route_dump (self);
}

//-----------------------------------------------------------------------------

void qbus_route_rm (QBusRoute self, QBusPvdConnection conn)
{
  CapeMapCursor* cursor = cape_map_cursor_create (self->nodes, CAPE_DIRECTION_FORW);
  
  while (cape_map_cursor_next (cursor))
  {
    QBusRouteNode node = cape_map_node_value (cursor->node);

    if (qbus_route_node__rm (node, conn))
    {
      cape_map_cursor_erase (self->nodes, cursor);      
    }
  }
  
  cape_map_cursor_destroy (&cursor);

  // tell all others our updates
  qbus_route_send_update (self, conn);
  
  qbus_route_dump (self);
}

//-----------------------------------------------------------------------------

void qbus_route_frame_nodes_add (QBusRoute self, QBusFrame frame, int as_node)
{
  CapeUdc route_nodes = qbus_route_node_get (self, as_node);
  
  if (route_nodes)
  {
    // set the payload frame
    qbus_frame_set_udc (frame, QBUS_MTYPE_JSON, &route_nodes);
  }
}

//-----------------------------------------------------------------------------

void qbus_route_send_update (QBusRoute self, QBusPvdConnection conn)
{
  CapeList user_ptrs = qbus_route_get__conn (self, conn);
  
  if (cape_list_size (user_ptrs) > 0)
  {
    // send old version as list
    {
      QBusFrame frame = qbus_frame_new ();
      
      // CH01: replace self->name with self->uuid and add name as module
      qbus_frame_set (frame, QBUS_FRAME_TYPE_ROUTE_UPD, NULL, self->name, NULL, self->uuid);
      
      // create an UDC structure of all nodes
      qbus_route_frame_nodes_add (self, frame, FALSE);
      
      // send the frame
      qbus_engines__broadcast (self->engines, frame, user_ptrs);
      
      qbus_frame_del (&frame);
    }
    
    // send new version as node
    {
      QBusFrame frame = qbus_frame_new ();
      
      // CH01: replace self->name with self->uuid and add name as module
      qbus_frame_set (frame, QBUS_FRAME_TYPE_ROUTE_UPD, NULL, self->name, NULL, self->uuid);
      
      // create an UDC structure of all nodes
      qbus_route_frame_nodes_add (self, frame, TRUE);
      
      // send the frame
      qbus_engines__broadcast (self->engines, frame, user_ptrs);
      
      qbus_frame_del (&frame);
    }
  }
  
  cape_list_del (&user_ptrs);
}

//-----------------------------------------------------------------------------

void qbus_route_dump (QBusRoute self)
{
  CapeMapCursor* cursor = cape_map_cursor_create (self->nodes, CAPE_DIRECTION_FORW);
  
  printf ("-----------------------------------------------------------------------------\n");
  printf ("NAME      |       GATEWAY             |  UUID\n");

  while (cape_map_cursor_next (cursor))
  {
    QBusRouteNode node = cape_map_node_value (cursor->node);
    
    printf ("%10s: --+-- ", (CapeString)cape_map_node_key (cursor->node));
    
    qbus_route_node__dump (node);
  }
  
  cape_map_cursor_destroy (&cursor);

  printf ("-----------------------------------------------------------------------------\n");
}

//-----------------------------------------------------------------------------

CapeList qbus_route_get__conn (QBusRoute self, QBusPvdConnection conn)
{
  CapeList user_ptrs = cape_list_new (NULL);
  
  {
    CapeMapCursor* cursor = cape_map_cursor_create (self->nodes, CAPE_DIRECTION_FORW);
    
    while (cape_map_cursor_next (cursor))
    {
      QBusRouteNode rn = cape_map_node_value (cursor->node);
      
      qbus_route_node__get_local_connections (rn, user_ptrs, conn);
    }
    
    cape_map_cursor_destroy (&cursor);
  }
  
  return user_ptrs;
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
    CapeMapCursor* cursor = cape_map_cursor_create (self->nodes, CAPE_DIRECTION_FORW);
    
    while (cape_map_cursor_next (cursor))
    {
      QBusRouteNode rn = cape_map_node_value (cursor->node);
      
      qbus_route_node__get_routings (rn, user_ptrs, routings);
    }
    
    cape_map_cursor_destroy (&cursor);
  }
  
  return user_ptrs;
}

//-----------------------------------------------------------------------------
