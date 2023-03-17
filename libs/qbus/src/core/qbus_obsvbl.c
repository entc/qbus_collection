#include "qbus_obsvbl.h"
#include "qbus_engines.h"

// cape includes
#include <stc/cape_map.h>
#include <fmt/cape_json.h>
#include <sys/cape_log.h>

//-----------------------------------------------------------------------------

struct QBusSubscriber_s
{
  void* user_ptr;  
  fct_qbus_on_emit user_fct;
  
  QBusObsvbl obsvbl;    // reference
  CapeString name;
};

//-----------------------------------------------------------------------------

QBusSubscriber qbus_subscriber_new (QBusObsvbl obsvbl, const CapeString name, void* user_ptr, fct_qbus_on_emit user_fct)
{
  QBusSubscriber self = CAPE_NEW(struct QBusSubscriber_s);
  
  self->obsvbl = obsvbl;
  self->name = cape_str_cp (name);
  
  self->user_ptr = user_ptr;
  self->user_fct = user_fct;
  
  return self;
}

//-----------------------------------------------------------------------------

void qbus_subscriber_del (QBusSubscriber* p_self)
{
  if (*p_self)
  {
    QBusSubscriber self = *p_self;
    
    cape_str_del (&(self->name));
    
    CAPE_DEL (p_self, struct QBusSubscriber_s);
  }
}

//-----------------------------------------------------------------------------

void qbus_subscriber__dump (QBusSubscriber self)
{
  printf ("%10s |   | %36s | %s\n", "SUBSCRIBE", "", self->name);
}

//-----------------------------------------------------------------------------

struct QBusObsvblEmitter_s
{
  CapeMap modules_uuids;
  int local;
  
}; typedef struct QBusObsvblEmitter_s* QBusObsvblEmitter;

//-----------------------------------------------------------------------------

void __STDCALL qbus_obsvbl__modules_uuids__on_del (void* key, void* val)
{
  {
    CapeString h = key; cape_str_del (&h);
  }
  {
    // reference
  }
}

//-----------------------------------------------------------------------------

QBusObsvblEmitter qbus_emitter_new (int is_local)
{
  QBusObsvblEmitter self = CAPE_NEW (struct QBusObsvblEmitter_s);

  self->modules_uuids = cape_map_new (NULL, qbus_obsvbl__modules_uuids__on_del, NULL);
  self->local = is_local;
  
  return self;
}

//-----------------------------------------------------------------------------

void qbus_emitter_del (QBusObsvblEmitter* p_self)
{
  if (*p_self)
  {
    QBusObsvblEmitter self = *p_self;
    
    cape_map_del (&(self->modules_uuids));
    
    CAPE_DEL (p_self, struct QBusObsvblEmitter_s);
  }
}

//-----------------------------------------------------------------------------

number_t qbus_emitter_add (QBusObsvblEmitter self, QBusRouteNameItem name_item)
{
  number_t ret = 0;
  
  const CapeString uuid = qbus_route_name_uuid_get (name_item);
  
  if (uuid)
  {
    CapeMapNode n = cape_map_find (self->modules_uuids, (void*)uuid);
    if (n == NULL)
    {
      cape_map_insert (self->modules_uuids, (void*)cape_str_cp(uuid), (void*)name_item);  
      ret++;
    }
  }
  
  return ret;
}

//-----------------------------------------------------------------------------

void qbus_emitter_rm (QBusObsvblEmitter self, QBusRouteNameItem name_item)
{
  if (self->modules_uuids)
  {
    CapeMapCursor* cursor = cape_map_cursor_create (self->modules_uuids, CAPE_DIRECTION_FORW);
    
    while (cape_map_cursor_next (cursor))
    {
      if (cape_map_node_value (cursor->node) == name_item)
      {
        cape_map_cursor_erase (self->modules_uuids, cursor);        
      }
    }
    
    cape_map_cursor_destroy (&cursor);
  }
}

//-----------------------------------------------------------------------------

void qbus_emitter_dump (QBusObsvblEmitter self, const CapeString subscriber_name)
{
  if (self->modules_uuids)
  {
    CapeMapCursor* cursor = cape_map_cursor_create (self->modules_uuids, CAPE_DIRECTION_FORW);
    
    while (cape_map_cursor_next (cursor))
    {
      qbus_route_name_dump2 (cape_map_node_value (cursor->node), "EMITTER", self->local, subscriber_name);
    }
    
    cape_map_cursor_destroy (&cursor);
  }
}

//-----------------------------------------------------------------------------

void qbus_emitter_emit (QBusObsvblEmitter self, QBusRoute route, QBusEngines engines, const CapeString subscriber_name, CapeUdc* p_value)
{
  QBusFrame frame = qbus_frame_new ();
  
  // assign payload to frame
  qbus_frame_set__payload (frame, p_value);
  
  {
    CapeMapCursor* cursor = cape_map_cursor_create (self->modules_uuids, CAPE_DIRECTION_FORW);
    
    while (cape_map_cursor_next (cursor))
    {
      QBusRouteNameItem name_item = cape_map_node_value (cursor->node);
      
      // override the frame credentials
      // -> use the chainkey as name to identify the module of the value
      // -> use the module as destination uuid
      // -> use the method for the value name
      qbus_frame_set (frame, QBUS_FRAME_TYPE_OBSVBL_VALUE, qbus_route_name_get (route), cape_map_node_key (cursor->node), subscriber_name, qbus_route_uuid_get (route));
      
      // send the frame to the conn
      qbus_engines__send (engines, frame, qbus_route_name__get_conn (name_item));
    }
    
    cape_map_cursor_destroy (&cursor);
  }
  
  qbus_frame_del (&frame);
}

//-----------------------------------------------------------------------------

struct QBusObsvbl_s
{
  QBusEngines engines;        // reference
  QBusRoute route;            // reference
  
  CapeMap observables;        // map to store local observables for receiving updates
  CapeMap emitters;           // map of emitters
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

void __STDCALL qbus_obsvbl__emitters__on_del (void* key, void* val)
{
  {
    CapeString h = key; cape_str_del (&h);
  }
  {
    QBusObsvblEmitter h = val; qbus_emitter_del (&h);
  }
}

//-----------------------------------------------------------------------------

QBusObsvbl qbus_obsvbl_new (QBusEngines engines, QBusRoute route)
{
  QBusObsvbl self = CAPE_NEW (struct QBusObsvbl_s);
  
  self->engines = engines;
  self->route = route;
  
  self->observables = cape_map_new (NULL, qbus_obsvbl__observables__on_del, NULL);
  self->emitters = cape_map_new (NULL, qbus_obsvbl__emitters__on_del, NULL);
  
  
  
  return self;
}

//-----------------------------------------------------------------------------

void qbus_obsvbl_del (QBusObsvbl* p_self)
{
  if (*p_self)
  {
    QBusObsvbl self = *p_self;
    
    cape_map_del (&(self->emitters));
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

  // emitters to other modules
  {
    CapeMapCursor* cursor = cape_map_cursor_create (self->emitters, CAPE_DIRECTION_FORW);
    
    while (cape_map_cursor_next (cursor))
    {
      QBusObsvblEmitter emitter = cape_map_node_value (cursor->node);
     
      // only transmit remote emitters
      if (emitter->local == FALSE)
      {
        CapeMapNode n = cape_map_find (emitter->modules_uuids, (void*)module_uuid);
        if (n)
        {
          cape_udc_add_s_cp (nodes, NULL, (CapeString)cape_map_node_key (cursor->node));        
        }        
      }
    }
    
    cape_map_cursor_destroy (&cursor);
  }

  return nodes;
}

//-----------------------------------------------------------------------------

number_t qbus_obsvbl_set__node (QBusObsvbl self, const CapeString subscriber_name, QBusRouteNameItem name_item)
{
  QBusObsvblEmitter emitter;
  
  int is_local = TRUE;
  
  // check for collisions
  {
    CapeMapNode n = cape_map_find (self->observables, (void*)subscriber_name);
    if (n)
    {
      is_local = FALSE;
    }
  }

  {
    CapeMapNode n = cape_map_find (self->emitters, (void*)subscriber_name);
    if (n)
    {
      emitter = cape_map_node_value (n);
    }
    else
    {
      emitter = qbus_emitter_new (is_local);
      
      cape_map_insert (self->emitters, (void*)cape_str_cp (subscriber_name), (void*)emitter);
    }
  }
    
  return qbus_emitter_add (emitter, name_item);
}

//-----------------------------------------------------------------------------

number_t qbus_obsvbl_set (QBusObsvbl self, CapeUdc observables, QBusRouteNameItem name_item)
{
  number_t ret = 0;
  
  if (observables && name_item)
  {
    CapeUdcCursor* cursor = cape_udc_cursor_new (observables, CAPE_DIRECTION_FORW);
        
    while (cape_udc_cursor_next (cursor))
    {
      switch (cape_udc_type (cursor->item))
      {
        case CAPE_UDC_STRING:
        {
          const CapeString subscriber_name = cape_udc_s (cursor->item, NULL);
          
          if (cape_str_not_empty (subscriber_name))
          {
            ret = ret + qbus_obsvbl_set__node (self, subscriber_name, name_item);
          }
          
          break;
        }
      }
    }
    
    cape_udc_cursor_del (&cursor);
  }
  
  return ret;
}

//-----------------------------------------------------------------------------

void qbus_obsvbl_rm (QBusObsvbl self, QBusRouteNameItem name_item)
{
  if (name_item)
  {
    CapeMapCursor* cursor = cape_map_cursor_create (self->emitters, CAPE_DIRECTION_FORW);
        
    while (cape_map_cursor_next (cursor))
    {
      qbus_emitter_rm (cape_map_node_value (cursor->node), name_item);
    }
    
    cape_map_cursor_destroy (&cursor);
  }
}

//-----------------------------------------------------------------------------

QBusSubscriber qbus_obsvbl_subscribe (QBusObsvbl self, const CapeString module_name, const CapeString value_name, void* user_ptr, fct_qbus_on_emit user_fct)
{
  QBusSubscriber ret = NULL;
  
  CapeString subscriber_name = cape_str_fmt ("#%s.%s", module_name, value_name);
  
  cape_str_to_lower (subscriber_name);
  
  {
    CapeMapNode n = cape_map_find (self->observables, (void*)subscriber_name);
    
    if (n == NULL)
    {
      ret = qbus_subscriber_new (self, subscriber_name, user_ptr, user_fct);
      
      cape_log_fmt (CAPE_LL_TRACE, "QBUS", "obsvbl", "subscribe to value = %s", subscriber_name);
      
      cape_map_insert (self->observables, (void*)cape_str_mv (&subscriber_name), (void*)ret);
    }
  }
  
  cape_str_del (&subscriber_name);
  
  return ret;
}

//-----------------------------------------------------------------------------

QBusSubscriber qbus_obsvbl_subscribe_uuid (QBusObsvbl self, const CapeString uuid, const CapeString value_name, void* user_ptr, fct_qbus_on_emit user_fct)
{
  QBusSubscriber ret = NULL;
  
  CapeString subscriber_name = cape_str_fmt ("%s.%s", uuid, value_name);
  
  cape_str_to_lower (subscriber_name);
  
  {
    CapeMapNode n = cape_map_find (self->observables, (void*)subscriber_name);
    
    if (n == NULL)
    {
      ret = qbus_subscriber_new (self, subscriber_name, user_ptr, user_fct);
      
      cape_log_fmt (CAPE_LL_TRACE, "QBUS", "obsvbl", "subscribe to value = %s", subscriber_name);
      
      cape_map_insert (self->observables, (void*)cape_str_mv (&subscriber_name), (void*)ret);
    }
  }
  
  cape_str_del (&subscriber_name);
  
  return ret;
}

//-----------------------------------------------------------------------------

void qbus_obsvbl_unsubscribe (QBusSubscriber* p_subscriber)
{
  QBusSubscriber subscriber = *p_subscriber;
  
  if (subscriber)
  {
    QBusObsvbl self = subscriber->obsvbl;
    
    {
      CapeMapNode n = cape_map_find (self->emitters, (void*)subscriber->name);
      
      if (n)
      {
        cape_map_erase (self->emitters, n);      
      }
    }
    {
      CapeMapNode n = cape_map_find (self->observables, (void*)subscriber->name);
      
      if (n)
      {
        cape_map_erase (self->observables, n);      
      }
    }
  }
}

//-----------------------------------------------------------------------------

void qbus_obsvbl_emit__ptrs (QBusObsvbl self, const CapeString value_name, CapeMap routings, CapeUdc* p_value)
{
/*
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
  */
}

//-----------------------------------------------------------------------------

void qbus_obsvbl_emit (QBusObsvbl self, const CapeString prefix, const CapeString value_name, CapeUdc* p_value)
{
  CapeString subscriber_name = cape_str_catenate_c (prefix, '.', value_name);
  
  cape_str_to_lower (subscriber_name);

  CapeMapNode n = cape_map_find (self->emitters, (void*)subscriber_name);
  if (n)
  {
    qbus_emitter_emit (cape_map_node_value (n), self->route, self->engines, subscriber_name, p_value);    
  }
  
  cape_udc_del (p_value);
  cape_str_del (&subscriber_name);
}

//-----------------------------------------------------------------------------

void qbus_obsvbl_value (QBusObsvbl self, const CapeString module_name, const CapeString subscriber_name, CapeUdc* p_value)
{
  printf ("seek subscriber: %s\n", subscriber_name);
  
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

  cape_udc_del (p_value);
}

//-----------------------------------------------------------------------------

void qbus_obsvbl_dump (QBusObsvbl self)
{
  {
    CapeMapCursor* cursor = cape_map_cursor_create (self->observables, CAPE_DIRECTION_FORW);
    
    while (cape_map_cursor_next (cursor))
    {
      qbus_subscriber__dump (cape_map_node_value (cursor->node));
    }
    
    cape_map_cursor_destroy (&cursor);
  }
  {
    CapeMapCursor* cursor = cape_map_cursor_create (self->emitters, CAPE_DIRECTION_FORW);
    
    while (cape_map_cursor_next (cursor))
    {
      qbus_emitter_dump (cape_map_node_value (cursor->node), cape_map_node_key (cursor->node));
    }
    
    cape_map_cursor_destroy (&cursor);
  }
}

//-----------------------------------------------------------------------------
