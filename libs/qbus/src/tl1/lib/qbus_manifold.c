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
  
}

//-----------------------------------------------------------------------------

void qbus_manifold_del (QBusManifold* p_self)
{
  
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

int qbus_manifold_send (QBusManifold self, void** p_node)
{
  
}

//-----------------------------------------------------------------------------
