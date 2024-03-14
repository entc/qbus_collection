#include "qbus_method.h"
#include "qbus.h"

//-----------------------------------------------------------------------------

struct QBusMethod_s
{
  CapeString chainkey;
  
  void* user_ptr;
  fct_qbus_onMessage user_fct;
};
 
//-----------------------------------------------------------------------------
 
QBusMethod qbus_method_new (const CapeString chainkey, void* user_ptr, fct_qbus_onMessage user_fct)
{
  QBusMethod self = CAPE_NEW (struct QBusMethod_s);
  
  self->chainkey = cape_str_cp (chainkey);
  self->user_ptr = user_ptr;
  self->user_fct = user_fct;
  
  return self;
}

//-----------------------------------------------------------------------------

void qbus_method_del (QBusMethod* p_self)
{
  if (*p_self)
  {
    QBusMethod self = *p_self;
    
    cape_str_del (&(self->chainkey));
    
    CAPE_DEL (p_self, struct QBusMethod_s);
  }
}

//-----------------------------------------------------------------------------

int qbus_method_run (QBusMethod self, QBus qbus, CapeErr err)
{
  if (self->user_fct)
  {
    QBusM qin = qbus_message_new (self->chainkey, NULL);
    QBusM qout = qbus_message_new (NULL, NULL);
    
    self->user_fct (qbus, self->user_ptr, qin, qout, err);
    
    qbus_message_del (&qin);
    qbus_message_del (&qout);
  }
  
  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------
