#include "cape_lock.h"
#include "sys/cape_types.h"
#include "sys/cape_mutex.h"
#include "sys/cape_log.h"

#if defined __WINDOWS_OS

#include <windows.h>

#else

#include <sys/sem.h>

#endif

//-----------------------------------------------------------------------------

struct CapeLockNode_s
{
  #if defined __WINDOWS_OS

    LONG refcnt;

    HANDLE revent;

  #else

    int semid;

  #endif

}; typedef struct CapeLockNode_s* CapeLockNode;

//-----------------------------------------------------------------------------

void __STDCALL cape_lock__locks__del (void* key, void* val)
{
  
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
      ln = CAPE_NEW (struct CapeLockNode_s);
      
      #if defined __WINDOWS_OS

        ln->refcnt = 0;
        ln->revent = CreateEvent (NULL, FALSE, TRUE, NULL);

      #else

        ln->semid = semget (IPC_PRIVATE, 1, IPC_CREAT  | IPC_EXCL | 0666);
        
        if (ln->semid == -1)
        {
          CapeErr err = cape_err_new ();
          
          cape_err_lastOSError (err);
          
          cape_log_fmt (CAPE_LL_ERROR, "CAPE", "sync del", "can't permforme semaphore action: %s", cape_err_text(err));
          
          cape_err_del (&err);
        }
        
        semctl (ln->semid, 0, SETVAL, 0);

      #endif
      
      ret = cape_map_insert (self->locks, (void*)name, ln);
    }
  }
  
  cape_mutex_unlock (self->mutex);
  
  // use the CapeLockNode to lock
  
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
  // -> if it is ZERO delete
  
  //cape_map_erase (self->locks, n);
  
  cape_mutex_unlock (self->mutex);
}

//-----------------------------------------------------------------------------
