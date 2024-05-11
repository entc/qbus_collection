#ifndef __QBUS_TL1__H
#define __QBUS_TL1__H 1

// cape includes
#include <sys/cape_export.h>
#include <sys/cape_err.h>
#include <stc/cape_udc.h>

// qbus includes
#include "qbus_method.h"

//-----------------------------------------------------------------------------

struct QBusManifold_s; typedef struct QBusManifold_s* QBusManifold; // use a simple version

//-----------------------------------------------------------------------------

typedef void          (__STDCALL *fct_qbus_manifold__on_rm)       (void* user_ptr);

typedef void          (__STDCALL *fct_qbus_manifold__on_add)      (void* user_ptr, const char* uuid, const char* module, void* node);

typedef void          (__STDCALL *fct_qbus_manifold__on_call)     (void* user_ptr, const CapeString method_name, QBusMethod* p_qbus_method, QBusM msg);

typedef void          (__STDCALL *fct_qbus_manifold__on_recv)     (void* user_ptr);

typedef void          (__STDCALL *fct_qbus_manifold__on_emit)     (void* user_ptr, CapeUdc val, const CapeString method_ident, const CapeString method_name);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   QBusManifold       qbus_manifold_new               ();

__CAPE_LIBEX   void               qbus_manifold_del               (QBusManifold*);

__CAPE_LIBEX   int                qbus_manifold_init              (QBusManifold, const CapeString uuid, const CapeString name, void* user_ptr, fct_qbus_manifold__on_add, fct_qbus_manifold__on_rm, fct_qbus_manifold__on_call, fct_qbus_manifold__on_recv, fct_qbus_manifold__on_emit, CapeErr err);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   void               qbus_manifold_emit              (QBusManifold, const CapeString uuid, CapeUdc val);

__CAPE_LIBEX   void               qbus_manifold_subscribe         (QBusManifold, const CapeString uuid, const CapeString module_uuid, const CapeString module_name, const CapeString value_name);

__CAPE_LIBEX   void               qbus_manifold_response          (QBusManifold, QBusM msg);

__CAPE_LIBEX   int                qbus_manifold_send              (QBusManifold, const CapeString uuid, void** p_node, const CapeString method, QBusM msg, QBusMethod* p_qbus_method, CapeErr);

//-----------------------------------------------------------------------------

#endif
