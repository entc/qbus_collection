#include "qbus_route.h"

// cape includes
#include <stc/cape_map.h>
#include <stc/cape_list.h>
#include <sys/cape_mutex.h>

//-----------------------------------------------------------------------------

struct QBusRoute_s
{
  CapeMap routes;

  CapeMutex mutex;
};

//-----------------------------------------------------------------------------

void __STDCALL qbus_route__routes_on_del (void* key, void* val)
{
  {
    CapeString h = key; cape_str_del (&h);
  }
  {
    CapeList h = val; cape_list_del (&h);
  }
}

//-----------------------------------------------------------------------------

QBusRoute qbus_route_new ()
{
  QBusRoute self = CAPE_NEW (struct QBusRoute_s);
  
  self->mutex = cape_mutex_new ();
  self->routes = cape_map_new (cape_map__compare__s, qbus_route__routes_on_del, NULL);
  
  return self;
}

//-----------------------------------------------------------------------------

void qbus_route_del (QBusRoute* p_self)
{
  if (*p_self)
  {
    QBusRoute self = *p_self;
    
    cape_map_del (&(self->routes));
    cape_mutex_del (&(self->mutex));

    CAPE_DEL (p_self, struct QBusRoute_s);
  }
}

//-----------------------------------------------------------------------------

void* qbus_route_get (QBusRoute self, const char* module)
{
  void* ret = NULL;
  
  cape_mutex_lock (self->mutex);
  
  {
    CapeMapNode n = cape_map_find (self->routes, (void*)module);
    if (n)
    {
      // always use the first entry
      CapeListNode h = cape_list_node_front (cape_map_node_value (n));
      if (h)
      {
        ret = cape_list_node_data (h);
        
        // TODO: increase reference
      }
    }
  }
  
  cape_mutex_unlock (self->mutex);
  
  return ret;
}

//-----------------------------------------------------------------------------

void __STDCALL qbus_route__destinations__on_del (void* ptr)
{
  
}

//-----------------------------------------------------------------------------

void qbus_route_add (QBusRoute self, const char* uuid, const char* module, void* node)
{
  cape_mutex_lock (self->mutex);

  {
    CapeList destinations;
    
    CapeMapNode n = cape_map_find (self->routes, (void*)module);
    if (n)
    {
      destinations = cape_map_node_value (n);
    }
    else
    {
      destinations = cape_list_new (qbus_route__destinations__on_del);
      
      cape_map_insert (self->routes, (void*)cape_str_cp (module), (void*)destinations);
    }
  
    // TODO: increase ref for node
  
    cape_list_push_back (destinations, node);
    
  }
  
  cape_mutex_unlock (self->mutex);
}

//-----------------------------------------------------------------------------
