#include "qbus_methods.h"

// cape includes
#include <stc/cape_str.h>
#include <stc/cape_map.h>
#include <sys/cape_log.h>

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

void qbus_methods_add (QBusMethods self)
{
  
}

//-----------------------------------------------------------------------------
