#include "qbus_chains.h"

// cape includes
#include <stc/cape_map.h>
#include <sys/cape_mutex.h>

//-----------------------------------------------------------------------------

struct QBusChains_s
{
  CapeMap chains;
  CapeMutex chains_mutex;

};

//-----------------------------------------------------------------------------

void __STDCALL qbus_chains__chains__on_del (void* key, void* val)
{
  {
    CapeString h = key; cape_str_del (&h);
  }
  {
    QBusMethod method = val; qbus_method_del (&method);
  }
}

//-----------------------------------------------------------------------------

QBusChains qbus_chains_new ()
{
  QBusChains self = CAPE_NEW (struct QBusChains_s);
  
  self->chains_mutex = cape_mutex_new ();
  self->chains = cape_map_new (cape_map__compare__s, qbus_chains__chains__on_del, NULL);

  return self;
}

//-----------------------------------------------------------------------------

void qbus_chains_del (QBusChains* p_self)
{
  if (*p_self)
  {
    QBusChains self = *p_self;
    
    cape_mutex_del (&(self->chains_mutex));
    cape_map_del (&(self->chains));
    
    CAPE_DEL (p_self, struct QBusChains_s);
  }
}

//-----------------------------------------------------------------------------

void qbus_chains_push (QBusChains self, const CapeString method_ident, QBusMethod* p_method)
{
  cape_mutex_lock (self->chains_mutex);

  cape_map_insert (self->chains, (void*)cape_str_cp (method_ident), (void*)*p_method);
  *p_method = NULL;

  cape_mutex_unlock (self->chains_mutex);
}

//-----------------------------------------------------------------------------

QBusMethod qbus_chains_pop (QBusChains self, const CapeString method_ident)
{
  QBusMethod ret = NULL;
  
  cape_mutex_lock (self->chains_mutex);
  
  CapeMapNode n2 = cape_map_find (self->chains, (void*)method_ident);
  if (n2)
  {
    ret = cape_map_node_value (n2);
    
    cape_map_node_set (n2, (void*)NULL);
    
    cape_map_erase (self->chains, n2);
  }

  cape_mutex_unlock (self->chains_mutex);
  
  return ret;
}

//-----------------------------------------------------------------------------
