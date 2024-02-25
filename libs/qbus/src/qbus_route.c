#include "qbus_route.h"

//-----------------------------------------------------------------------------

struct QBusRoute_s
{
  
  
  
  
};

//-----------------------------------------------------------------------------

QBusRoute qbus_route_new ()
{
  QBusRoute self = CAPE_NEW (struct QBusRoute_s);
  
  
  return self;
}

//-----------------------------------------------------------------------------

void qbus_route_del (QBusRoute* p_self)
{
  if (*p_self)
  {
    
    
    CAPE_DEL (p_self, struct QBusRoute_s);
  }
}

//-----------------------------------------------------------------------------
