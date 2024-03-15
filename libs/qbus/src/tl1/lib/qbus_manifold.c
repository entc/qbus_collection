#include "qbus_tl1.h" 

struct QBusManifold_s
{
  void* user_ptr;

  fct_qbus_manifold__on_add on_add;
  fct_qbus_manifold__on_rm on_rm;
  fct_qbus_manifold__on_call on_call;
  fct_qbus_manifold__on_emit on_emit;
};

//-----------------------------------------------------------------------------

QBusManifold qbus_manifold_new (void* user_ptr, fct_qbus_manifold__on_add on_add, fct_qbus_manifold__on_rm on_rm, fct_qbus_manifold__on_call on_call, fct_qbus_manifold__on_emit on_emit)
{
    QBusManifold self = CAPE_NEW (struct QBusManifold_s);

    self->user_ptr = user_ptr;

    self->on_call = on_call;
    self->on_emit = on_emit;

    self->on_add = on_add;
    self->on_rm = on_rm;

    return self;
}

//-----------------------------------------------------------------------------

void qbus_manifold_del (QBusManifold* p_self)
{
  if (*p_self)
  {
    QBusManifold self = *p_self;
    
    CAPE_DEL (p_self, struct QBusManifold_s);
  }
}

//-----------------------------------------------------------------------------

int qbus_manifold_init (QBusManifold self, const CapeString uuid, const CapeString name, void* user_ptr, fct_qbus_manifold__on_add on_add, fct_qbus_manifold__on_rm on_rm, fct_qbus_manifold__on_call on_call, fct_qbus_manifold__on_emit on_emit, CapeErr err)
{
    
    
    return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

void qbus_manifold_emit (QBusManifold self)
{
  
}

//-----------------------------------------------------------------------------

void qbus_manifold_subscribe (QBusManifold self)
{
  
}

//-----------------------------------------------------------------------------

void qbus_manifold_response (QBusManifold self)
{
  
}

//-----------------------------------------------------------------------------

int qbus_manifold_send (QBusManifold self, void** p_node, const CapeString method, QBusM msg, QBusMethod* p_qbus_method)
{
  
}

//-----------------------------------------------------------------------------
