#include "cape_mutex.h"
#include "sys/cape_types.h"
#include "sys/cape_log.h"

//-----------------------------------------------------------------------------

#if defined(__LINUX_OS) || defined(__BSD_OS)

#include <pthread.h>

//-----------------------------------------------------------------------------

CapeMutex cape_mutex_new (void)
{
    pthread_mutex_t* self = CAPE_NEW(pthread_mutex_t);
    
    memset (self, 0, sizeof(pthread_mutex_t));
    
    if (pthread_mutex_init (self, NULL) != 0)
    {
        cape_log_msg (CAPE_LL_ERROR, "CAPE", "MUTEX", "failed to initialize mutex");

        CAPE_DEL (&self, pthread_mutex_t);
        return NULL;
    }
    
    return self;
}

//-----------------------------------------------------------------------------

void cape_mutex_del (CapeMutex* p_self)
{
    if (p_self && *p_self)
    {
        pthread_mutex_t* self = *p_self;
        
        pthread_mutex_destroy (self);
        
        CAPE_DEL(p_self, pthread_mutex_t);
    }
}

//-----------------------------------------------------------------------------

void cape_mutex_lock (CapeMutex self)
{
    if (!self)
    {
        cape_log_msg (CAPE_LL_ERROR, "CAPE", "MUTEX", "object was not allocated yet");
        return;
    }
    
    if (pthread_mutex_lock (self) != 0)
    {
        cape_log_msg (CAPE_LL_ERROR, "CAPE", "MUTEX", "failed to lock mutex");
        return;
    }
}

//-----------------------------------------------------------------------------

void cape_mutex_unlock (CapeMutex self)
{
    if (!self)
    {
        cape_log_msg (CAPE_LL_ERROR, "CAPE", "MUTEX", "object was not allocated yet");
        return;
    }

    if (pthread_mutex_unlock (self) != 0)
    {
        cape_log_msg (CAPE_LL_ERROR, "CAPE", "MUTEX", "failed to unlock mutex");
        return;
    }
}

//-----------------------------------------------------------------------------

#elif defined(__WINDOWS_OS)

#include <windows.h>

//-----------------------------------------------------------------------------

CapeMutex cape_mutex_new (void)
{
    CRITICAL_SECTION* self = CAPE_NEW (CRITICAL_SECTION);
    
    InitializeCriticalSection (self);
    
    return self;
}

//-----------------------------------------------------------------------------

void cape_mutex_del (CapeMutex* p_self)
{
    if (p_self && *p_self)
    {
        CRITICAL_SECTION* self = *p_self;
        
        DeleteCriticalSection (self);
        
        CAPE_DEL (p_self, CRITICAL_SECTION);
    }
}

//-----------------------------------------------------------------------------

void cape_mutex_lock (CapeMutex self)
{
    if (!self)
    {
        cape_log_msg (CAPE_LL_ERROR, "CAPE", "MUTEX", "object was not allocated yet");
        return;
    }

    EnterCriticalSection (self);
}

//-----------------------------------------------------------------------------

void cape_mutex_unlock (CapeMutex self)
{
    if (!self)
    {
        cape_log_msg (CAPE_LL_ERROR, "CAPE", "MUTEX", "object was not allocated yet");
        return;
    }

    LeaveCriticalSection (self);
}

//-----------------------------------------------------------------------------

#elif defined(CAPE_USE_FREERTOS)

//-----------------------------------------------------------------------------

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

//-----------------------------------------------------------------------------

CapeMutex cape_mutex_new (void)
{
    SemaphoreHandle_t self = xSemaphoreCreateMutex ();
  
    if (!self)
    {
        cape_log_msg (CAPE_LL_ERROR, "CAPE", "MUTEX", "failed to create mutex");

        configASSERT (FALSE);
    }

    return self;
}

//-----------------------------------------------------------------------------

void cape_mutex_del (CapeMutex* p_self)
{
    if (p_self && *p_self)
    {
        SemaphoreHandle_t self = *p_self;
      
        vSemaphoreDelete (self);

        *p_self = NULL;
    }
}

//-----------------------------------------------------------------------------

void cape_mutex_lock (CapeMutex self)
{
    if (!self)
    {
        cape_log_msg (CAPE_LL_ERROR, "CAPE", "MUTEX", "object was not allocated yet");

        // exit
        configASSERT (FALSE);
        return;
    }
    
    // wait for lock maximum 5 seconds
    if (pdTRUE != xSemaphoreTake (self, pdMS_TO_TICKS (5000)))
    {
        // timeout
        cape_log_msg (CAPE_LL_ERROR, "CAPE", "MUTEX", "timeout on mutex lock");
        
        configASSERT (FALSE);
        return;
    }
}

//-----------------------------------------------------------------------------

void cape_mutex_unlock (CapeMutex self)
{
    if (!self)
    {
        cape_log_msg (CAPE_LL_ERROR, "CAPE", "MUTEX", "object was not allocated yet");
        
        // exit
        configASSERT (FALSE);
        return;
    }

    if (pdTRUE != xSemaphoreGive (self))
    {
        cape_log_msg (CAPE_LL_ERROR, "CAPE", "MUTEX", "failed to release mutex");

        configASSERT (FALSE);
        return;
    }
}

//-----------------------------------------------------------------------------

#endif
