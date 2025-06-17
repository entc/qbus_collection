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

int qbus_methods_add (QBusMethods self, const CapeString method, void* user_ptr, fct_qbus_on_msg on_msg, fct_qbus_on_rm on_rm, CapeErr err)
{
  
  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

int qbus_methods_run (QBusMethods self, const CapeString method, CapeErr err)
{
  return cape_err_set_fmt (err, CAPE_ERR_NOT_FOUND, "method [%s] not found", method);
}

//-----------------------------------------------------------------------------
