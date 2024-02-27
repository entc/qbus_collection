#include "qbus_storage.h"

// cape includes
#include <stc/cape_map.h>
#include <sys/cape_mutex.h>

//-----------------------------------------------------------------------------

struct QBusStorage_s
{
  CapeMap methods;
  CapeMutex mutex;  
};

//-----------------------------------------------------------------------------

void __STDCALL qbus_storage__methods_on_del (void* key, void* val)
{
  {
    CapeString h = key; cape_str_del (&h);
  }
  {
    QBusMethod h = val; qbus_method_del (&h);
  }
}

//-----------------------------------------------------------------------------

QBusStorage qbus_storage_new ()
{
  QBusStorage self = CAPE_NEW (struct QBusStorage_s);
  
  self->mutex = cape_mutex_new ();
  self->methods = cape_map_new (cape_map__compare__s, qbus_storage__methods_on_del, NULL);

  return self;
}

//-----------------------------------------------------------------------------

void qbus_storage_del (QBusStorage* p_self)
{
  if (*p_self)
  {
    QBusStorage self = *p_self;
    
    cape_map_del (&(self->methods));
    cape_mutex_del (&(self->mutex));
    
    CAPE_DEL (p_self, struct QBusStorage_s);
  }
}

//-----------------------------------------------------------------------------

int qbus_storage_add (QBusStorage self, const CapeString method, void* user_ptr, fct_qbus_onMessage on_msg, CapeErr err)
{
  int res;
  
  cape_mutex_lock (self->mutex);

  {
    CapeMapNode n = cape_map_find (self->methods, (void*)method);
    if (n)
    {
      res = cape_err_set (err, CAPE_ERR_OUT_OF_BOUNDS, "ERR.ALREADY_ADDED");
    }
    else
    {
      QBusMethod method_holder = qbus_method_new (NULL, user_ptr, on_msg);
      
      cape_map_insert (self->methods, (void*)cape_str_cp (method), (void*)method_holder);
      
      res = CAPE_ERR_NONE;
    }
  }
  
  cape_mutex_unlock (self->mutex);
  
  return res;
}

//-----------------------------------------------------------------------------

QBusMethod qbus_storage_get (QBusStorage self, const CapeString method)
{
  QBusMethod ret = NULL;
  
  cape_mutex_lock (self->mutex);

  {
    CapeMapNode n = cape_map_find (self->methods, (void*)method);
    if (n)
    {
      ret = cape_map_node_value (n);
    }
  }
  
  cape_mutex_unlock (self->mutex);

  return ret;
}

//-----------------------------------------------------------------------------

