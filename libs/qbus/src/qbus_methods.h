#ifndef __QBUS_METHODS__H
#define __QBUS_METHODS__H 1

#include "sys/cape_export.h"
#include "sys/cape_err.h"
#include "stc/cape_udc.h"
#include "stc/cape_stream.h"
#include "aio/cape_aio_ctx.h"

#include "qbus_message.h"
#include "qbus.h"

//-----------------------------------------------------------------------------

struct QBusMethodItem_s; typedef struct QBusMethodItem_s* QBusMethodItem;

__CAPE_LIBEX   void                 qbus_method_item_del      (QBusMethodItem* p_self);

__CAPE_LIBEX   const CapeString     qbus_method_item_cid      (QBusMethodItem);

__CAPE_LIBEX   const CapeString     qbus_method_item_skey     (QBusMethodItem);

//-----------------------------------------------------------------------------

struct QBusMethods_s; typedef struct QBusMethods_s* QBusMethods;

//-----------------------------------------------------------------------------

typedef void           (__STDCALL *fct_qbus_methods__on_res) (void* user_ptr, QBusMethodItem mitem, QBusM* p_msg);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   QBusMethods          qbus_methods_new          (QBus);

__CAPE_LIBEX   void                 qbus_methods_del          (QBusMethods*);

__CAPE_LIBEX   int                  qbus_methods_init         (QBusMethods, number_t threads, void* user_ptr, fct_qbus_methods__on_res, CapeErr);

__CAPE_LIBEX   int                  qbus_methods_add          (QBusMethods, const CapeString method, void* user_ptr, fct_qbus_on_msg, fct_qbus_on_rm, CapeErr err);

__CAPE_LIBEX   int                  qbus_methods_run          (QBusMethods, const CapeString method, const CapeString saves_key, QBusM* p_msg, CapeErr err);

__CAPE_LIBEX   void                 qbus_methods_response     (QBusMethods, QBusMethodItem mitem, QBusM* p_msg, CapeErr err);

__CAPE_LIBEX   void                 qbus_methods_queue        (QBusMethods, QBusMethodItem mitem, QBusM* p_qin, const CapeString saves_key);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   QBusMethodItem       qbus_methods_load         (QBusMethods, const CapeString save_key);

__CAPE_LIBEX   const CapeString     qbus_methods_save         (QBusMethods, void* user_ptr, fct_qbus_on_msg, const CapeString saves_key, const CapeString sender, CapeUdc rinfo, const CapeString debug);

//-----------------------------------------------------------------------------

#endif
