#ifndef __QBUS_MESSAGE__H
#define __QBUS_MESSAGE__H 1

#include "sys/cape_export.h"
#include "sys/cape_err.h"
#include "stc/cape_udc.h"
#include "aio/cape_aio_ctx.h"

//-----------------------------------------------------------------------------

struct QBusMessage_s
{
  number_t mtype;

  CapeUdc clist;    // list of all parameters
  
  CapeUdc cdata;    // public object as parameters
  
  CapeUdc pdata;    // private object as parameters
  
  CapeUdc rinfo;
  
  CapeUdc files;    // if the content is too big, payload is stored in temporary files
  
  CapeStream blob;  // binary blob within the CapeStream
  
  CapeErr err;
  
  CapeString chain_key;  // don't change this key
  
  CapeString sender;     // don't change this
  
}; typedef struct QBusMessage_s* QBusM;

//-----------------------------------------------------------------------------

__CAPE_LIBEX   QBusM              qbus_message_new       (const CapeString key, const CapeString sender);

__CAPE_LIBEX   void               qbus_message_del       (QBusM*);

__CAPE_LIBEX   void               qbus_message_clr       (QBusM, u_t cdata_udc_type);

//-----------------------------------------------------------------------------

#endif
