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
  pthread_cancel (self->tid);
}

//-----------------------------------------------------------------------------------

void cape_thread_start (CapeThread self, cape_thread_worker_fct fct, void* ptr)
{
  // define some special attributes
  pthread_attr_t attr;
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
  // assign the callback parameters
  self->fct = fct;
  self->ptr = ptr;
  // finally create the thread
  self->status = (pthread_create(&(self->tid), &attr, cape_thread_run, self) == 0);
}

//-----------------------------------------------------------------------------------

void cape_thread_join (CapeThread self)
{
  if (self->status) {
    void* status;
    pthread_join(self->tid, &status);
    
    self->status = FALSE;
  }
}

//-----------------------------------------------------------------------------

void cape_thread_sleep (unsigned long milliseconds)
{
  usleep (milliseconds * 1000);
}

//-----------------------------------------------------------------------------

void cape_thread_cb (CapeThread self, cape_thread_on_done on_done)
{
  self->on_done = on_done;
}

//-----------------------------------------------------------------------------

void cape_thread_nosignals ()
{
  sigset_t set;
  
  sigemptyset(&set);

  // disable the following signals
  sigaddset(&set, SIGINT);
  sigaddset(&set, SIGKILL);
  
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

