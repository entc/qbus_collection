#ifndef __JOBS__SCHED__H
#define __JOBS__SCHED__H 1

// adbl includes
#include <adbl.h>

// qbus includes
#include <qbus.h>

// cape includes
#include <sys/cape_export.h>
#include <sys/cape_types.h>
#include <sys/cape_queue.h>

//-----------------------------------------------------------------------------

struct JobsScheduler_s; typedef struct JobsScheduler_s* JobsScheduler;

//-----------------------------------------------------------------------------

__CAPE_LIBEX   JobsScheduler  jobs_sched_new   (QBus, AdblSession adbl_session);

__CAPE_LIBEX   void           jobs_sched_del   (JobsScheduler*);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   int            jobs_sched_init  (JobsScheduler, QBus qbus, CapeErr err);

__CAPE_LIBEX   int            jobs_sched_add   (JobsScheduler, CapeErr err);

//-----------------------------------------------------------------------------

#endif
