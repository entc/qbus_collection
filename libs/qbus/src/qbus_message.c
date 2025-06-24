#include "qbus_message.h"

// cape includes
#include <sys/cape_log.h>

//-----------------------------------------------------------------------------

QBusM qbus_message_new (const CapeString key, const CapeString sender)
{
  QBusM self = CAPE_NEW (struct QBusMessage_s);
  
  self->chain_key = cape_str_cp (key);
  self->sender = cape_str_cp (sender);
  
  // init the objects
  self->cdata = NULL;
  self->pdata = NULL;
  self->clist = NULL;
  self->rinfo = NULL;
  self->files = NULL;
  self->blob = NULL;
  
  self->err = NULL;
  
  self->mtype = QBUS_MTYPE_NONE;
    
  return self;
}

//-----------------------------------------------------------------------------

void qbus_message_clr (QBusM self, u_t cdata_udc_type)
{
  cape_udc_del (&(self->cdata));
  cape_udc_del (&(self->pdata));
  cape_udc_del (&(self->clist));
  cape_stream_del (&(self->blob));
  
  cape_err_del (&(self->err));
  
  if (cdata_udc_type != CAPE_UDC_UNDEFINED)
  {
    // we don't need a name
    self->cdata = cape_udc_new (cdata_udc_type, NULL);
  }
}

//-----------------------------------------------------------------------------

QBusM qbus_message_mv (QBusM original)
{
  QBusM self = CAPE_NEW (struct QBusMessage_s);

  self->mtype = original->mtype;
  
  self->chain_key = cape_str_mv (&(original->chain_key));
  self->sender = cape_str_mv (&(original->sender));
  
  self->cdata = cape_udc_mv (&(original->cdata));
  self->pdata = cape_udc_mv (&(original->pdata));
  self->clist = cape_udc_mv (&(original->clist));
  self->rinfo = cape_udc_mv (&(original->rinfo));
  self->files = cape_udc_mv (&(original->files));
  self->files = cape_udc_mv (&(original->files));

  self->blob = cape_stream_mv (&(original->blob));
  
  return self;
}

//-----------------------------------------------------------------------------

void qbus_message_del (QBusM* p_self)
{
  if (*p_self)
  {
    QBusM self = *p_self;
    
    qbus_message_clr (self, CAPE_UDC_UNDEFINED);
    
    // only clear it here
    cape_udc_del (&(self->rinfo));
    cape_udc_del (&(self->files));
    
    cape_str_del (&(self->chain_key));
    cape_str_del (&(self->sender));
    
    CAPE_DEL (p_self, struct QBusMessage_s);
  }
}

//-----------------------------------------------------------------------------

int qbus_message_role_has (QBusM self, const CapeString role_name)
{
  CapeUdc roles;
  CapeUdc role;

  if (self->rinfo == NULL)
  {
    return FALSE;
  }
  
  roles = cape_udc_get (self->rinfo, "roles");
  if (roles == NULL)
  {
    return FALSE;
  }

  role = cape_udc_get (roles, role_name);
  if (role == NULL)
  {
    return FALSE;
  }
  
  return TRUE;
}

//-----------------------------------------------------------------------------

int qbus_message_role_or2 (QBusM self, const CapeString role01, const CapeString role02)
{
  CapeUdc roles;
  CapeUdc role;
  
  if (self->rinfo == NULL)
  {
    cape_log_msg (CAPE_LL_WARN, "QBUS", "role or2", "rinfo is NULL");
    return FALSE;
  }
  
  roles = cape_udc_get (self->rinfo, "roles");
  if (roles == NULL)
  {
    return FALSE;
  }

  role = cape_udc_get (roles, role01);
  if (role == NULL)
  {
    role = cape_udc_get (roles, role02);
    if (role == NULL)
    {
      return FALSE;
    }
  }
  
  return TRUE;
}

//-----------------------------------------------------------------------------
