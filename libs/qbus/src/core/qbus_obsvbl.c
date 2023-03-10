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

struct QBusNode_s
{
  CapeMap routings;
  
}; typedef struct QBusNode_s* QBusNode; 

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

QBusNode qbus_node_new ()
{
  QBusNode self = CAPE_NEW(struct QBusNode_s);

  self->routings = cape_map_new (NULL, qbus_obsvbl__routings__on_del, NULL);
  
  return self;
}

//-----------------------------------------------------------------------------

void qbus_node_del (QBusNode* p_self)
{
  if (*p_self)
  {
    QBusNode self = *p_self;
    
    cape_map_del (&(self->routings));
    
    CAPE_DEL (p_self, struct QBusNode_s);
  }
}

//-----------------------------------------------------------------------------

void qbus_node_upd (QBusNode self, const CapeString module_name, const CapeString uuid)
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
    QBusNode h = val; qbus_node_del (&h);
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

CapeUdc qbus_obsvbl_get_observables (QBusObsvbl self)
{
  CapeUdc nodes = cape_udc_new (CAPE_UDC_LIST, NULL);
  
  CapeMapCursor* cursor = cape_map_cursor_create (self->observables, CAPE_DIRECTION_FORW);
  
  while (cape_map_cursor_next (cursor))
  {    
    cape_udc_add_s_cp (nodes, NULL, (CapeString)cape_map_node_key (cursor->node));
  }
  
  cape_map_cursor_destroy (&cursor);
  
  return nodes;
}

//-----------------------------------------------------------------------------

void qbus_obsvbl_add_nodes (QBusObsvbl self, const CapeString module_name, const CapeString module_uuid, QBusPvdConnection conn, CapeUdc* p_nodes)
{
  CapeUdc nodes = *p_nodes;
  
  if (nodes)
  {
    CapeUdcCursor* cursor = cape_udc_cursor_new (nodes, CAPE_DIRECTION_FORW);
    
    while (cape_udc_cursor_next (cursor))
    {
      switch (cape_udc_type (cursor->item))
      {
        case CAPE_UDC_STRING:
        {
          const CapeString subscriber_name = cape_udc_s (cursor->item, NULL);
          
          if (subscriber_name)
          {
            CapeMapNode n = cape_map_find (self->nodes, (void*)subscriber_name);
            
            if (n)
            {
              QBusNode node = cape_map_node_value (n);
              
              qbus_node_upd (node, module_name, module_uuid);
            }
            else
            {
              QBusNode node = qbus_node_new ();

              qbus_node_upd (node, module_name, module_uuid);

              cape_map_insert (self->nodes, cape_str_cp (subscriber_name), (void*)node);
            }
          }

          break;
        }
      }
    }

    cape_udc_cursor_del (&cursor);
  }
  
  cape_udc_del (p_nodes);
}

//-----------------------------------------------------------------------------

void qbus_obsvbl_send_update (QBusObsvbl self, QBusPvdConnection conn_ex, QBusPvdConnection conn_di)
{
  CapeList user_ptrs = cape_list_new (NULL);
  
  if (conn_di)
  {
    cape_list_push_back (user_ptrs, conn_di);
  }
  else
  {
    
  }

  if (cape_list_size (user_ptrs) > 0)
  {
    QBusFrame frame = qbus_frame_new ();
    
    // CH01: replace self->name with self->uuid and add name as module
    qbus_frame_set (frame, QBUS_FRAME_TYPE_OBSVBL_UPD, NULL, qbus_route_name_get (self->route), NULL, qbus_route_uuid_get (self->route));
    
    {
      CapeUdc nodes = qbus_obsvbl_get_observables (self);

      if (nodes)
      {
        // set the payload frame
        qbus_frame_set_udc (frame, QBUS_MTYPE_JSON, &nodes);
      }
    }
    
    // send the frame
    qbus_engines__broadcast (self->engines, frame, user_ptrs);
    
    qbus_frame_del (&frame);
  }
  
  cape_list_del (&user_ptrs);
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

void qbus_obsvbl_emit__broadcast (QBusObsvbl self, const CapeString value_name, CapeMap routings, CapeUdc* p_value)
{
  // fetch the route list
  CapeMap user_ptrs = qbus_route_get__routings (self->route, routings);
  
  if (cape_map_size (user_ptrs) > 0)
  {
    QBusFrame frame = qbus_frame_new ();
        
    // debug
    if (*p_value)
    {
      CapeString h = cape_json_to_s (*p_value);
      
      printf ("EMIT %s\n", h);

      cape_str_del (&h);

    }

    qbus_frame_set__payload (frame, p_value);
    
    {
      CapeMapCursor* cursor = cape_map_cursor_create (user_ptrs, CAPE_DIRECTION_FORW);
      
      while (cape_map_cursor_next (cursor))
      {
        qbus_frame_set (frame, QBUS_FRAME_TYPE_OBSVBL_VALUE, NULL, cape_map_node_key (cursor->node), value_name, qbus_route_uuid_get (self->route));
        
        qbus_engines__send (self->engines, frame, cape_map_node_value (cursor->node));
      }
      
      cape_map_cursor_destroy (&cursor);
    }
    
    qbus_frame_del (&frame);
  }
  
  cape_map_del (&user_ptrs);
}

//-----------------------------------------------------------------------------

void qbus_obsvbl_emit (QBusObsvbl self, const CapeString value_name, CapeUdc* p_value)
{
  CapeString subscriber_name = cape_str_catenate_c (qbus_route_name_get (self->route), '.', value_name);
  
  cape_str_to_lower (subscriber_name);

  CapeMapNode n = cape_map_find (self->nodes, (void*)subscriber_name);
  
  if (n)
  {
    QBusNode node = cape_map_node_value (n);

    qbus_obsvbl_emit__broadcast (self, value_name, node->routings, p_value);
  }
  
  cape_udc_del (p_value);
  cape_str_del (&subscriber_name);
}

//-----------------------------------------------------------------------------

void qbus_obsvbl_value (QBusObsvbl self, const CapeString value_name, CapeUdc* p_value)
{
  if (*p_value)
  {
    CapeString h = cape_json_to_s (*p_value);
    
    printf ("ON VAL: %s <- %s\n", value_name, h);
    
    cape_str_del (&h);
  }
  
  cape_udc_del (p_value);
}

//-----------------------------------------------------------------------------

void qbus_obsvbl_subloads (QBusObsvbl self)
{
  
}

//-----------------------------------------------------------------------------
