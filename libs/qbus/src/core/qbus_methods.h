#ifndef __QBUS__METHODS__H
#define __QBUS__METHODS__H 1

#include "qbus_route.h"
#include "qbus_message.h"
#include "qbus_queue.h"

#include <sys/cape_export.h>
#include <sys/cape_err.h>
#include <stc/cape_udc.h>

//-----------------------------------------------------------------------------

#define QBUS_METHOD_TYPE__REQUEST      1
#define QBUS_METHOD_TYPE__RESPONSE     2
#define QBUS_METHOD_TYPE__FORWARD      3
#define QBUS_METHOD_TYPE__METHODS      4

//-----------------------------------------------------------------------------

struct QBusMethods_s; typedef struct QBusMethods_s* QBusMethods;

//-----------------------------------------------------------------------------

__CAPE_LOCAL   QBusMethods        qbus_methods_new                    (QBusEngines, QBusQueue);

__CAPE_LOCAL   void               qbus_methods_del                    (QBusMethods*);

__CAPE_LOCAL   int                qbus_methods_add                    (QBusMethods, const CapeString method, void* user_ptr, fct_qbus_onMessage on_event, CapeErr);

//-----------------------------------------------------------------------------

__CAPE_LOCAL   void               qbus_methods_handle_response        (QBusMethods, QBus qbus, QBusM msg);

__CAPE_LOCAL   void               qbus_methods_proc_request           (QBusMethods, QBus qbus, QBusQueueItem qitem, const CapeString sender);

//-----------------------------------------------------------------------------

__CAPE_LOCAL   void               qbus_methods_call                   (QBusMethods, QBusFrame frame, QBusPvdConnection conn);

__CAPE_LOCAL   void               qbus_methods_recv_response          (QBusMethods, QBusFrame* p_frame, QBusPvdConnection conn);

__CAPE_LOCAL   void               qbus_methods_recv_methods           (QBusMethods, QBusPvdConnection conn);

__CAPE_LOCAL   void               qbus_methods_recv_forward           (QBusMethods, QBusFrame frame, QBusPvdConnection conn, const CapeString sender);

//-----------------------------------------------------------------------------

__CAPE_LOCAL   void               qbus_methods_send_request           (QBusMethods, QBusPvdConnection conn, QBusQueueItem qitem, const CapeString sender, int cont);

__CAPE_LOCAL   void               qbus_methods_send_methods           (QBusMethods, QBusPvdConnection conn, const CapeString module, const CapeString sender, void* user_ptr, fct_qbus_on_methods user_fct);

//-----------------------------------------------------------------------------

#endif
