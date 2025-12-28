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

__CAPE_LIBEX   QBusMethodItem       qbus_method_item_new      (void* user_ptr, fct_qbus_on_msg on_msg, const CapeString saves_key, const CapeString sender, const CapeString cid, const CapeUdc rinfo);

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

//-----------------------------------------------------------------------------

__CAPE_LIBEX   int                  qbus_methods__rpc_add     (QBusMethods, const CapeString method, void* user_ptr, fct_qbus_on_msg, fct_qbus_on_rm, CapeErr err);

__CAPE_LIBEX   int                  qbus_methods_run          (QBusMethods, const CapeString method, const CapeString saves_key, QBusM* p_msg, CapeErr err);

__CAPE_LIBEX   void                 qbus_methods_response     (QBusMethods, QBusMethodItem mitem, QBusM* p_msg, CapeErr err);

__CAPE_LIBEX   void                 qbus_methods_send         (QBusMethods, const CapeString saves_key, CapeErr err);

__CAPE_LIBEX   void                 qbus_methods_queue        (QBusMethods, QBusMethodItem mitem, QBusM* p_qin, const CapeString saves_key);

__CAPE_LIBEX   void                 qbus_methods_abort        (QBusMethods, const CapeString cid, const CapeString name);

__CAPE_LIBEX   QBusMethodItem       qbus_methods_load         (QBusMethods, const CapeString save_key);

__CAPE_LIBEX   const CapeString     qbus_methods_save         (QBusMethods, void* user_ptr, fct_qbus_on_msg, const CapeString saves_key, const CapeString cid_src, CapeUdc rinfo, const CapeString cid_dst);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   int                  qbus_methods__sub_add     (QBusMethods, const CapeString topic, void* user_ptr, fct_qbus_on_val, CapeErr err);

__CAPE_LIBEX   void                 qbus_methods__sub_rm      (QBusMethods, const CapeString topic);

__CAPE_LIBEX   int                  qbus_methods__sub_run     (QBusMethods, const CapeString topic, CapeUdc* p_val, CapeErr err);

//-----------------------------------------------------------------------------

#endif
