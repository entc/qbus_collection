#ifndef __JOBS__LIST__H
#define __JOBS__LIST__H 1

// qbus includes
#include <qbus.h>

// adbl includes
#include <adbl.h>

// cape includes
#include <sys/cape_export.h>
#include <sys/cape_types.h>
#include <sys/cape_queue.h>

// qjobs includes
#include <qjobs.h>

//-----------------------------------------------------------------------------

struct JobsList_s; typedef struct JobsList_s* JobsList;

//-----------------------------------------------------------------------------

__CAPE_LIBEX   JobsList    jobs_list_new          (QBus, AdblSession adbl_session, QJobs jobs);

__CAPE_LIBEX   void        jobs_list_del          (JobsList*);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   int         jobs_list_get          (JobsList*, QBusM qin, QBusM qout, CapeErr err);

__CAPE_LIBEX   int         jobs_list_add          (JobsList*, QBusM qin, QBusM qout, CapeErr err);

__CAPE_LIBEX   int         jobs_list_set          (JobsList*, QBusM qin, QBusM qout, CapeErr err);

__CAPE_LIBEX   int         jobs_list_rm           (JobsList*, QBusM qin, QBusM qout, CapeErr err);

__CAPE_LIBEX   int         jobs_list_run          (JobsList*, QBusM qin, QJobsEvent event, CapeErr err);

//-----------------------------------------------------------------------------

#endif
