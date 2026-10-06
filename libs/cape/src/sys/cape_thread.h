#ifndef __CAPE_SYS__THREAD__H
#define __CAPE_SYS__THREAD__H 1

#include "sys/cape_export.h"
#include "sys/cape_err.h"
#include "sys/cape_types.h"

//=============================================================================

typedef int (__STDCALL *cape_thread_worker_fct)(void* ptr);
typedef void (__STDCALL *cape_thread_on_done)(void* ptr);

struct CapeThread_s; typedef struct CapeThread_s* CapeThread;

//-----------------------------------------------------------------------------

__CAPE_LIBEX   CapeThread      cape_thread_new         (void);                // allocate memory and initialize the object

__CAPE_LIBEX   void            cape_thread_del         (CapeThread*);

__CAPE_LIBEX   void            cape_thread_start       (CapeThread, cape_thread_worker_fct, void* ptr);

__CAPE_LIBEX   void            cape_thread_join        (CapeThread);

__CAPE_LIBEX   void            cape_thread_cancel      (CapeThread);

__CAPE_LIBEX   void            cape_thread_cb          (CapeThread, cape_thread_on_done);

__CAPE_LIBEX   void            cape_thread_signal      (CapeThread);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   void            cape_thread_nosignals       ();

//-----------------------------------------------------------------------------

__CAPE_LIBEX   void            cape_thread_sleep           (unsigned long milliseconds);

                               /* returns the available physical cores of the system */
__CAPE_LIBEX   number_t        cape_thread_concurrency     ();

//-----------------------------------------------------------------------------

struct CapeThreadPool_s; typedef struct CapeThreadPool_s* CapeThreadPool;
struct CapeThreadPoolItem_s; typedef struct CapeThreadPoolItem_s* CapeThreadPoolItem;

/* will be called before thread was created and started */
typedef void* (__STDCALL *cape_thread_pool__on_new)(void* user_ptr);

/* will be called after the join of the thread */
typedef void (__STDCALL *cape_thread_pool__on_del)(void* obj_ptr);

/* will be called before the join */
typedef void (__STDCALL *cape_thread_pool__on_stop)(void* obj_ptr, void* user_ptr);

/* will be called as worker thread */
typedef int (__STDCALL *cape_thread_pool__worker)(void* obj_ptr, void* user_ptr, CapeThreadPoolItem);

//-----------------------------------------------------------------------------

                               /* constructor */
__CAPE_LIBEX   CapeThreadPool  cape_thread_pool_new        (void* user_ptr, cape_thread_pool__worker, cape_thread_pool__on_new, cape_thread_pool__on_stop, cape_thread_pool__on_del);

                               /* destructor, stops all threads */
__CAPE_LIBEX   void            cape_thread_pool_del        (CapeThreadPool*);

                               /* starts all threads */
__CAPE_LIBEX   void            cape_thread_pool_start      (CapeThreadPool, number_t amount, number_t timeout_in_ms);

                               /* send a signal to all threads */
__CAPE_LIBEX   void            cape_thread_pool_signal     (CapeThreadPool);

//-----------------------------------------------------------------------------

                               /* sets idle or active status of a thread */
__CAPE_LIBEX   void            cape_thread_item_set        (CapeThreadPoolItem, int active);

//-----------------------------------------------------------------------------

                               /* atomic increase of p_var, returns the old value of p_var */
__CAPE_LIBEX   number_t        cape_thread_atomic_inc      (number_t* p_var);

                               /* atomic increase of p_var (only if p_var is not negative), returns the old value of p_var */
__CAPE_LIBEX   number_t        cape_thread_atomic_inc__nn  (number_t* p_var);

                               /* atomic decrease of p_var, returns the old value of p_var */
__CAPE_LIBEX   number_t        cape_thread_atomic_dec      (number_t* p_var);

                               /* atomic decrease of p_var (only if p_var is not negative), returns the old value of p_var */
__CAPE_LIBEX   number_t        cape_thread_atomic_dec__nn  (number_t* p_var);

//-----------------------------------------------------------------------------

#endif



