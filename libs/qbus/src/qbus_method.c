#include "qbus_method.h"
#include "qbus.h"

//-----------------------------------------------------------------------------

struct QBusMethod_s
{
  CapeString module_ident;
  CapeString method_ident;
  
  void* user_ptr;
  fct_qbus_onMessage user_fct;
};
 
//-----------------------------------------------------------------------------
 
QBusMethod qbus_method_new (void* user_ptr, fct_qbus_onMessage user_fct, const CapeString module_ident, const CapeString method_ident)
{
  QBusMethod self = CAPE_NEW (struct QBusMethod_s);
  
  self->user_ptr = user_ptr;
  self->user_fct = user_fct;
  
  self->module_ident = cape_str_cp (module_ident);
  self->method_ident = cape_str_cp (method_ident);
  
  return self;
}

//-----------------------------------------------------------------------------

void qbus_method_del (QBusMethod* p_self)
{
  if (*p_self)
  {
    QBusMethod self = *p_self;
    
    cape_str_del (&(self->module_ident));
    cape_str_del (&(self->method_ident));
    
    CAPE_DEL (p_self, struct QBusMethod_s);
  }
}

//-----------------------------------------------------------------------------

int qbus_method_run (QBusMethod self, QBus qbus, QBusM qin, QBusM qout, CapeErr err)
{
  if (self->user_fct)
  {
    self->user_fct (qbus, self->user_ptr, qin, qout, err);
  }
  
  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

void qbus_method_idents (QBusMethod self, CapeString* p_module_ident, CapeString* p_method_ident)
{
  cape_str_replace_cp (p_module_ident, self->module_ident);
  cape_str_replace_cp (p_method_ident, self->method_ident);
}

//-----------------------------------------------------------------------------
