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

__CAPE_LIBEX     QJobs    qjobs_new        (AdblSession adbl_session, const CapeString database_table);

__CAPE_LIBEX     void     qjobs_del        (QJobs*);

//-----------------------------------------------------------------------------

struct QJobsEvent_s
{
  number_t rpid;
  number_t wpid;
  number_t gpid;
  
  CapeUdc params;
  const CapeString rinfo_encrypted;
  
  CapeString ref_mod;
  CapeString ref_umi;

  number_t r1id;
  number_t r2id;
  
}; typedef struct QJobsEvent_s* QJobsEvent;

                         /*
                            return ->
                            CAPE_ERR_NONE: continue with next period
                            CAPE_ERR_CONTINUE: for asyncronous return, don't delete the job will be continued later
                            CAPE_ERR_EOF: stop and delete the job
                          */
typedef int          (__STDCALL *fct_qjobs__on_event)   (QJobs, QJobsEvent, void* user_ptr, CapeErr err);

__CAPE_LIBEX     int      qjobs_init       (QJobs, CapeAioContext aio_ctx, number_t precision_in_ms, void* user_ptr, fct_qjobs__on_event, CapeErr);

//-----------------------------------------------------------------------------

__CAPE_LIBEX     int      qjobs_add        (QJobs, CapeDatetime* dt, number_t period_in_s, CapeUdc* p_params, CapeUdc rinfo, const CapeString ref_mod, const CapeString ref_umi, number_t ref_id1, number_t ref_id2, const CapeString vsec, CapeErr);

__CAPE_LIBEX     int      qjobs_set        (QJobs, number_t rpid, CapeDatetime* dt, number_t period_in_s, CapeUdc* p_params, const CapeString vsec, CapeErr);

__CAPE_LIBEX     int      qjobs_rm         (QJobs, number_t jobid, CapeErr);

__CAPE_LIBEX     int      qjobs_run        (QJobs, number_t jobid, CapeErr);

//-----------------------------------------------------------------------------

#endif
