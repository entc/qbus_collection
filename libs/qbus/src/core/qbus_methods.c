#include "qbus_methods.h"

// cape includes
#include <sys/cape_log.h>
#include <sys/cape_queue.h>
#include <fmt/cape_json.h>

//-----------------------------------------------------------------------------

struct QBusMethods_s
{
  
};

//-----------------------------------------------------------------------------

QBusMethods qbus_methods_new ()
{
  QBusMethods self = CAPE_NEW (struct QBusMethods_s);
  
  
  return self;
}

//-----------------------------------------------------------------------------

void qbus_methods_del (QBusMethods* p_self)
{
  if (*p_self)
  {
    QBusMethods self = *p_self;
    
    
    CAPE_DEL (p_self, struct QBusMethods_s);
  }
}

//-----------------------------------------------------------------------------
