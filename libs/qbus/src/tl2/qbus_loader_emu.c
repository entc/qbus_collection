#include "qbus_tl2.h"

//-----------------------------------------------------------------------------

struct QBusEngine_s
{
  
};

//-----------------------------------------------------------------------------

QBusEngine qbus_engine_new ()
{
  QBusEngine self = CAPE_NEW (struct QBusEngine_s);

    
  return self;
}

//-----------------------------------------------------------------------------

void qbus_engine_del (QBusEngine* p_self)
{
  if (*p_self)
  {
    QBusEngine self = *p_self;
    

    CAPE_DEL (p_self, struct QBusEngine_s);
  }
}

//-----------------------------------------------------------------------------

int qbus_engine_init (QBusEngine self, void* user_ptr, fct_qbus_engine__on_connect on_connect, fct_qbus_engine__on_disconnect on_disconnect, CapeErr err)
{
  
}

//-----------------------------------------------------------------------------
