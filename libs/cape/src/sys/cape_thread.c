#include "cape_thread.h"

// cape includes
#include "sys/cape_types.h"
#include "sys/cape_log.h"

//-----------------------------------------------------------------------------

#if defined __LINUX_OS || defined __BSD_OS

#include <pthread.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>

#include "sys/cape_types.h"

#if defined __BSD_OS

#include <sys/sysctl.h>

#endif

//-----------------------------------------------------------------------------------

struct CapeThread_s
{
  cape_thread_worker_fct fct;
  
  void* ptr;
  
  pthread_t tid;
  
  int status;
  
  cape_thread_on_done on_done;
};

//-----------------------------------------------------------------------------------

static void* cape_thread_run (void* params)
{
  CapeThread self = params;
  
  cape_log_msg (CAPE_LL_TRACE, "CAPE", "thread", "thread created");

  if (self->fct)
  {
    while (self->fct (self->ptr))
    {
      pthread_testcancel();
      
      wait(0);
    }
  }
  
  if (self->on_done)
  {
    self->on_done (self->ptr);
  }

  cape_log_msg (CAPE_LL_TRACE, "CAPE", "thread", "thread terminated");
  
  return NULL;
}

//-----------------------------------------------------------------------------------

CapeThread cape_thread_new (void)
{
  CapeThread self = CAPE_NEW (struct CapeThread_s);
  
  self->fct = NULL;
  self->ptr = NULL;
  //memset(self->tid, 0x00, sizeof(pthread_t));
  
  self->status = FALSE;
  
  self->on_done = NULL;
  
  return self;
}

//-----------------------------------------------------------------------------------

void cape_thread_del (CapeThread* p_self)
{
  if (*p_self)
  {
    CapeThread self = *p_self;
    
    if (self->status)
    {
      // WARNING
      cape_thread_join (self);
    }
    
    CAPE_DEL (p_self, struct CapeThread_s);
  }
}

//-----------------------------------------------------------------------------------

void cape_thread_cancel (CapeThread self)
{
  int errno = pthread_cancel (self->tid);
  
  if (errno)
  {
    CapeErr err = cape_err_new ();
    
    cape_err_lastOSError (err);
    
    cape_log_fmt (CAPE_LL_ERROR, "CAPE", "thread", "can't cancel thread: %s", cape_err_text (err));
    
    cape_err_del (&err);
  }
}

//-----------------------------------------------------------------------------------

void cape_thread_start (CapeThread self, cape_thread_worker_fct fct, void* ptr)
{
  // define some special attributes
  pthread_attr_t attr;
  pthread_attr_init (&attr);
  pthread_attr_setdetachstate (&attr, PTHREAD_CREATE_JOINABLE);
    
  // assign the callback parameters
  self->fct = fct;
  self->ptr = ptr;

  // finally create the thread
  self->status = (pthread_create(&(self->tid), &attr, cape_thread_run, self) == 0);
}

//-----------------------------------------------------------------------------------

void cape_thread_join (CapeThread self)
{
  if (self->status)
  {
    void* status;
    pthread_join(self->tid, &status);
    
    self->status = FALSE;
  }
}

//-----------------------------------------------------------------------------

void cape_thread_sleep (unsigned long milliseconds)
{
  usleep ((useconds_t)(milliseconds * 1000));
}

//-----------------------------------------------------------------------------

void cape_thread_cb (CapeThread self, cape_thread_on_done on_done)
{
  self->on_done = on_done;
}

//-----------------------------------------------------------------------------

void cape_thread_signal (CapeThread self)
{
  pthread_kill (self->tid, SIGUSR1);
}

//-----------------------------------------------------------------------------

void cape_thread_nosignals ()
{
  sigset_t set;
  
  sigemptyset(&set);

  // disable the following signals
  sigaddset(&set, SIGINT);
  sigaddset(&set, SIGTERM);
  
  // for linux
  pthread_sigmask (SIG_BLOCK, &set, NULL);
}

//-----------------------------------------------------------------------------

#elif defined _WIN64 || defined _WIN32

#include <windows.h>

//-----------------------------------------------------------------------------

struct CapeThread_s
{
  HANDLE th;
  
  cape_thread_worker_fct fct;
  
  void* ptr;
};

//-----------------------------------------------------------------------------------

DWORD WINAPI cape_thread_run (LPVOID ptr)
{
  CapeThread self = ptr;
  
  if (self->fct)
  {
    // do the user defined loop
    while (self->fct(self->ptr));
  }
  return 0;
}

//-----------------------------------------------------------------------------------

CapeThread cape_thread_new (void)
{
  CapeThread self = CAPE_NEW (struct CapeThread_s);
  
  self->th = NULL;
  
  return self;
}

//-----------------------------------------------------------------------------------

void cape_thread_del (CapeThread* pself)
{
  CapeThread self = *pself;
  
  cape_thread_join(self);
  
  CAPE_DEL (pself, struct CapeThread_s);
}

//-----------------------------------------------------------------------------------

void cape_thread_start (CapeThread self, cape_thread_worker_fct fct, void* ptr)
{
  if (self->th == NULL)
  {
    self->fct = fct;
    self->ptr = ptr;
    self->th = CreateThread (NULL, 0, (LPTHREAD_START_ROUTINE)cape_thread_run, (LPVOID)self, 0, NULL);
  }
}

//-----------------------------------------------------------------------------------

void cape_thread_join (CapeThread self)
{
  if (self->th != NULL)
  {
    // wait until the thread terminates
    WaitForSingleObject (self->th, INFINITE);
    // release resources
    CloseHandle(self->th);
    self->th = NULL;
  }
}

//-----------------------------------------------------------------------------------

void cape_thread_cancel (CapeThread self)
{
  if (self->th != NULL)
  {
    if (TerminateThread(self->th, 0) == 0)
    {
      CapeErr err = cape_err_new ();
      
      cape_err_lastOSError (err);
      
      cape_log_fmt (CAPE_LL_ERROR, "CAPE", "thread", "can't cancel thread: %s", cape_err_text (err));
      
      cape_err_del (&err);
    }
  }
}

//-----------------------------------------------------------------------------------

void cape_thread_sleep (unsigned long milliseconds)
{
  Sleep (milliseconds);
}

//-----------------------------------------------------------------------------------

void cape_thread_signal (CapeThread self)
{
}

//-----------------------------------------------------------------------------------

void cape_thread_nosignals()
{
}

#endif

//-----------------------------------------------------------------------------

number_t cape_thread_concurrency ()
{
#if defined __LINUX_OS
  
  return sysconf (_SC_NPROCESSORS_ONLN);
  
#elif defined __BSD_OS
  
  int mib[4];
  int number_of_cpus;
  size_t len = sizeof(number_of_cpus);
  
  /* set the mib for hw.ncpu */
  mib[0] = CTL_HW;
  mib[1] = HW_AVAILCPU;  // alternatively, try HW_NCPU;
  
  /* get the number of CPUs from the system */
  sysctl (mib, 2, &number_of_cpus, &len, NULL, 0);
  
  if (number_of_cpus < 1)
  {
    mib[1] = HW_NCPU;
    sysctl (mib, 2, &number_of_cpus, &len, NULL, 0);
    
    if (number_of_cpus < 1)
    {
      number_of_cpus = 1;
    }
  }
  
  return number_of_cpus;
  
#elif defined _WIN64 || defined _WIN32

  SYSTEM_INFO sysinfo;

  GetSystemInfo(&sysinfo);

  return sysinfo.dwNumberOfProcessors;

#else
  
  return 1;
  
#endif
}

//-----------------------------------------------------------------------------

number_t cape_thread_atomic_inc  (number_t* p_var)
{
#if defined(__GNUC__) || defined(__clang__)
    
    return __atomic_fetch_add (p_var, 1, __ATOMIC_SEQ_CST);
  
#elif defined(_MSC_VER)

    return _InterlockedExchangeAdd64 ((volatile __int64*)p_var, 1);

#else

    #error "No atomic implementation available for this compiler"

#endif
}

//-----------------------------------------------------------------------------

number_t cape_thread_atomic_inc__nn (number_t* p_var)
{
#if defined(__GNUC__) || defined(__clang__)
    
    number_t old_value;
    number_t new_value;

    do
    {
        old_value = __atomic_load_n (p_var, __ATOMIC_ACQUIRE);

        if (old_value < 0)
        {
            return old_value;
        }

        new_value = old_value + 1;
    }
    while (!__atomic_compare_exchange_n (p_var, &old_value, new_value, FALSE, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE));

    return old_value;
    
#elif defined(_MSC_VER)

    number_t old_value;
    number_t new_value;

    do
    {
        old_value = _InterlockedCompareExchange64((volatile __int64*)p_var, 0, 0);

        if (old_value < 0)
        {
            return old_value;
        }

        new_value = old_value + 1;

    }
    while (_InterlockedCompareExchange64((volatile __int64*)p_var, (__int64)new_value, (__int64)old_value) != (__int64)old_value);

    return old_value;

#endif
}

//-----------------------------------------------------------------------------

number_t cape_thread_atomic_dec  (number_t* p_var)
{
#if defined(__GNUC__) || defined(__clang__)

    return __atomic_fetch_sub (p_var, 1, __ATOMIC_SEQ_CST);

#elif defined(_MSC_VER)

    return _InterlockedExchangeAdd64 ((volatile __int64*)p_var, -1);
    
#else

    #error "No atomic implementation available for this compiler"

#endif
}

//-----------------------------------------------------------------------------

number_t cape_thread_atomic_dec__nn (number_t* p_var)
{
#if defined(__GNUC__) || defined(__clang__)

    number_t old_value;
    number_t new_value;

    do
    {
        old_value = __atomic_load_n(p_var, __ATOMIC_ACQUIRE);

        if (old_value < 0)
        {
            return old_value;
        }

        new_value = old_value - 1;
    }
    while (!__atomic_compare_exchange_n (p_var, &old_value, new_value, FALSE, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE));

    return old_value;

#elif defined(_MSC_VER)

    number_t old_value;
    number_t new_value;

    do
    {
        old_value = _InterlockedCompareExchange64((volatile __int64*)p_var, 0, 0);

        if (old_value < 0)
        {
            return old_value;
        }

        new_value = old_value - 1;

    }
    while (_InterlockedCompareExchange64 ((volatile __int64*)p_var, (__int64)new_value, (__int64)old_value) != (__int64)old_value);

    return old_value;
    
#endif
}

//-----------------------------------------------------------------------------
