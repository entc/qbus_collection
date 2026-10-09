#include "cape_queue.h"

// cape includes
#include "sys/cape_types.h"
#include "sys/cape_log.h"
#include "sys/cape_mutex.h"
#include "sys/cape_thread.h"
#include "stc/cape_list.h"

//-----------------------------------------------------------------------------

#if defined(CAPE_USE_FREERTOS)

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#define CAPE_SYNC_EVENT_DONE    (1 << 0)

#elif defined(__LINUX_OS)

#include <unistd.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <semaphore.h>
#include <errno.h>
#include <time.h>

#elif defined(__BSD_OS)

#include <unistd.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <dispatch/dispatch.h>

#elif defined(__WINDOWS_OS)

#include <windows.h>

#endif

//-----------------------------------------------------------------------------

struct CapeSync_s
{
#if defined(CAPE_USE_FREERTOS)

    int refcnt;
    SemaphoreHandle_t mutex;
    EventGroupHandle_t revent;
    
#elif defined(__LINUX_OS) || defined(__BSD_OS)

    int semid;

#elif defined(__WINDOWS_OS)

    LONG refcnt;
    HANDLE revent;

#endif
};

//-----------------------------------------------------------------------------

CapeSync cape_sync_new (void)
{
  CapeSync self = CAPE_NEW (struct CapeSync_s);
  
#if defined(CAPE_USE_FREERTOS)

    self->refcnt = 0;

    self->mutex = xSemaphoreCreateMutex ();
    self->revent = xEventGroupCreate ();
    
    xEventGroupSetBits (self->revent, CAPE_SYNC_EVENT_DONE);
    
#elif defined(__LINUX_OS) || defined(__BSD_OS)

    self->semid = semget (IPC_PRIVATE, 1, IPC_CREAT  | IPC_EXCL | 0666);
    
    if (self->semid == -1)
    {
        cape_log_err (CAPE_LL_ERROR, "CAPE", "SYNC", "can't create semaphore");
    }
    else
    {
        semctl (self->semid, 0, SETVAL, 0);
    }
    
#elif defined(__WINDOWS_OS)

    self->refcnt = 0;
    self->revent = CreateEvent (NULL, TRUE, TRUE, NULL);

#endif
    
    return self;
}

//-----------------------------------------------------------------------------

void cape_sync_del (CapeSync* p_self)
{
    if (*p_self)
    {
        CapeSync self = *p_self;

        cape_sync_wait (self);

#if defined(CAPE_USE_FREERTOS)

        vEventGroupDelete (self->revent);
        vSemaphoreDelete (self->mutex);
        
#elif defined(__LINUX_OS) || defined(__BSD_OS)

        semctl(self->semid, 0, IPC_RMID);

#elif defined(__WINDOWS_OS)

        CloseHandle (self->revent);

#endif

        CAPE_DEL (p_self, struct CapeSync_s);
    }
}

//-----------------------------------------------------------------------------

void cape_sync_inc (CapeSync self)
{
    if (self)
    {
#if defined(CAPE_USE_FREERTOS)

        xSemaphoreTake (self->mutex, portMAX_DELAY);

        self->refcnt++;
        xEventGroupClearBits (self->revent, CAPE_SYNC_EVENT_DONE);

        xSemaphoreGive (self->mutex);
        
#elif defined(__LINUX_OS) || defined(__BSD_OS)

        struct sembuf sops[1];

        sops[0].sem_num = 0;
        sops[0].sem_op = 1;
        sops[0].sem_flg = 0;

        if (semop (self->semid, sops, 1) == -1)
        {
            cape_log_err (CAPE_LL_ERROR, "CAPE", "SYNC", "can't increase semaphore");
        }

#elif defined(__WINDOWS_OS)

        InterlockedIncrement (&(self->refcnt));

        ResetEvent (self->revent);

#endif
    }
}

//-----------------------------------------------------------------------------

void cape_sync_dec (CapeSync self)
{
    if (self)
    {
#if defined(CAPE_USE_FREERTOS)

        xSemaphoreTake (self->mutex, portMAX_DELAY);

        self->refcnt--;

        if (self->refcnt == 0)
        {
            xEventGroupSetBits (self->revent, CAPE_SYNC_EVENT_DONE);
        }

        xSemaphoreGive (self->mutex);
        
#elif defined(__LINUX_OS) || defined(__BSD_OS)

        struct sembuf sops[1];
        
        sops[0].sem_num = 0;
        sops[0].sem_op = -1;
        sops[0].sem_flg = 0;
        
        if (semop (self->semid, sops, 1) == -1)
        {
            cape_log_err (CAPE_LL_ERROR, "CAPE", "SYNC", "can't decrease semaphore");
        }

#elif defined(__WINDOWS_OS)

        int var = InterlockedDecrement (&(self->refcnt));
        if (var == 0)
        {
            SetEvent (self->revent);
        }

#endif
    }
}

//-----------------------------------------------------------------------------

void cape_sync_wait (CapeSync self)
{
    if (self)
    {

#if defined(CAPE_USE_FREERTOS)

        xEventGroupWaitBits (self->revent, CAPE_SYNC_EVENT_DONE, pdFALSE, pdTRUE, portMAX_DELAY);
        
#elif defined(__LINUX_OS) || defined(__BSD_OS)

        struct sembuf sops[1];
        
        sops[0].sem_op = 0;
        sops[0].sem_flg = 0;
        sops[0].sem_num = 0;
        
        if (semop (self->semid, sops, 1) == -1)
        {
            cape_log_err (CAPE_LL_ERROR, "CAPE", "SYNC", "can't wait semaphore");
        }

#elif defined(__WINDOWS_OS)

        if (WaitForSingleObject (self->revent, INFINITE) == WAIT_OBJECT_0)
        {
            
        }
        else
        {
            cape_log_err (CAPE_LL_ERROR, "CAPE", "SYNC", "can't wait for SingleObject");
        }

#endif
    }
}

//-----------------------------------------------------------------------------

struct CapeQueue_s
{
    CapeMutex mutex;
    
    CapeThreadPool thread_pool;
    
    CapeList queue;
    
#if defined(CAPE_USE_FREERTOS)

    SemaphoreHandle_t sem;

#elif defined(__LINUX_OS)
            
    sem_t sem;    // semaphore structure

#elif defined(__BSD_OS)

    dispatch_semaphore_t sem;

#elif defined(__WINDOWS_OS)

    HANDLE semaphore;

#endif

    int terminated;
    
    number_t timeout_in_ms;
};

//-----------------------------------------------------------------------------

struct CapeQueueItem_s
{
    cape_queue_cb_fct on_event;
    
    cape_queue_cb_fct on_done;
    
    cape_queue_cb_fct on_cancel;
    
    void* ptr;
    
    CapeSync sync;    // reference
    
    number_t pos;

}; typedef struct CapeQueueItem_s* CapeQueueItem;

//-----------------------------------------------------------------------------

void __STDCALL cape_queue__item__on_del (void* ptr)
{
  CapeQueueItem item = ptr;
  
  if (item->on_done)
  {
    item->on_done (item->ptr, item->pos, 0);
  }
  
  cape_sync_dec (item->sync);
  
  CAPE_DEL (&item, struct CapeQueueItem_s);
}

//-----------------------------------------------------------------------------

static int cape_queue__next (CapeQueue self)
{
#if defined(__LINUX_OS)

    struct timespec ts;
    
    if (clock_gettime (CLOCK_REALTIME, &ts) == -1)
    {
        cape_log_err (CAPE_LL_ERROR, "CAPE", "QUEUE", "can't get realtime clock");
      
        return FALSE;
    }
    
    ts.tv_sec += 5;
    
    int res = sem_timedwait (&(self->sem), &ts);
    
    if (res == -1)
    {
        switch (errno)
        {
            case EINTR:
            case ETIMEDOUT:
            {
                break;
            }
            default:
            {
                cape_log_err (CAPE_LL_ERROR, "CAPE", "QUEUE", "can't permforme sem_wait");
                return FALSE;
            }
        }
    }

#elif defined(__BSD_OS)

    dispatch_semaphore_wait (self->sem, dispatch_time (DISPATCH_TIME_NOW, 5 * NSEC_PER_SEC));

#elif defined(__WINDOWS_OS)

    DWORD res = WaitForSingleObject (self->semaphore, INFINITE);
    
    if (res == WAIT_OBJECT_0)
    {
        // done
    }
    else
    {
        cape_log_err (CAPE_LL_ERROR, "CAPE", "QUEUE", "can't permforme queue next");
    }

#elif defined(CAPE_USE_FREERTOS)

    if (xSemaphoreTake (self->sem, pdMS_TO_TICKS (5000)) != pdTRUE)
    {
        
    }
    
#endif

    return TRUE;
}

//-----------------------------------------------------------------------------

static CapeQueueItem cape_queue__pop (CapeQueue self)
{
    CapeQueueItem item = NULL;

    cape_mutex_lock (self->mutex);
    
    item = cape_list_pop_front (self->queue);
    
    cape_mutex_unlock (self->mutex);

    return item;
}

//-----------------------------------------------------------------------------

static number_t cape_queue__size (CapeQueue self)
{
    number_t queue_size;
    
    cape_mutex_lock (self->mutex);
    
    queue_size = cape_list_size (self->queue);
    
    cape_mutex_unlock (self->mutex);

    return queue_size;
}

//-----------------------------------------------------------------------------

static void* __STDCALL cape_queue__on_new_item (void* user_ptr)
{
    CapeQueue self = user_ptr;
    
    // waits until a possible queue event might be available
    if (cape_queue__next (self))
    {
        // fetch the queue event and return it as obj_ptr
        return cape_queue__pop (self);
    }
    else
    {
        return NULL;
    }
}

//-----------------------------------------------------------------------------

static int __STDCALL cape_queue__worker__thread (void* obj_ptr, void* user_ptr, CapeThreadPoolItem pool_item)
{
    CapeQueue self = user_ptr;
    CapeQueueItem item = obj_ptr;
    
    if (item && item->on_event)
    {
        // set the active state
        cape_thread_item_set (pool_item, TRUE);
        
        // we don't know what happens here
        // this might block forever
        item->on_event (item->ptr, item->pos, cape_queue__size (self));

        // unset the active state
        cape_thread_item_set (pool_item, FALSE);
    }

    return !self->terminated;
}

//-----------------------------------------------------------------------------

void __STDCALL cape_queue__on_stop_item (void* obj_ptr, void* user_ptr)
{
    CapeQueue self = user_ptr;
    
#if defined(CAPE_USE_FREERTOS)

    xSemaphoreGive (self->sem);
    
#elif defined(__LINUX_OS)
            
    sem_post (&(self->sem));

#elif defined(__BSD_OS)

    dispatch_semaphore_signal (self->sem);

#elif defined(__WINDOWS_OS)

    // increase the count
    if (ReleaseSemaphore (self->semaphore, 1, NULL) == 0)
    {
        cape_log_err (CAPE_LL_ERROR, "CAPE", "QUEUE", "can't permforme stop");
    }

#endif
}

//-----------------------------------------------------------------------------

CapeQueue cape_queue_new (number_t timeout_in_ms)
{
    CapeQueue self = CAPE_NEW (struct CapeQueue_s);
    
    // calculate the deciseconds
    self->timeout_in_ms = timeout_in_ms;
    
    self->terminated = FALSE;
    
    // create the thread pool with the related callbacks
    self->thread_pool = cape_thread_pool_new (self, cape_queue__worker__thread, cape_queue__on_new_item, cape_queue__on_stop_item, cape_queue__item__on_del);

    self->mutex = cape_mutex_new ();

    self->queue = cape_list_new (cape_queue__item__on_del);

#if defined(CAPE_USE_FREERTOS)

    self->sem = xSemaphoreCreateCounting (1000, 0);

#elif defined(__LINUX_OS)
            
    int res = sem_init (&(self->sem), 0, 0);

    if (res == -1)
    {
        cape_log_err (CAPE_LL_ERROR, "CAPE", "QUEUE", "can't initialize semaphore");
    }

#elif defined(__BSD_OS)

    self->sem = dispatch_semaphore_create (0);

#elif defined(__WINDOWS_OS)

    self->semaphore = CreateSemaphore (NULL, 0, 1000, NULL);

#endif
    
    return self;
}

//-----------------------------------------------------------------------------

void cape_queue_del (CapeQueue* p_self)
{
    if (*p_self)
    {
        CapeQueue self = *p_self;
        
        cape_log_fmt (CAPE_LL_TRACE, "CAPE", "queue del", "tear down queue processes");

        self->terminated = TRUE;

        // waits until all threads have been joined
        // destroys all threads
        cape_thread_pool_del (&(self->thread_pool));

        {
            number_t left_queues = cape_list_size (self->queue);

            if (left_queues > 0)
            {
                cape_log_fmt (CAPE_LL_WARN, "CAPE", "queue del", "%i left in queue", left_queues);
            }
        }
    
        cape_list_del (&(self->queue));
        
        cape_mutex_del (&(self->mutex));
        
        //cape_log_fmt (CAPE_LL_TRACE, "CAPE", "queue del", "tear down done");
        
        CAPE_DEL (p_self, struct CapeQueue_s);
    }
}

//-----------------------------------------------------------------------------

int cape_queue_start (CapeQueue self, number_t amount_of_threads, CapeErr err)
{
    cape_thread_pool_start (self->thread_pool, amount_of_threads, self->timeout_in_ms);
  
    return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

void cape_queue_add (CapeQueue self, CapeSync sync, cape_queue_cb_fct on_event, cape_queue_cb_fct on_done, cape_queue_cb_fct on_cancel, void* ptr, number_t pos)
{
    CapeQueueItem item = CAPE_NEW (struct CapeQueueItem_s);
    
    item->on_done = on_done;
    item->on_event = on_event;
    item->on_cancel = on_cancel;
    item->ptr = ptr;
    item->sync = sync;
    item->pos = pos;

    // monitor
    {
        cape_mutex_lock (self->mutex);
        
        cape_list_push_back (self->queue, item);
        
        cape_mutex_unlock (self->mutex);
    }

    cape_sync_inc (sync);

#if defined(__LINUX_OS)

    sem_post (&(self->sem));

#elif defined(__BSD_OS)

    dispatch_semaphore_signal (self->sem);

#elif defined(__WINDOWS_OS)

    // increase the count
    if (ReleaseSemaphore (self->semaphore, 1, NULL) == 0)
    {
        cape_log_err (CAPE_LL_ERROR, "CAPE", "QUEUE", "can't permforme add");
    }

#elif defined(CAPE_USE_FREERTOS)

    xSemaphoreGive (self->sem);
    
#endif
}

//-----------------------------------------------------------------------------
