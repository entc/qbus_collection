#include "qbus_member.h"

// cape includes
#include <stc/cape_map.h>
#include <stc/cape_str.h>
#include <sys/cape_mutex.h>

//-----------------------------------------------------------------------------

struct QBusManifoldMember_s
{
  CapeString name;
  
  void* user_ptr;
  
  fct_qbus_manifold__on_add on_add;
  fct_qbus_manifold__on_rm on_rm;
  fct_qbus_manifold__on_call on_call;
  fct_qbus_manifold__on_recv on_recv;
  fct_qbus_manifold__on_emit on_emit;
  
};

//-----------------------------------------------------------------------------

QBusManifoldMember qbus_manifold_member_new (const CapeString name, void* user_ptr, fct_qbus_manifold__on_add on_add, fct_qbus_manifold__on_rm on_rm, fct_qbus_manifold__on_call on_call,fct_qbus_manifold__on_recv on_recv, fct_qbus_manifold__on_emit on_emit)
{
  QBusManifoldMember self = CAPE_NEW (struct QBusManifoldMember_s);
  
  self->name = cape_str_cp (name);
  
  self->user_ptr = user_ptr;
  
  self->on_call = on_call;
  self->on_emit = on_emit;
  
  self->on_add = on_add;
  self->on_rm = on_rm;
  
  return self;
}

//-----------------------------------------------------------------------------

void qbus_manifold_member_del (QBusManifoldMember* p_self)
{
  if (*p_self)
  {
    QBusManifoldMember self = *p_self;
    
    cape_str_del (&(self->name));
    
    CAPE_DEL (p_self, struct QBusManifoldMember_s);
  }
}

//-----------------------------------------------------------------------------

const CapeString qbus_manifold_member_name (QBusManifoldMember self)
{
  return self->name;
}

//-----------------------------------------------------------------------------

void qbus_manifold_member_add (QBusManifoldMember self, const char* uuid, const char* module, void* node)
{
  if (self->on_add)
  {
    self->on_add (self->user_ptr, uuid, module, node);
  }
}

//-----------------------------------------------------------------------------

void qbus_manifold_member_set (QBusManifoldMember self, void* node)
{
  
}

//-----------------------------------------------------------------------------

int qbus_manifold_member_call (QBusManifoldMember self, const CapeString method_name, QBusMethod* p_qbus_method, QBusM msg, CapeErr err)
{
  if (self->on_call)
  {
    self->on_call (self->user_ptr, method_name, p_qbus_method, msg);
  }
  
  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

void qbus_manifold_member_emit (QBusManifoldMember self, CapeUdc val, const CapeString uuid, const CapeString name)
{
  if (self->on_emit)
  {
    self->on_emit (self->user_ptr, val, uuid, name);
  }
}

//-----------------------------------------------------------------------------
