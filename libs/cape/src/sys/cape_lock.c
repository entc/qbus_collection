#include "cape_lock.h"
#include "sys/cape_types.h"
#include "sys/cape_mutex.h"
#include "sys/cape_log.h"

typedef struct CapeLockNode_s* CapeLockNode;

#if defined(__LINUX_OS) || defined(__BSD_OS)

#include <sys/sem.h>

struct CapeLockNode_s
{
    int semid;
};

#elif defined(__WINDOWS_OS)

#include <windows.h>

struct CapeLockNode_s
{
    LONG refcnt;
    HANDLE revent;
};

#elif defined(CAPE_USE_FREERTOS)

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

struct CapeLockNode_s
{
    SemaphoreHandle_t sem;
};

#endif

//-----------------------------------------------------------------------------

static CapeLockNode cape_lock_node_new ()
{
    CapeLockNode self = CAPE_NEW (struct CapeLockNode_s);

#if defined(__LINUX_OS) || defined(__BSD_OS)

    self->semid = semget (IPC_PRIVATE, 1, IPC_CREAT  | IPC_EXCL | 0666);
    
    if (self->semid == -1)
    {
        CapeErr err = cape_err_new ();
        
        cape_err_lastOSError (err);
        
        cape_log_fmt (CAPE_LL_ERROR, "CAPE", "LOCK", "can't permforme semaphore action: %s", cape_err_text(err));
        
        cape_err_del (&err);
    }
    else
    {
        semctl (self->semid, 0, SETVAL, 0);
    }

#elif defined(__WINDOWS_OS)

    self->refcnt = 0;
    self->revent = CreateEvent (NULL, FALSE, TRUE, NULL);

#elif defined(CAPE_USE_FREERTOS)

    self->sem = xSemaphoreCreateBinary ();

    if (self->sem == NULL)
    {
        cape_log_fmt (CAPE_LL_ERROR, "CAPE", "LOCK", "can't create semaphore");
    }
    else
    {
        xSemaphoreGive (self->sem);
    }

#endif

    return self;
}

//-----------------------------------------------------------------------------

static void cape_lock_node_del (CapeLockNode* p_self)
{
    if (*p_self)
    {
        CapeLockNode self = *p_self;
        
#if defined(__LINUX_OS) || defined(__BSD_OS)

        if (self->semid != -1)
        {
            semctl (self->semid, 0, IPC_RMID);
        }

#elif defined(__WINDOWS_OS)

        if (self->revent)
        {
            CloseHandle (self->revent);
        }

#elif defined(CAPE_USE_FREERTOS)

        if (self->sem)
        {
            vSemaphoreDelete (self->sem);
        }

#endif

        CAPE_DEL (p_self, struct CapeLockNode_s);
    }
}

//-----------------------------------------------------------------------------

static void cape_lock_node_lock (CapeLockNode self)
{
#if defined(__LINUX_OS) || defined(__BSD_OS)

    struct sembuf sb;

    sb.sem_num = 0;
    sb.sem_op = -1;
    sb.sem_flg = 0;

    if (semop (self->semid, &sb, 1) == -1)
    {
        CapeErr err = cape_err_new ();

        cape_err_lastOSError (err);

        cape_log_fmt (CAPE_LL_ERROR, "CAPE", "LOCK", "can't acquire semaphore: %s", cape_err_text (err));

        cape_err_del (&err);
    }
    
#elif defined(__WINDOWS_OS)

    WaitForSingleObject (self->revent, INFINITE);
    
#elif defined(CAPE_USE_FREERTOS)

    if (self->sem)
    {
        if (xSemaphoreTake (self->sem, pdMS_TO_TICKS (5000)) != pdTRUE)
        {
            cape_log_msg (CAPE_LL_ERROR, "CAPE", "LOCK", "can't acquire semaphore");
        }
    }

#endif
}

//-----------------------------------------------------------------------------

static void cape_lock_node_unlock (CapeLockNode self)
{
#if defined(__LINUX_OS) || defined(__BSD_OS)

    struct sembuf sb;

    sb.sem_num = 0;
    sb.sem_op = 1;
    sb.sem_flg = 0;

    if (semop (self->semid, &sb, 1) == -1)
    {
        CapeErr err = cape_err_new ();

        cape_err_lastOSError (err);

        cape_log_fmt (CAPE_LL_ERROR, "CAPE", "LOCK", "can't release semaphore: %s", cape_err_text (err));

        cape_err_del (&err);
    }
    
#elif defined(__WINDOWS_OS)

    SetEvent (self->revent);

#elif defined(CAPE_USE_FREERTOS)

    if (self->sem)
    {
        xSemaphoreGive (self->sem);
    }

#endif
}

//-----------------------------------------------------------------------------

static void __STDCALL cape_lock__locks__del (void* key, void* val)
{
    {
        CapeString h = key; cape_str_del (&h);
    }
    {
        CapeLockNode h = (CapeLockNode)val; cape_lock_node_del (&h);
    }
}

//-----------------------------------------------------------------------------

struct CapeLock_s
{
    CapeMutex mutex;
    CapeMap locks;
};

//-----------------------------------------------------------------------------

CapeLock cape_lock_new (void)
{
    CapeLock self = CAPE_NEW (struct CapeLock_s);
    
    self->mutex = cape_mutex_new ();
    self->locks = cape_map_new (NULL, cape_lock__locks__del, NULL);
    
    return self;
}

//-----------------------------------------------------------------------------

void cape_lock_del (CapeLock* p_self)
{
    if (*p_self)
    {
        CapeLock self = *p_self;
        
        cape_mutex_del (&(self->mutex));
        cape_map_del (&(self->locks));
        
        CAPE_DEL(p_self, struct CapeLock_s);
    }
}

//-----------------------------------------------------------------------------

CapeMapNode cape_lock_get_s (CapeLock self, const CapeString name)
{
    CapeMapNode ret = NULL;
    CapeLockNode ln = NULL;
    
    cape_mutex_lock (self->mutex);

    {
        CapeMapNode n = cape_map_find (self->locks, (void*)name);
        if (n)
        {
            ln = cape_map_node_value (n);
            
            ret = n;
        }
        else
        {
            ln = cape_lock_node_new ();
            
            ret = cape_map_insert (self->locks, (void*)cape_str_cp (name), ln);
        }
    }

    cape_mutex_unlock (self->mutex);
  
    // use the CapeLockNode to lock
    cape_lock_node_lock (ln);
  
    return ret;
}

//-----------------------------------------------------------------------------

void cape_lock_get_fmt (CapeLock self, const CapeString format, ...)
{
  
}

//-----------------------------------------------------------------------------

void cape_lock_release (CapeLock self, CapeMapNode n)
{
    CapeLockNode ln = NULL;

    cape_mutex_lock (self->mutex);

    ln = cape_map_node_value (n);
    
    // unlock and check if it is ZERO ?
    cape_lock_node_unlock (ln);
    
    // -> if it is ZERO delete
    
    //cape_map_erase (self->locks, n);
    
    cape_mutex_unlock (self->mutex);
}

//-----------------------------------------------------------------------------
