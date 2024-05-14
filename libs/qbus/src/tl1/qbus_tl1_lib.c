#include "qbus_chains.h"
#include "qbus_tl1.h"
#include "qbus_member.h"

//-----------------------------------------------------------------------------

struct QBusManifold_s
{
  QBusChains chains;

  
};

//-----------------------------------------------------------------------------

QBusManifold qbus_manifold_new ()
{
  QBusManifold self = CAPE_NEW (struct QBusManifold_s);

  self->chains = qbus_chains_new ();

  return self;
}

//-----------------------------------------------------------------------------

void qbus_manifold_del (QBusManifold* p_self)
{
  if (*p_self)
  {
    QBusManifold self = *p_self;
    
    qbus_chains_del (&(self->chains));

    CAPE_DEL (p_self, struct QBusManifold_s);
  }
}

//-----------------------------------------------------------------------------

int qbus_manifold_init (QBusManifold self, const CapeString uuid, const CapeString name, void* user_ptr, fct_qbus_manifold__on_add on_add, fct_qbus_manifold__on_rm on_rm, fct_qbus_manifold__on_call on_call, fct_qbus_manifold__on_recv on_recv, fct_qbus_manifold__on_emit on_emit, CapeErr err)
{
    
    
    return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

void qbus_manifold_emit (QBusManifold self, const CapeString uuid, CapeUdc val)
{
  
}

//-----------------------------------------------------------------------------

void qbus_manifold_subscribe (QBusManifold self, const CapeString uuid, const CapeString module_uuid, const CapeString module_name, const CapeString value_name)
{
  
}

//-----------------------------------------------------------------------------

void qbus_manifold_response (QBusManifold self, QBusM msg)
{
  
}

//-----------------------------------------------------------------------------

int qbus_manifold_send (QBusManifold self, const CapeString module_ident, void** p_node, const CapeString method, QBusM msg, QBusMethod* p_qbus_method, CapeErr err)
{
  
  cape_str_del (&(msg->method_ident));
  msg->method_ident = cape_str_uuid ();

  cape_str_del (&(msg->module_ident));
  msg->module_ident = cape_str_cp (module_ident);
  
  qbus_chains_push (self->chains, msg->method_ident, p_qbus_method);
  
  
  
}

//-----------------------------------------------------------------------------
