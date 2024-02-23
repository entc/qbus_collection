#include "qbus_method.h"
 
struct QBusMethod_s
{
     CapeString chainkey;
     
     void* user_ptr;
     fct_qbus_onMessage user_fct;     
};
 
//-----------------------------------------------------------------------------
 
QBusMethod qbus_method_new (const CapeString chainkey, void* user_ptr, fct_qbus_onMessage user_fct)
{
    QBusMethod self = CaPE_NEW (struct QBusMethod_s);
    
    self->chainkey = cape_str_cp (chainkey);
    self->user_ptr = user_ptr;
    self->user_fct = user_fct;
    
    return self;
}

//-----------------------------------------------------------------------------

void qbus_method_del (QBusMethod* p_self)
{
    
}

//-----------------------------------------------------------------------------

