#ifndef __QJOBS__H
#define __QJOBS__H 1

//-----------------------------------------------------------------------------

// cape includes
#include "sys/cape_export.h"
#include "sys/cape_types.h"
#include <sys/cape_err.h>
#include <aio/cape_aio_ctx.h>
#include <sys/cape_time.h>
#include <stc/cape_str.h>
#include <stc/cape_udc.h>

// adbl includes
#include <adbl.h>

//-----------------------------------------------------------------------------

struct QJobs_s; typedef struct QJobs_s* QJobs;

//-----------------------------------------------------------------------------

__CAPE_LIBEX     QJobs       qjobs_new      (AdblSession adbl_session, const CapeString database_table);

__CAPE_LIBEX     void        qjobs_del      (QJobs*);

//-----------------------------------------------------------------------------

typedef void                  (__STDCALL *fct_qjobs__on_event)   (void* user_ptr, CapeUdc params);

__CAPE_LIBEX     int         qjobs_init     (QJobs, CapeAioContext aio_ctx, number_t precision_in_ms, void* user_ptr, fct_qjobs__on_event, CapeErr);

//-----------------------------------------------------------------------------

__CAPE_LIBEX     int         qjobs_event    (QJobs, number_t wpid, number_t gpid, CapeDatetime* dt, number_t repeats, CapeUdc* p_params, const CapeString ref_mod, const CapeString ref_umi, number_t ref_id1, number_t ref_id2, CapeErr);

//-----------------------------------------------------------------------------

#endif
