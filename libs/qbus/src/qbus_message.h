#ifndef __QBUS_MESSAGE__H
#define __QBUS_MESSAGE__H 1

#include "sys/cape_export.h"
#include "sys/cape_err.h"
#include "stc/cape_udc.h"
#include "aio/cape_aio_ctx.h"

//-----------------------------------------------------------------------------

#define QBUS_MTYPE_NONE         0
#define QBUS_MTYPE_JSON         1
#define QBUS_MTYPE_FILE         2
#define QBUS_MTYPE_SUBV         3

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

                                  /* transfers the content to another QBusM object */
__CAPE_LIBEX   QBusM              qbus_message_mv        (QBusM original);

__CAPE_LIBEX   int                qbus_message_role_has  (QBusM, const CapeString role_name);

__CAPE_LIBEX   int                qbus_message_role_or2  (QBusM, const CapeString role01, const CapeString role02);

__CAPE_LIBEX   CapeErr            qbus_message_err_new   (QBusM);

//-----------------------------------------------------------------------------

#endif
