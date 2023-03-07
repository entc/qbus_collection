#include "qbus_obsvbl.h"

//-----------------------------------------------------------------------------

struct QBusObsvbl_s
{
  QBusEngines engines;        // reference
  
  
};

//-----------------------------------------------------------------------------

QBusObsvbl qbus_obsvbl_new (QBusEngines engines)
{
  QBusObsvbl self = CAPE_NEW (struct QBusObsvbl_s);
  
  self->engines = engines;
  
  return self;
}

//-----------------------------------------------------------------------------

void qbus_obsvbl_del (QBusObsvbl* p_self)
{
  if (*p_self)
  {
    
    
    CAPE_DEL(p_self, struct QBusObsvbl_s);    
  }
}

//-----------------------------------------------------------------------------
