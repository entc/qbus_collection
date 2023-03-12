#include "qbus_obsvbl.h"
#include "qbus_engines.h"

// cape includes
#include <stc/cape_map.h>
#include <fmt/cape_json.h>

//-----------------------------------------------------------------------------

struct QBusSubscriber_s
{
  void* user_ptr;  
  fct_qbus_on_emit user_fct;
};

//-----------------------------------------------------------------------------

QBusSubscriber qbus_subscriber_new (void* user_ptr, fct_qbus_on_emit user_fct)
{
  QBusSubscriber self = CAPE_NEW(struct QBusSubscriber_s);
  
  self->user_ptr = user_ptr;
  self->user_fct = user_fct;
  
  return self;
}

//-----------------------------------------------------------------------------

void qbus_subscriber_del (QBusSubscriber* p_self)
{
  if (*p_self)
  {
    CAPE_DEL (p_self, struct QBusSubscriber_s);
  }
}

//-----------------------------------------------------------------------------

struct QBusObsvblNode_s
{
  CapeMap routings;
  
}; typedef struct QBusObsvblNode_s* QBusObsvblNode;

//-----------------------------------------------------------------------------

void __STDCALL qbus_obsvbl__routings__on_del (void* key, void* val)
{
  {
    CapeString h = key; cape_str_del (&h);
  }
  {
    CapeString h = val; cape_str_del (&h);
  }
}

//-----------------------------------------------------------------------------

QBusObsvblNode qbus_node_new ()
{
  QBusObsvblNode self = CAPE_NEW(struct QBusObsvblNode_s);

  self->routings = cape_map_new (NULL, qbus_obsvbl__routings__on_del, NULL);
  
  return self;
}

//-----------------------------------------------------------------------------

void qbus_node_del (QBusObsvblNode* p_self)
{
  if (*p_self)
  {
    QBusObsvblNode self = *p_self;
    
    cape_map_del (&(self->routings));
    
    CAPE_DEL (p_self, struct QBusObsvblNode_s);
  }
}

//-----------------------------------------------------------------------------

void qbus_node_upd (QBusObsvblNode self, const CapeString module_name, const CapeString uuid)
{
  CapeMapNode n = cape_map_find (self->routings, (void*)uuid);
  
  if (n)
  {
    
  }
  else
  {
    cape_map_insert (self->routings, (void*)cape_str_cp (uuid), (void*)cape_str_cp (module_name));
  }
}

//-----------------------------------------------------------------------------

void qbus_node__append_to_nodes (QBusObsvblNode self, const CapeString subscriber_name, const CapeString module_uuid, CapeUdc nodes)
{
  if (self->routings)
  {
    CapeMapCursor* cursor = cape_map_cursor_create (self->routings, CAPE_DIRECTION_FORW);
    
    while (cape_map_cursor_next (cursor))
    {
      if (cape_str_equal (module_uuid, cape_map_node_key (cursor->node)))
      {
        cape_udc_add_s_cp (nodes, NULL, subscriber_name);
      }
    }
    
    cape_map_cursor_destroy (&cursor);
  }
}

//-----------------------------------------------------------------------------

void qbus_node__verify_nodes (QBusObsvblNode self, CapeUdc nodes)
{
  if (self->routings)
  {
    CapeMapCursor* cursor = cape_map_cursor_create (self->routings, CAPE_DIRECTION_FORW);
    
    while (cape_map_cursor_next (cursor))
    {
      const CapeString uuid = cape_map_node_key (cursor->node);
      CapeUdc n = cape_udc_get (nodes, uuid);
      
      if (n)
      {
        
      }
      else
      {
        cape_map_cursor_erase (self->routings, cursor);
      }
    }
  }
}

//-----------------------------------------------------------------------------

void qbus_node_dump (QBusObsvblNode self)
{
  number_t cnt = 0;

  if (self->routings)
  {
    CapeMapCursor* cursor = cape_map_cursor_create (self->routings, CAPE_DIRECTION_FORW);
    
    while (cape_map_cursor_next (cursor))
    {
      if (cnt)
      {
        printf ("%20s    +-- ", "");
      }
      
      printf ("ooo %s -> %s\n", (CapeString)cape_map_node_key (cursor->node), cape_map_node_value (cursor->node));
      
      cnt++;
    }
    
    cape_map_cursor_destroy (&cursor);
  }
}

//-----------------------------------------------------------------------------

struct QBusObsvbl_s
{
  QBusEngines engines;        // reference
  QBusRoute route;            // reference
  
  CapeMap observables;        // map to store local observables for receiving updates
  CapeMap nodes;              // map of remote observables
};

//-----------------------------------------------------------------------------

void __STDCALL qbus_obsvbl__observables__on_del (void* key, void* val)
{
  {
    CapeString h = key; cape_str_del (&h);
  }
  {
    QBusSubscriber h = val; qbus_subscriber_del (&h);
  }
}

//-----------------------------------------------------------------------------

void __STDCALL qbus_obsvbl__nodes__on_del (void* key, void* val)
{
  {
    CapeString h = key; cape_str_del (&h);
  }
  {
    QBusObsvblNode h = val; qbus_node_del (&h);
  }
}

//-----------------------------------------------------------------------------

QBusObsvbl qbus_obsvbl_new (QBusEngines engines, QBusRoute route)
{
  QBusObsvbl self = CAPE_NEW (struct QBusObsvbl_s);
  
  self->engines = engines;
  self->route = route;
  
  self->observables = cape_map_new (NULL, qbus_obsvbl__observables__on_del, NULL);
  self->nodes = cape_map_new (NULL, qbus_obsvbl__nodes__on_del, NULL);
  
  return self;
}

//-----------------------------------------------------------------------------

void qbus_obsvbl_del (QBusObsvbl* p_self)
{
  if (*p_self)
  {
    QBusObsvbl self = *p_self;
    
    cape_map_del (&(self->nodes));
    cape_map_del (&(self->observables));
    
    CAPE_DEL(p_self, struct QBusObsvbl_s);    
  }
}

//-----------------------------------------------------------------------------

CapeUdc qbus_obsvbl_get (QBusObsvbl self, const CapeString module_name, const CapeString module_uuid)
{
  CapeUdc nodes = cape_udc_new (CAPE_UDC_LIST, NULL);

  // our own observables
  {
    CapeMapCursor* cursor = cape_map_cursor_create (self->observables, CAPE_DIRECTION_FORW);
    
    while (cape_map_cursor_next (cursor))
    {
      const CapeString uuid = qbus_route_uuid_get (self->route);
      
      if (cape_str_equal (uuid, module_uuid))
      {
        cape_udc_add_s_cp (nodes, NULL, (CapeString)cape_map_node_key (cursor->node));
      }
    }
    
    cape_map_cursor_destroy (&cursor);
  }

  // remote observables
  {
    CapeMapCursor* cursor = cape_map_cursor_create (self->nodes, CAPE_DIRECTION_FORW);
    
    while (cape_map_cursor_next (cursor))
    {
      qbus_node__append_to_nodes (cape_map_node_value (cursor->node), (CapeString)cape_map_node_key (cursor->node), module_uuid, nodes);
    }
    
    cape_map_cursor_destroy (&cursor);
  }

  return nodes;
}

//-----------------------------------------------------------------------------

void qbus_obsvbl_set__node (QBusObsvbl self, const CapeString subscriber_name, const CapeString module_name, const CapeString module_uuid)
{
  CapeMapNode n = cape_map_find (self->nodes, (void*)subscriber_name);
  if (n)
  {
    QBusObsvblNode node = cape_map_node_value (n);
    
    qbus_node_upd (node, module_name, module_uuid);
  }
  else
  {
    QBusObsvblNode node = qbus_node_new ();
    
    qbus_node_upd (node, module_name, module_uuid);
    
    cape_map_insert (self->nodes, cape_str_cp (subscriber_name), (void*)node);
  }

}

//-----------------------------------------------------------------------------

void qbus_obsvbl_set (QBusObsvbl self, const CapeString module_name, const CapeString module_uuid, CapeUdc observables)
{
  if (observables)
  {
    CapeUdcCursor* cursor = cape_udc_cursor_new (observables, CAPE_DIRECTION_FORW);
    
    while (cape_udc_cursor_next (cursor))
    {
      qbus_obsvbl_set__node (self, cape_udc_s (cursor->item, NULL), module_name, module_uuid);
    }
    
    cape_udc_cursor_del (&cursor);
  }

  qbus_obsvbl_dump (self);
}

//-----------------------------------------------------------------------------

QBusSubscriber qbus_obsvbl_subscribe (QBusObsvbl self, const CapeString module_name, const CapeString value_name, void* user_ptr, fct_qbus_on_emit user_fct)
{
  QBusSubscriber ret = NULL;
  
  CapeString subscriber_name = cape_str_catenate_c (module_name, '.', value_name);
    
  cape_str_to_lower (subscriber_name);
  
  {
    CapeMapNode n = cape_map_find (self->observables, (void*)subscriber_name);
    
    if (n == NULL)
    {
      ret = qbus_subscriber_new (user_ptr, user_fct);
      
      cape_map_insert (self->observables, (void*)cape_str_mv (&subscriber_name), (void*)ret);
    }
  }
  
  cape_str_del (&subscriber_name);
  
  return ret;
}

//-----------------------------------------------------------------------------

QBusSubscriber qbus_obsvbl_subscribe_uuid (QBusObsvbl self, const CapeString uuid, const CapeString value_name, void* user_ptr, fct_qbus_on_emit user_fct)
{
  
}

//-----------------------------------------------------------------------------

void qbus_obsvbl_emit__ptrs (QBusObsvbl self, const CapeString value_name, CapeMap routings, CapeUdc* p_value)
{
  // fetch a map <uuid:conn> from the routing
  // -> we need the uuid to add to the frame as identifier
  CapeMap user_ptrs = qbus_route_get__routings (self->route, routings);

  // only continue with endpoints found
  if (cape_map_size (user_ptrs) > 0)
  {
    QBusFrame frame = qbus_frame_new ();
    
    // assign payload to frame
    qbus_frame_set__payload (frame, p_value);
    
    {
      CapeMapCursor* cursor = cape_map_cursor_create (user_ptrs, CAPE_DIRECTION_FORW);
      
      while (cape_map_cursor_next (cursor))
      {
        // override the frame credentials
        // -> use the chainkey as name to identify the module of the value
        // -> use the module as destination uuid
        // -> use the method for the value name
        qbus_frame_set (frame, QBUS_FRAME_TYPE_OBSVBL_VALUE, qbus_route_name_get (self->route), cape_map_node_key (cursor->node), value_name, qbus_route_uuid_get (self->route));
        
        // send the frame to the conn
        qbus_engines__send (self->engines, frame, cape_map_node_value (cursor->node));
      }
      
      cape_map_cursor_destroy (&cursor);
    }
    
    qbus_frame_del (&frame);
  }
  
  cape_map_del (&user_ptrs);
  cape_udc_del (p_value);
}

//-----------------------------------------------------------------------------

void qbus_obsvbl_emit (QBusObsvbl self, const CapeString value_name, CapeUdc* p_value)
{
  CapeString subscriber_name = cape_str_catenate_c (qbus_route_name_get (self->route), '.', value_name);
  
  cape_str_to_lower (subscriber_name);

  // find the nodes assigned to the subscriber name
  CapeMapNode n = cape_map_find (self->nodes, (void*)subscriber_name);
  if (n)
  {
    QBusObsvblNode node = cape_map_node_value (n);

    // send to all ptrs
    qbus_obsvbl_emit__ptrs (self, value_name, node->routings, p_value);
  }
  
  cape_udc_del (p_value);
  cape_str_del (&subscriber_name);
}

//-----------------------------------------------------------------------------

void qbus_obsvbl_value (QBusObsvbl self, const CapeString module_name, const CapeString value_name, CapeUdc* p_value)
{
  CapeString subscriber_name = cape_str_catenate_c (module_name, '.', value_name);
  
  cape_str_to_lower (subscriber_name);
  
  // find the nodes assigned to the subscriber name
  CapeMapNode n = cape_map_find (self->observables, (void*)subscriber_name);
  if (n)
  {
    QBusSubscriber subscriber = cape_map_node_value (n);
    
    // TODO: add callback to queue model
    if (subscriber->user_fct)
    {
      subscriber->user_fct (subscriber->user_ptr, subscriber_name, p_value);
    }
  }

  cape_str_del (&subscriber_name);
  cape_udc_del (p_value);
}

//-----------------------------------------------------------------------------

void qbus_obsvbl_subloads (QBusObsvbl self, QBusPvdConnection conn)
{
  CapeUdc nodes = qbus_route_node_get (self->route, TRUE);

  {
    CapeMapCursor* cursor = cape_map_cursor_create (self->nodes, CAPE_DIRECTION_FORW);
    
    while (cape_map_cursor_next (cursor))
    {
      qbus_node__verify_nodes (cape_map_node_value (cursor->node), nodes);
    }
    
    cape_map_cursor_destroy (&cursor);
  }
  
  qbus_obsvbl_dump (self);
}

//-----------------------------------------------------------------------------

void qbus_obsvbl_dump (QBusObsvbl self)
{
  CapeMapCursor* cursor = cape_map_cursor_create (self->nodes, CAPE_DIRECTION_FORW);
  
  printf ("-----------------------------------------------------------------------------\n");
  
  while (cape_map_cursor_next (cursor))
  {
    QBusObsvblNode node = cape_map_node_value (cursor->node);

    printf ("%20s: --+-- ", (CapeString)cape_map_node_key (cursor->node));
    
    qbus_node_dump (node);
  }
  
  cape_map_cursor_destroy (&cursor);
  
  printf ("-----------------------------------------------------------------------------\n");
}

//-----------------------------------------------------------------------------
