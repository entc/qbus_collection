#include "qbus_message.h"

//-----------------------------------------------------------------------------

#define QBUS_MTYPE_NONE         0
#define QBUS_MTYPE_JSON         1
#define QBUS_MTYPE_FILE         2

//-----------------------------------------------------------------------------

QBusM qbus_message_new (const CapeString key, const CapeString sender)
{
  QBusM self = CAPE_NEW (struct QBusMessage_s);
  
  if (key)
  {
    // clone the key
    self->chain_key = cape_str_cp (key);
  }
  else
  {
    // create a new key
    self->chain_key = cape_str_uuid ();
  }
  
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
