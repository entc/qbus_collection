#ifndef __CAPE_SYS__QUEUE__H
#define __CAPE_SYS__QUEUE__H 1

#include "sys/cape_export.h"
#include "sys/cape_err.h"
#include "sys/cape_types.h"

//-----------------------------------------------------------------------------

struct CapeSync_s; typedef struct CapeSync_s* CapeSync;

//-----------------------------------------------------------------------------

__CAPE_LIBEX   CapeSync    cape_sync_new           (void);

__CAPE_LIBEX   void        cape_sync_del           (CapeSync*);

__CAPE_LIBEX   void        cape_sync_inc           (CapeSync);

__CAPE_LIBEX   void        cape_sync_dec           (CapeSync);

__CAPE_LIBEX   void        cape_sync_wait          (CapeSync);

//-----------------------------------------------------------------------------

struct CapeQueue_s; typedef struct CapeQueue_s* CapeQueue;

typedef void (__STDCALL *cape_queue_cb_fct)(void* ptr, number_t pos, number_t queue_size);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   CapeQueue   cape_queue_new          (number_t timeout_in_ms);

                           /*
                            * frees memory and wait for all threads to terminate
                            * -> if there is a task running, it blocks until it has been finished
                            */
__CAPE_LIBEX   void        cape_queue_del          (CapeQueue*);

//-----------------------------------------------------------------------------

                           /*
                            * adds a new task
                            */
__CAPE_LIBEX   void        cape_queue_add          (CapeQueue, CapeSync, cape_queue_cb_fct on_event, cape_queue_cb_fct on_done, cape_queue_cb_fct on_cancel, void* ptr, number_t pos);

                           /*
                            * starts the queueing in background
                            * -> threads will be created
                            * -> amount of threads defines the worker threads waiting for events
                            */
__CAPE_LIBEX   int         cape_queue_start        (CapeQueue, number_t amount_of_threads, CapeErr err);

//-----------------------------------------------------------------------------

#endif
