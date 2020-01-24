#include "cape_mutex.h"
#include "sys/cape_types.h"

//-----------------------------------------------------------------------------

#if defined __LINUX_OS || defined __BSD_OS

#include <pthread.h>

//-----------------------------------------------------------------------------

CapeMutex cape_mutex_new (void)
{
  pthread_mutex_t* self = CAPE_NEW(pthread_mutex_t);
  
  memset (self, 0, sizeof(pthread_mutex_t));
  
  pthread_mutex_init (self, NULL);
  
  return self;
}

//-----------------------------------------------------------------------------

void cape_mutex_del (CapeMutex* p_self)
{
  if (*p_self)
  {
    pthread_mutex_t* self = *p_self;
    
    pthread_mutex_destroy (self);
    
    CAPE_DEL(p_self, pthread_mutex_t);  
  }
}

//-----------------------------------------------------------------------------

void cape_mutex_lock (CapeMutex self)
{
  pthread_mutex_lock (self);
}

//-----------------------------------------------------------------------------

void cape_mutex_unlock (CapeMutex self)
{
  pthread_mutex_unlock (self);
}

//-----------------------------------------------------------------------------

#elif defined _WIN64 || defined _WIN32

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
  CRITICAL_SECTION* self = *p_self;
  
  DeleteCriticalSection (self);
  
  CAPE_DEL (p_self, CRITICAL_SECTION);
}

//-----------------------------------------------------------------------------

void cape_mutex_lock (CapeMutex self)
{
  EnterCriticalSection (self);
}

//-----------------------------------------------------------------------------

void cape_mutex_unlock (CapeMutex self)
{
  LeaveCriticalSection (self);
}

//-----------------------------------------------------------------------------

#endif
