#include "qbus_route.h"

// cape includes
#include <sys/cape_file.h>
#include <sys/cape_log.h>
#include <sys/cape_thread.h>
#include <fmt/cape_args.h>
#include <fmt/cape_json.h>
#include <fmt/cape_tokenizer.h>

//-----------------------------------------------------------------------------

struct QBusRoute_s
{
  CapeString uuid;            // the unique id of this module
  CapeString name;            // the name of the module (multiple modules might have the same name)
};

//-----------------------------------------------------------------------------

QBusRoute qbus_route_new (const CapeString name)
{
  QBusRoute self = CAPE_NEW (struct QBusRoute_s);

  self->uuid = cape_str_uuid ();
  self->name = cape_str_cp (name);
  
  return self;
}

//-----------------------------------------------------------------------------

void qbus_route_del (QBusRoute* p_self)
{
  if (*p_self)
  {
    QBusRoute self = *p_self;
    
    cape_str_del (&(self->name));
    cape_str_del (&(self->uuid));
    
    CAPE_DEL (p_self, struct QBusRoute_s);
  }
}

//-----------------------------------------------------------------------------

const CapeString qbus_route_uuid_get (QBusRoute self)
{
  return self->uuid;
}

//-----------------------------------------------------------------------------

const CapeString qbus_route_name_get (QBusRoute self)
{
  return self->name;
}

//-----------------------------------------------------------------------------

CapeUdc qbus_route_node_get (QBusRoute self)
{
  return NULL;
}

//-----------------------------------------------------------------------------
