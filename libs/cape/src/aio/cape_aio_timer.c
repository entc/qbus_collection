#include "cape_aio_timer.h"

#include "sys/cape_types.h"
#include "sys/cape_log.h"

//*****************************************************************************

#if defined __BSD_OS || defined __LINUX_OS

//-----------------------------------------------------------------------------

#if defined __BSD_OS

#include <sys/event.h>

#elif defined __LINUX_OS

#include <sys/timerfd.h>

#endif

//-----------------------------------------------------------------------------

#include <memory.h>
#include <unistd.h>

//-----------------------------------------------------------------------------

struct CapeAioTimer_s
{
  CapeAioHandle aioh;
  
  void* ptr;
  
  fct_cape_aio_timer_onEvent onEvent;
  
#if defined __BSD_OS

  number_t timeout;
  
#elif defined __LINUX_OS

  number_t handle;
  
#endif
};

//-----------------------------------------------------------------------------

CapeAioTimer cape_aio_timer_new (void* handle)
{
  CapeAioTimer self = CAPE_NEW(struct CapeAioTimer_s);
  
#if defined __BSD_OS

  self->timeout = 0;

#elif defined __LINUX_OS

  self->handle = 0;

#endif
  
  self->ptr = NULL;
  self->onEvent = NULL;
  
  self->aioh = NULL;
  
  return self;
}

//-----------------------------------------------------------------------------

int cape_aio_timer_set (CapeAioTimer self, long timeoutInMs, void* ptr, fct_cape_aio_timer_onEvent fct, CapeErr err)
{
  
#if defined __BSD_OS

  self->timeout = timeoutInMs;
  
#elif defined __LINUX_OS
  
  self->handle = timerfd_create (CLOCK_MONOTONIC, TFD_NONBLOCK);
  
  if (self->handle == -1)
  {
    return cape_err_lastOSError (err);
  }
  
  // set timer interval
  {
    struct timespec ts;

    struct itimerspec newValue;
    struct itimerspec oldValue;
    
    memset (&newValue, 0, sizeof(newValue));  
    memset (&oldValue, 0, sizeof(oldValue));

    // convert from milliseconds to timespec
    ts.tv_sec   =  timeoutInMs / 1000;
    ts.tv_nsec  = (timeoutInMs % 1000) * 1000000;
    
    newValue.it_value = ts; 
    newValue.it_interval = ts;
    
    if (timerfd_settime (self->handle, 0, &newValue, &oldValue) < 0)
    {
      return cape_err_lastOSError (err);
    }
  }
  
#endif

  self->ptr = ptr;
  self->onEvent = fct;
  
  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

static int __STDCALL cape_aio_timer_onEvent (void* ptr, int hflags, unsigned long events, unsigned long param1)
{
  int res = TRUE;
  
  CapeAioTimer self = ptr;
  
#if defined __LINUX_OS

  {
    long value;
    read (self->handle, &value, 8);
  }
  
#endif
  
  if (self->onEvent)
  {
    res = self->onEvent (self->ptr);
  }
  
#if defined __LINUX_OS

  return res ? CAPE_AIO_READ : CAPE_AIO_DONE;
  
#elif defined __BSD_OS

  return res ? CAPE_AIO_TIMER : CAPE_AIO_DONE;

#endif
}

//-----------------------------------------------------------------------------

static void __STDCALL cape_aio_timer_onUnref (void* ptr, CapeAioHandle aioh, int force_close)
{
  CapeAioTimer self = ptr;
  
#if defined __LINUX_OS

  close (self->handle);

#endif
  
  // delete the AIO handle
  cape_aio_handle_del (&(self->aioh));
  
  CAPE_DEL (&self, struct CapeAioTimer_s);
}

//-----------------------------------------------------------------------------

int cape_aio_timer_add (CapeAioTimer* p_self, CapeAioContext aio)
{
  CapeAioTimer self = *p_self;
  
  *p_self = NULL;
  
#if defined __BSD_OS

  self->aioh = cape_aio_handle_new (CAPE_AIO_TIMER, self, cape_aio_timer_onEvent, cape_aio_timer_onUnref);

  cape_aio_context_add (aio, self->aioh, self, self->timeout);

#else
  
  self->aioh = cape_aio_handle_new (CAPE_AIO_READ, self, cape_aio_timer_onEvent, cape_aio_timer_onUnref);
  
  cape_aio_context_add (aio, self->aioh, (void*)self->handle, 0);

#endif
  
  return 0;
}

//*****************************************************************************

#elif defined __WINDOWS_OS

#include <Windows.h>
#include <stdio.h>

//-----------------------------------------------------------------------------

struct CapeAioTimer_s
{
  // the handle to the device descriptor
  CapeAioHandle aioh;
  
  number_t timeout_in_ms;

  // *** callback ***

  void* ptr;
  fct_cape_aio_timer_onEvent onEvent;

  HANDLE htimer;
};

//-----------------------------------------------------------------------------

CapeAioTimer cape_aio_timer_new ()
{
  CapeAioTimer self = CAPE_NEW (struct CapeAioTimer_s);

  self->ptr = NULL;
  self->onEvent = NULL;
  
  self->timeout_in_ms = 0;
  self->htimer = NULL;
  
  return self;
}

//-----------------------------------------------------------------------------

#define _MSECOND 10000

//-----------------------------------------------------------------------------

void cape_aio_timer__timeout (CapeAioTimer self, LARGE_INTEGER* timeout)
{
  // use negative time for relative time period
  __int64 qwDueTime = -(self->timeout_in_ms) * _MSECOND;

  // copy the relative time into a LARGE_INTEGER.
  timeout->LowPart = (DWORD)(qwDueTime & 0xFFFFFFFF);
  timeout->HighPart = (LONG)(qwDueTime >> 32);
}

//-----------------------------------------------------------------------------

VOID CALLBACK cape_aio_timer__on_event(LPVOID lpArg, DWORD dwTimerLowValue, DWORD dwTimerHighValue);

//-----------------------------------------------------------------------------

void cape_aio_timer__set (CapeAioTimer self)
{
  LARGE_INTEGER timeout;
  cape_aio_timer__timeout(self, &timeout);

  if (FALSE == SetWaitableTimer (self->htimer, &timeout, 0, cape_aio_timer__on_event, self, FALSE))
  {
    CapeErr err = cape_err_new();

    cape_err_lastOSError(err);

    cape_log_fmt(CAPE_LL_ERROR, "CAPE", "timer set", "can't set timer: %s", cape_err_text(err));

    cape_err_del(&err);
  }
  else
  {
    cape_log_msg(CAPE_LL_TRACE, "cape", "aio timer", "timer in APC was registered");
  }
}

//-----------------------------------------------------------------------------

VOID CALLBACK cape_aio_timer__on_event (LPVOID lpArg, DWORD dwTimerLowValue, DWORD dwTimerHighValue)
{
  int res = FALSE;
  CapeAioTimer self = (CapeAioTimer)lpArg;

  if (self->onEvent)
  {
    res = self->onEvent (self->ptr);
  }

  if (res)
  {
    cape_aio_timer__set (self);
  }
  else
  {
    // remove the timer
    if (FALSE == CancelWaitableTimer(self->htimer))
    {


    }
  }
}

//-----------------------------------------------------------------------------

int __STDCALL cape_aio_timer__on_register (void* ptr, int hflags, unsigned long events, unsigned long extra)
{
  CapeAioTimer self = ptr;

  cape_aio_timer__set (self);

  return CAPE_AIO_DONE;
}

//-----------------------------------------------------------------------------

int cape_aio_timer_add (CapeAioTimer* p_self, CapeAioContext aio)
{
  CapeAioTimer self = *p_self;

  // create a user event in the IOCP to trigger a callback
  // in the callback run the SetWaitableTimer
  // because the APC uses the current thread for wait operations
  // when the system gets into a status of 'WAIT' the APC callback will be triggered
  // wait for IOCP is considered as 'WAIT' operation

  // TODO: add a done method
  cape_aio_context_tcb (aio, self, cape_aio_timer__on_register);

  *p_self = NULL;
  return 0;
}

//-----------------------------------------------------------------------------

int cape_aio_timer_set (CapeAioTimer self, long inMs, void* ptr, fct_cape_aio_timer_onEvent on_event, CapeErr err)
{
  self->timeout_in_ms = inMs;
  self->onEvent = on_event;
  self->ptr = ptr;

  // allocate the HANDLE
  self->htimer = CreateWaitableTimer (NULL, FALSE, TEXT("cape_timer")); 
  if (self->htimer == NULL)
  {
    cape_err_lastOSError (err);

    cape_log_fmt (CAPE_LL_ERROR, "CAPE", "timer set", "can't create timer handle: %s", cape_err_text (err));

    return cape_err_code (err);
  }

  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

#endif

//-----------------------------------------------------------------------------
