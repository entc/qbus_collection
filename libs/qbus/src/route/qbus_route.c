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
  
};

//-----------------------------------------------------------------------------

QBusRoute qbus_route_new (const CapeString name)
{
  QBusRoute self = CAPE_NEW (struct QBusRoute_s);

  
  return self;
}

//-----------------------------------------------------------------------------

void qbus_route_del (QBusRoute* p_self)
{
  if (*p_self)
  {
    QBusRoute self = *p_self;
    
    
    CAPE_DEL (p_self, struct QBusRoute_s);
  }
}

//-----------------------------------------------------------------------------

const CapeString qbus_route_uuid_get (QBusRoute self)
{
  
}

//-----------------------------------------------------------------------------

const CapeString qbus_route_name_get (QBusRoute self)
{
  
}

//-----------------------------------------------------------------------------

CapeUdc qbus_route_node_get (QBusRoute self)
{
  
}

//-----------------------------------------------------------------------------
