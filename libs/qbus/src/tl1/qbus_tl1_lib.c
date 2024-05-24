#include "qbus_chains.h"
#include "qbus_tl1.h"
#include "qbus_member.h"
#include "qbus_tl2.h"

// cape includes
#include <stc/cape_map.h>
#include <sys/cape_mutex.h>

//-----------------------------------------------------------------------------

struct QBusManifold_s
{
  CapeMap members;
  QBusChains chains;

  QBusEngine engine;
};

//-----------------------------------------------------------------------------

void __STDCALL qbus_manifold__members__on_del (void* key, void* val)
{
  {
    CapeString h = key; cape_str_del (&h);
  }
  {
    QBusManifoldMember h = val; qbus_manifold_member_del (&h);
  }
}

//-----------------------------------------------------------------------------

QBusManifold qbus_manifold_new ()
{
  QBusManifold self = CAPE_NEW (struct QBusManifold_s);

  self->members = cape_map_new (cape_map__compare__s, qbus_manifold__members__on_del, NULL);

  self->chains = qbus_chains_new ();
  self->engine = qbus_engine_new ();
    
  return self;
}

//-----------------------------------------------------------------------------

void qbus_manifold_del (QBusManifold* p_self)
{
  if (*p_self)
  {
    QBusManifold self = *p_self;
    
    qbus_engine_del (&(self->engine));
    qbus_chains_del (&(self->chains));

    cape_map_del (&(self->members));

    CAPE_DEL (p_self, struct QBusManifold_s);
  }
}

//-----------------------------------------------------------------------------

void __STDCALL qbus_manifold__on_connect (void* user_ptr, void* node)
{
  QBusManifoldMember member = user_ptr;

  
}

//-----------------------------------------------------------------------------

void __STDCALL qbus_manifold__on_disconnect (void* user_ptr)
{
  QBusManifoldMember member = user_ptr;

  
}

//-----------------------------------------------------------------------------

int qbus_manifold_init (QBusManifold self, const CapeString uuid, const CapeString name, void* user_ptr, fct_qbus_manifold__on_add on_add, fct_qbus_manifold__on_rm on_rm, fct_qbus_manifold__on_call on_call, fct_qbus_manifold__on_recv on_recv, fct_qbus_manifold__on_emit on_emit, CapeErr err)
{
  int res;
  
  QBusManifoldMember member;
  
  CapeMapNode n = cape_map_find (self->members, (void*)uuid);
  
  if (n)
  {
    return cape_err_set (err, CAPE_ERR_EOF, "already set");
  }

  member = qbus_manifold_member_new (name, user_ptr, on_add, on_rm, on_call, on_recv, on_emit);
  
  n = cape_map_insert (self->members, (void*)cape_str_cp (uuid), member);

  res = qbus_engine_init (self->engine, member, qbus_manifold__on_connect, qbus_manifold__on_disconnect, err);

  
  
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
