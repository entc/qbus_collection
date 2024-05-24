#ifndef __QBUS_MANIFOLD_MEMBER__H
#define __QBUS_MANIFOLD_MEMBER__H 1

// cape includes
#include <sys/cape_export.h>
#include <sys/cape_err.h>
#include <stc/cape_udc.h>

// qbus includes
#include "qbus_tl1.h"
#include "qbus_method.h"

//-----------------------------------------------------------------------------

struct QBusManifoldMember_s; typedef struct QBusManifoldMember_s* QBusManifoldMember;

//-----------------------------------------------------------------------------

__CAPE_LIBEX   QBusManifoldMember      qbus_manifold_member_new    (const CapeString name, void* user_ptr, fct_qbus_manifold__on_add on_add, fct_qbus_manifold__on_rm on_rm, fct_qbus_manifold__on_call on_call,fct_qbus_manifold__on_recv on_recv, fct_qbus_manifold__on_emit on_emit);

__CAPE_LIBEX   void                    qbus_manifold_member_del    (QBusManifoldMember*);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   const CapeString        qbus_manifold_member_name   (QBusManifoldMember);

__CAPE_LIBEX   void                    qbus_manifold_member_add    (QBusManifoldMember, const char* uuid, const char* module, void* node);

__CAPE_LIBEX   void                    qbus_manifold_member_set    (QBusManifoldMember, void* node);

__CAPE_LIBEX   int                     qbus_manifold_member_call   (QBusManifoldMember, const CapeString method_name, QBusMethod* p_qbus_method, QBusM msg, CapeErr err);

__CAPE_LIBEX   void                    qbus_manifold_member_emit   (QBusManifoldMember, CapeUdc val, const CapeString uuid, const CapeString name);

//-----------------------------------------------------------------------------

#endif

