#include "qbus_subscriber.h"

// cape includes
#include <stc/cape_map.h>
#include <stc/cape_str.h>
#include <sys/cape_mutex.h>

//-----------------------------------------------------------------------------

struct QBusManifoldSubscriber_s
{
  QBusManifoldMember member;
  
  CapeString uuid;
  CapeString name;
    
}; typedef struct QBusManifoldSubscriber_s* QBusManifoldSubscriber;

//-----------------------------------------------------------------------------

QBusManifoldSubscriber qbus_manifold_subscriber_new (QBusManifoldMember member, const CapeString uuid, const CapeString name)
{
  QBusManifoldSubscriber self = CAPE_NEW (struct QBusManifoldSubscriber_s);
  
  self->member = member;
  
  self->uuid = cape_str_cp (uuid);
  self->name = cape_str_cp (name);
    
  return self;
}

//-----------------------------------------------------------------------------

void qbus_manifold_subscriber_del (QBusManifoldSubscriber* p_self)
{
  if (*p_self)
  {
    QBusManifoldSubscriber self = *p_self;
    
    cape_str_del (&(self->uuid));
    cape_str_del (&(self->name));
    
    CAPE_DEL (p_self, struct QBusManifoldSubscriber_s);
  }
}

//-----------------------------------------------------------------------------

void qbus_manifold_subscriber_emit (QBusManifoldSubscriber self, CapeUdc val, const CapeString module_ident, const CapeString module_name)
{
  if (self->uuid)
  {
    if (cape_str_equal (self->uuid, module_ident))
    {
      qbus_manifold_member_emit (self->member, val, module_ident, module_name);
    }
  }
  else if (self->name)
  {
    if (cape_str_equal (self->name, module_name))
    {
      qbus_manifold_member_emit (self->member, val, module_ident, module_name);
    }
  }
}

//-----------------------------------------------------------------------------
