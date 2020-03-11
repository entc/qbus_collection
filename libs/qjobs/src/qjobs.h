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

__CAPE_LIBEX     int         qjobs_init     (QJobs, CapeAioContext aio_ctx, void* user_ptr, fct_qjobs__on_event, CapeErr);

//-----------------------------------------------------------------------------

__CAPE_LIBEX     int         qjobs_event    (QJobs, CapeDatetime* dt, CapeUdc* p_params, CapeErr);

//-----------------------------------------------------------------------------

#endif
