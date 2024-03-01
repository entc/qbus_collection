#include "qbus_tl1.h" 

// cape includes
#include <stc/cape_map.h>

//-----------------------------------------------------------------------------

struct QBusManifoldMember_s
{
  void* user_ptr;
  
  fct_qbus_manifold__on_add on_add;
  fct_qbus_manifold__on_rm on_rm;
  fct_qbus_manifold__on_call on_call;
  fct_qbus_manifold__on_emit on_emit;
  
}; typedef struct QBusManifoldMember_s* QBusManifoldMember;

//-----------------------------------------------------------------------------

QBusManifoldMember qbus_manifold_member_new (void* user_ptr, fct_qbus_manifold__on_add on_add, fct_qbus_manifold__on_rm on_rm, fct_qbus_manifold__on_call on_call, fct_qbus_manifold__on_emit on_emit)
{
  QBusManifoldMember self = CAPE_NEW (struct QBusManifoldMember_s);
  
  self->user_ptr = NULL;
  
  self->on_call = NULL;
  self->on_emit = NULL;
  
  self->on_add = NULL;
  self->on_rm = NULL;
  
  return self;
}

//-----------------------------------------------------------------------------

void qbus_manifold_member_del (QBusManifoldMember* p_self)
{
  if (*p_self)
  {
    
    CAPE_DEL (p_self, struct QBusManifoldMember_s);
  }
}

//-----------------------------------------------------------------------------

struct QBusManifold_s
{
  CapeMap members;
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
  
  return self;
}

//-----------------------------------------------------------------------------

void qbus_manifold_del (QBusManifold* p_self)
{
  if (*p_self)
  {
    QBusManifold self = *p_self;
    
    cape_map_del (&(self->members));
    
    CAPE_DEL (p_self, struct QBusManifold_s);
  }
}

//-----------------------------------------------------------------------------

int qbus_manifold_init (QBusManifold self, const CapeString uuid, const CapeString name, void* user_ptr, fct_qbus_manifold__on_add on_add, fct_qbus_manifold__on_rm on_rm, fct_qbus_manifold__on_call on_call, fct_qbus_manifold__on_emit on_emit, CapeErr err)
{
  printf ("manifold init uuid = %s\n", uuid);
  
  CapeMapNode n = cape_map_find (self->members, (void*)uuid);
  
  if (n)
  {
    return cape_err_set (err, CAPE_ERR_EOF, "already set");
  }

  cape_map_insert (self->members, (void*)cape_str_cp (uuid), qbus_manifold_member_new (user_ptr, on_add, on_rm, on_call, on_emit));

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

int qbus_manifold_send (QBusManifold self, void** p_node)
{
  //self->on_call (self->user_ptr);
}

//-----------------------------------------------------------------------------
