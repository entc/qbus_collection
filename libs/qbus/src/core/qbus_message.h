#ifndef __QBUS__MESSAGE__H
#define __QBUS__MESSAGE__H 1

#include "qbus_frame.h"

#include <sys/cape_export.h>
#include <sys/cape_err.h>
#include <stc/cape_udc.h>

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

struct QBus_s; typedef struct QBus_s* QBus; // use a simple version

typedef int    (__STDCALL         *fct_qbus_onMessage)   (QBus, void* ptr, QBusM qin, QBusM qout, CapeErr);
typedef void   (__STDCALL         *fct_qbus_onRemoved)   (void* ptr);
typedef void     (__STDCALL     *fct_qbus_on_methods)    (QBus, void* ptr, const CapeUdc modules, CapeErr err);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   QBusM              qbus_message_new       (const CapeString key, const CapeString sender);

__CAPE_LIBEX   void               qbus_message_del       (QBusM*);

__CAPE_LIBEX   void               qbus_message_clr       (QBusM, u_t cdata_udc_type);

__CAPE_LIBEX   QBusM              qbus_message_data_mv   (QBusM);

__CAPE_LIBEX   QBusM              qbus_message_frame     (QBusFrame);

__CAPE_LIBEX   CapeUdc            qbus_frame_set_qmsg    (QBusFrame, QBusM qmsg, CapeErr err);

__CAPE_LIBEX   void               qbus_message_dump      (QBusM, const CapeString description);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   int                qbus_message_role_has  (QBusM, const CapeString role_name);

__CAPE_LIBEX   int                qbus_message_role_or2  (QBusM, const CapeString role01, const CapeString role02);

//-----------------------------------------------------------------------------

__CAPE_LOCAL   void               qbus_message__no_route (QBusM, QBus, void* ptr, fct_qbus_onMessage onMsg);

//-----------------------------------------------------------------------------

#endif