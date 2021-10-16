#include "cape_aio_ctx.h" 

// CAPE includes
#include "sys/cape_types.h"
#include "sys/cape_err.h"
#include "stc/cape_list.h"
#include "sys/cape_log.h"

//*****************************************************************************

#if defined __BSD_OS || defined __LINUX_OS

#if defined __BSD_OS

#include <sys/event.h>

#else

#include <sys/epoll.h>
#include <sys/signalfd.h>

#define CAPE_AIO_EPOLL_MAXEVENTS 1

#endif

#include <pthread.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>

//-----------------------------------------------------------------------------

struct CapeAioHandle_s
{
  void* ptr;
  
  int hflags;
  
  fct_cape_aio_onUnref on_unref;
  
  // callback method for any kind of event
  fct_cape_aio_onEvent on_event;
  
#if defined __BSD_OS

  int option;
  
#endif
  
#if defined __LINUX_OS
  
  // epoll don't commit the file descriptor :-(
  // -> so we need to store it somewhere
  void* handle;
  
#endif
};

//-----------------------------------------------------------------------------

CapeAioHandle cape_aio_handle_new (int hflags, void* ptr, fct_cape_aio_onEvent on_event, fct_cape_aio_onUnref on_unref)
{
  CapeAioHandle self = CAPE_NEW (struct CapeAioHandle_s);
  
#if defined __BSD_OS

  self->option = 0;
  
#endif

#if defined __LINUX_OS

  self->handle = NULL;   // will be set later

#endif
  
  self->ptr = ptr;
  
  self->on_event = on_event;
  self->on_unref = on_unref;
  
  self->hflags = hflags;
  
  return self;
}

//-----------------------------------------------------------------------------

void cape_aio_handle_del (CapeAioHandle* p_self)
{
  if (*p_self)
  {
    CAPE_DEL (p_self, struct CapeAioHandle_s);
  }
}

//-----------------------------------------------------------------------------

struct CapeAioContext_s
{
  
#if defined __BSD_OS
  
  int kq;               // kevent file descriptor

#else
  
  int efd;              // epoll file descriptor
  
  long sfd;             // eventfd file descriptor
  
  int smap[32];         // map for signal handling
  
#endif
  
  CapeList events;      // store all events into this list (used only for destruction)
  
  pthread_mutex_t mutex;
};

//-----------------------------------------------------------------------------

void cape_aio_handle_unref (CapeAioHandle self)
{
  if (self->on_unref)
  {
    // call the callback to signal the destruction of the handle
    self->on_unref (self->ptr, self, FALSE);
  }
}

//-----------------------------------------------------------------------------

void cape_aio_handle_close (CapeAioHandle self)
{
  //cape_log_fmt (CAPE_LL_TRACE, "CAPE", "aio_ctx rm", "close handle: %p", self->hfd);
  
  if (self->on_unref)
  {
    // call the callback to signal the destruction of the handle
    self->on_unref (self->ptr, self, TRUE);
  }
}

//-----------------------------------------------------------------------------

static void __STDCALL cape_aio_context_events_onDestroy (void* ptr)
{
  cape_aio_handle_unref (ptr);
}

//-----------------------------------------------------------------------------

CapeAioContext cape_aio_context_new (void)
{
  CapeAioContext self = CAPE_NEW (struct CapeAioContext_s);
  
  memset (&(self->mutex), 0, sizeof(pthread_mutex_t));
  
  pthread_mutex_init (&(self->mutex), NULL);
  
#if defined __BSD_OS

  self->kq = -1;

#else

  self->efd = -1;
  self->sfd = -1;

  {
    int i;
    
    for (i = 0; i < 32; i++)
    {
      self->smap[i] = 0;
    }
  }
  
#endif
    
  self->events = cape_list_new (cape_aio_context_events_onDestroy);
  
  return self;
}

//-----------------------------------------------------------------------------

void cape_aio_context_closeAll (CapeAioContext self)
{
  
#if defined __BSD_OS

  if (self->kq >= 0)
  {
    close (self->kq);
    self->kq = -1;
  }
  
#else
  
  if (self->efd >= 0)
  {
    close (self->efd);
    
    self->efd = -1;
  }

#endif
  
  cape_log_msg (CAPE_LL_TRACE, "CAPE", "aio close", "start closing all handles");

  pthread_mutex_lock(&(self->mutex));
  
  if (self->events)
  {
    cape_list_del (&(self->events));
  }
  
  pthread_mutex_unlock(&(self->mutex));
  
  cape_log_msg (CAPE_LL_TRACE, "CAPE", "aio close", "all handles were closed");
}

//-----------------------------------------------------------------------------

void cape_aio_context_del (CapeAioContext* p_self)
{
  if (*p_self)
  {
    CapeAioContext self = *p_self;
    
    cape_aio_context_closeAll (self);
    
    pthread_mutex_destroy (&(self->mutex));
    
    CAPE_DEL (p_self, struct CapeAioContext_s);
  }
}

//-----------------------------------------------------------------------------

int cape_aio_context_open (CapeAioContext self, CapeErr err)
{
#if defined __BSD_OS

  // create a new kevent
  self->kq = kqueue ();
  
  // check if the open was successful
  if (self->kq == -1)
  {
    return cape_err_lastOSError (err);
  }

#else
  
  // create a new epoll
  self->efd = epoll_create1 (0);
  
  // check if the open was successful
  if (self->efd == -1)
  {
    return cape_err_lastOSError (err);
  }

#endif
  
  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

int cape_aio_context_close (CapeAioContext self, CapeErr err)
{
  // create an user event
  //return cape_aio_context_add (self, NULL, 0, self, cape_aio_context_close_onEvent, NULL);
  
  //printf ("send internal SIGTERM\n");
  
  
  //close (self->efd);
  
  kill (getpid(), SIGTERM);
  
  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

int cape_aio_context_wait (CapeAioContext self, CapeErr err)
{
  while (cape_aio_context_next (self, -1, err) == CAPE_ERR_NONE);
  
  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

void cape_aio_remove_handle (CapeAioContext self, CapeAioHandle hobj)
{
  void* ptr = NULL;
  
  // enter monitor
  pthread_mutex_lock (&(self->mutex));
  
  // try to find the handle (expensive)
  // TODO: we might want to use a map for storing the handles
  {
    CapeListCursor cursor; cape_list_cursor_init (self->events, &cursor, CAPE_DIRECTION_FORW);
    
    while (cape_list_cursor_next (&cursor))
    {
      CapeAioHandle h = cape_list_node_data (cursor.node);
      
      if (h == hobj)
      {
        // extract the ptr, otherwise a deadlock can apear
        // if in the on_del method another handle will be added to the AIO system
        ptr = cape_list_node_extract (self->events, cursor.node);
        
        goto exit_and_unlock;
      }
    }
  }
  
exit_and_unlock:
  
  // leave monitor
  pthread_mutex_unlock (&(self->mutex));
  
  if (ptr)
  {
    // clsoe handle
    cape_aio_handle_close (ptr);
  }
}

//-----------------------------------------------------------------------------

#if defined __BSD_OS

//-----------------------------------------------------------------------------

int cape_aio_set_kevent (CapeAioContext self, CapeAioHandle aioh, void* handle, int hflags, int flags)
{
  int filter_cnt = 0;
  
  if (hflags & CAPE_AIO_READ)
  {
    filter_cnt++;
  }
    
  if (hflags & CAPE_AIO_WRITE)
  {
    filter_cnt++;
  }

  if (hflags & CAPE_AIO_TIMER)
  {
    filter_cnt++;
  }

  if (filter_cnt)
  {
    int res;
    int i = 0;
    
    struct kevent* kevs = CAPE_ALLOC (sizeof(struct kevent) * filter_cnt);
    memset (kevs, 0x0, sizeof(struct kevent) * filter_cnt);
    
    if (hflags & CAPE_AIO_READ)
    {
      EV_SET (&(kevs[i]), (number_t)handle, EVFILT_READ, flags, 0, aioh->option, aioh);
      i++;

      //cape_log_fmt (CAPE_LL_TRACE, "CAPE", "aio", "set filter to READ");
    }
      
    if (hflags & CAPE_AIO_WRITE)
    {
      EV_SET (&(kevs[i]), (number_t)handle, EVFILT_WRITE, flags, 0, aioh->option, aioh);
      i++;

      //cape_log_fmt (CAPE_LL_TRACE, "CAPE", "aio", "set filter to WRITE");
    }

    if (hflags & CAPE_AIO_TIMER)
    {
      EV_SET (&(kevs[i]), (number_t)handle, EVFILT_TIMER, flags, 0, aioh->option, aioh);

      //cape_log_fmt (CAPE_LL_TRACE, "CAPE", "aio", "set filter to TIMER");
    }

    res = kevent (self->kq, kevs, filter_cnt, NULL, 0, NULL);
    
    CAPE_FREE (kevs);
    
    if (res < 0)
    {
      CapeErr err = cape_err_new ();
      
      cape_err_lastOSError (err);
      
      cape_log_fmt (CAPE_LL_WARN, "CAPE", "aio add", cape_err_text(err));
      
      cape_err_del (&err);
      
      return FALSE;
    }
  }

  return TRUE;
}

//-----------------------------------------------------------------------------

int cape_aio_add_event (CapeAioContext self, CapeAioHandle aioh, void* handle, number_t option)
{
  // important to set the option
  // -> this value is needed for updating later
  aioh->option = option;
  
  return cape_aio_set_kevent (self, aioh, handle, aioh->hflags, EV_ADD | EV_ENABLE | EV_ONESHOT);
}

//-----------------------------------------------------------------------------

void cape_aio_delete_event (CapeAioContext self, CapeAioHandle aioh, void* handle)
{
  cape_aio_set_kevent (self, aioh, handle, aioh->hflags, EV_DELETE);
}

//-----------------------------------------------------------------------------

void cape_aio_update_event (CapeAioContext self, CapeAioHandle aioh, void* handle, number_t option)
{
  // important to set the option
  // -> this value is needed for updating later
  aioh->option = option;

  cape_aio_set_kevent (self, aioh, handle, aioh->hflags, EV_ADD | EV_ENABLE | EV_ONESHOT);
}

//-----------------------------------------------------------------------------

#else

//-----------------------------------------------------------------------------

void cape_aio_update_events (struct epoll_event* event, int hflags)
{
  
  event->events = EPOLLET | EPOLLONESHOT;
  
  if (hflags & CAPE_AIO_READ)
  {
    //cape_log_msg (CAPE_LL_TRACE, "CAPE", "update events", "set READ");
    event->events |= EPOLLIN;
  }
  
  if (hflags & CAPE_AIO_WRITE)
  {
    //cape_log_msg (CAPE_LL_TRACE, "CAPE", "update events", "set WRITE");
    event->events |= EPOLLOUT;
  }
  
  if (hflags & CAPE_AIO_ALIVE)
  {
    //cape_log_msg (CAPE_LL_TRACE, "CAPE", "update events", "set ALIVE");
    event->events |= EPOLLHUP;
  }
 
  if (hflags & CAPE_AIO_ERROR)
  {
    //cape_log_msg (CAPE_LL_TRACE, "CAPE", "update events", "set ERROR");
    event->events |= EPOLLERR;
  }
}

//-----------------------------------------------------------------------------

int cape_aio_context_sigmask (CapeAioContext self, sigset_t* sigset)
{
  int res, i;
  
  // null the sigset
  res = sigemptyset (sigset);
  
  for (i = 0; i < 32; i++)
  {
    if (self->smap[i])
    {
      // add this signal to the sigset
      res = sigaddset (sigset, i);
    }
  }
  
  return 0;
}

#endif

//-----------------------------------------------------------------------------

int cape_aio_context_next (CapeAioContext self, long timeout_in_ms, CapeErr err)
{
#if defined __BSD_OS

  int res;
  struct timespec tmout;
  
  struct kevent event;
  memset (&event, 0x0, sizeof(struct kevent));
  
  if (timeout_in_ms == -1)
  {
    res = kevent (self->kq, NULL, 0, &event, 1, NULL);
  }
  else
  {
    tmout.tv_sec = timeout_in_ms / 1000;
    tmout.tv_nsec = (timeout_in_ms % 1000) * 1000;
    
    res = kevent (self->kq, NULL, 0, &event, 1, &tmout);
  }
  
  //cape_log_fmt (CAPE_LL_TRACE, "CAPE", "aio", "event %i", res);
  
  if( res == -1 )
  {
    if (errno == EINTR)
    {
      return CAPE_ERR_NONE;
    }
    
    return cape_err_lastOSError (err);
  }
  else if (event.flags & EV_ERROR)
  {
    return cape_err_lastOSError (err);
  }
  else if (res == 0)
  {
    return CAPE_ERR_NONE;  // timeout
  }
  else
  {
    // retrieve the handle object from the userdata of the epoll event
    CapeAioHandle hobj = event.udata;
    if (hobj)
    {
      number_t hflags_result;
      
      if (hobj->on_event)
      {
        hflags_result = hobj->on_event (hobj->ptr, hobj->hflags, event.flags, 0);
      }
      else
      {
        hflags_result = 0;
      }
      
      if (hflags_result & CAPE_AIO_DONE)
      {
        // remove the event from the kqueue
        cape_aio_delete_event (self, hobj, (void*)event.ident);

        // remove the handle from events
        cape_aio_remove_handle (self, hobj);

        if (hflags_result & CAPE_AIO_ABORT)
        {
          return CAPE_ERR_CONTINUE;
        }
      }
      else
      {
        if (hflags_result & CAPE_AIO_ABORT)
        {
          return CAPE_ERR_CONTINUE;
        }

        if (hflags_result & CAPE_AIO__INTERNAL_NO_CHANGE)
        {
          // there is no change on the hflags
        }
        else
        {
          hobj->hflags = hflags_result;
        }
        
        cape_aio_update_event (self, hobj, (void*)event.ident, hobj->option);
      }
    }
    else
    {
      const char* signalKind = NULL;
      
      // assign all known signals
      switch (event.ident)
      {
        case 1: signalKind = "SIGHUP (Hangup detected on controlling terminal or death of controlling process)"; break;
        case 2: signalKind = "SIGINT (Interrupt from keyboard)"; break;
        case 3: signalKind = "SIGQUIT (Quit from keyboard)"; break;
        case 4: signalKind = "SIGILL (Illegal Instruction)"; break;
        case 6: signalKind = "SIGABRT (Abort signal from abort(3))"; break;
        case 8: signalKind = "SIGFPE (Floating-point exception)"; break;
        case 9: signalKind = "SIGKILL (Kill signal)"; break;
        case 11: signalKind = "SIGSEGV (Invalid memory reference)"; break;
        case 13: signalKind = "SIGPIPE (Broken pipe: write to pipe with no readers; see pipe(7))"; break;
        case 15: signalKind = "SIGTERM (Termination signal)"; break;
      }
      
      if (signalKind)
      {
        cape_log_fmt (CAPE_LL_TRACE, "CAPE", "aio next", "signal seen [%i] -> %s", event.ident, signalKind);
        
        if (event.ident == SIGINT || event.ident == SIGTERM)
        {
          return CAPE_ERR_CONTINUE;
        }
      }
      else
      {
        //eclog_fmt (LL_TRACE, "ENTC", "signal", "signal seen [%i] -> unknown signal", event.ident);
      }
    }
    
    return CAPE_ERR_NONE;
  }
  
#else
  
  int n, res;
  struct epoll_event *events;
  sigset_t sigset;
  
  res = cape_aio_context_sigmask (self, &sigset);
  // we must block the signals in order for signalfd to receive them
  //res = sigprocmask (SIG_BLOCK, &sigset, NULL);
  
  // we should also block sigpipe
  sigaddset (&sigset, SIGPIPE);
  
  events = calloc (CAPE_AIO_EPOLL_MAXEVENTS, sizeof(struct epoll_event));
  
  if (events == NULL)
  {
    return -1;
  }
  
  //printf ("[%p] wait for next event\n", self);
  
  n = epoll_pwait (self->efd, events, CAPE_AIO_EPOLL_MAXEVENTS, timeout_in_ms, &sigset);
  
  if (n < 0)
  {
    res = cape_err_lastOSError (err);    
    goto exit;
  }
  
  if (n > 0)
  {
    int i = 0;
    
    //printf ("[%p] GOT EVENT %i ON %i\n", self, n, events[i].events);
    
    // retrieve the handle object from the userdata of the epoll event
    CapeAioHandle hobj = events[i].data.ptr;
    if (hobj)
    {
      number_t hflags_result;
      
      if (hobj->on_event)
      {
        hflags_result = hobj->on_event (hobj->ptr, hobj->hflags, events[i].events, 0);
      }
      else
      {
        cape_log_msg (CAPE_LL_WARN, "CAPE", "aio next", "no 'on_event' method was set in the aio handle");
        hflags_result = CAPE_AIO_NONE;
      }
      
      if (hflags_result & CAPE_AIO_DONE)
      {
        epoll_ctl (self->efd, EPOLL_CTL_DEL, (long)hobj->handle, &(events[i]));
        
        // remove the handle from events
        cape_aio_remove_handle (self, hobj);
        
        //printf ("[%p] handle removed\n", self);
        
        goto exit;
      }
      
      if (hflags_result & CAPE_AIO_ABORT)
      {
        //printf ("SET ABORT\n");
        
        res = CAPE_ERR_CONTINUE;
      }
      
      if (hflags_result == CAPE_AIO_NONE)
      {
        cape_aio_update_events (&(events[i]), hobj->hflags);
        
        epoll_ctl (self->efd, EPOLL_CTL_MOD, (long)hobj->handle, &(events[i]));
      }
      else
      {
        cape_aio_update_events (&(events[i]), hflags_result);
        
        hobj->hflags = hflags_result;
        
        epoll_ctl (self->efd, EPOLL_CTL_MOD, (long)hobj->handle, &(events[i]));
      }
    }
  }
  
  //---------------------
exit:
  
  // finally cleanup
  free (events);
  return res;

#endif
}

//-----------------------------------------------------------------------------

void cape_aio_context_mod (CapeAioContext self, CapeAioHandle aioh, void* handle, int hflags, number_t option)
{
#if defined __BSD_OS

  aioh->hflags = hflags;

  cape_aio_update_event (self, aioh, handle, option);

#else
  struct epoll_event event;
  
  // set the user pointer to the handle
  event.data.ptr = aioh;
  
  // set the current handle, we need it later
  aioh->handle = handle;
  
  if (hflags & CAPE_AIO_DONE)
  {
    epoll_ctl (self->efd, EPOLL_CTL_DEL, (long)handle, &event);
    
    cape_aio_remove_handle (self, aioh);
  }
  else
  {
    cape_aio_update_events (&event, hflags);
    
    int s = epoll_ctl (self->efd, EPOLL_CTL_MOD, (long)handle, &event);
    if (s < 0)
    {
      int errCode = errno;
      if (errCode == EPERM)
      {
        printf ("this filedescriptor is not supported by epoll\n");
        
      }
      else
      {
        CapeErr err = cape_err_new ();
        
        cape_err_lastOSError (err);
        
        printf ("can't add fd to epoll: %s\n", cape_err_text (err));
        
        cape_err_del (&err);
      }
      
      return;
    }
    
    aioh->hflags = hflags;
  }
  
#endif
}

//-----------------------------------------------------------------------------

int cape_aio_context_add (CapeAioContext self, CapeAioHandle aioh, void* handle, number_t option)
{
#if defined __BSD_OS

  if (!cape_aio_add_event (self, aioh, handle, option))
  {
    return FALSE;
  }
  
  cape_log_fmt (CAPE_LL_TRACE, "CAPE", "context add", "event added");
  
#else
  
  if (self->efd < 0)
  {
    cape_aio_handle_unref (aioh);    
    return FALSE;
  }
  
  struct epoll_event event;
  
  // set the user pointer to the handle
  event.data.ptr = aioh;
  
  // set the current handle, we need it later
  aioh->handle = handle;
  
  cape_aio_update_events (&event, aioh->hflags);
  
  int s = epoll_ctl (self->efd, EPOLL_CTL_ADD, (long)handle, &event);
  if (s < 0)
  {
    int errCode = errno;
    if (errCode == EPERM)
    {
      printf ("this filedescriptor is not supported by epoll\n");
      
    }
    else
    {
      CapeErr err = cape_err_new ();
      
      cape_err_lastOSError (err);
      
      printf ("can't add fd [%li] to epoll: %s\n", (long)handle, cape_err_text (err));
      
      cape_err_del (&err);
    }
    
    return FALSE;
  }
  
#endif

  pthread_mutex_lock (&(self->mutex));
  
  cape_list_push_back (self->events, aioh);
  
  pthread_mutex_unlock (&(self->mutex));

  return TRUE;
}

#if defined __LINUX_OS

//-----------------------------------------------------------------------------

int cape_aio_context_signal_map (CapeAioContext self, int signal, int status)
{
  if (signal > 0 && signal < 32)
  {
    self->smap[signal] = status;
    
    return 0;
  }
  
  return -1;
}

//-----------------------------------------------------------------------------

static int __STDCALL cape_aio_context_signal_onEvent (void* ptr, int hflags, unsigned long events, unsigned long param1)
{
  CapeAioContext self = ptr;
  
  struct signalfd_siginfo info;
  
  ssize_t bytes = read ((long)self->sfd, &info, sizeof(info));
  
  if (bytes == sizeof(info))
  {
    int sig = info.ssi_signo;
    
    //printf ("SIGNAL SEEN %i\n", sig);
    
    // send again (otherwise other signal handlers will not be triggered)
    kill (getpid(), sig);
    
    if (sig > 0 && sig < 32)
    {
      return self->smap[sig] | CAPE_AIO_READ;
    }
  }
  
  return CAPE_AIO_NONE;
}

//-----------------------------------------------------------------------------

static void __STDCALL cape_aio_context_signal_onUnref (void* ptr, CapeAioHandle aioh, int force_close)
{
  CapeAioContext self = ptr;
  
  //printf ("close signalfs\n");
  
  close ((long)(self->sfd));
  
  cape_aio_handle_del (&aioh);
}

#endif

//-----------------------------------------------------------------------------

int cape_aio_context_set_interupts (CapeAioContext self, int sigint, int term, CapeErr err)
{
#if defined __BSD_OS

  int res;
  sigset_t sigset;

  // null the sigset
  res = sigemptyset (&sigset);
  
  if (sigint)
  {
    struct kevent kev;
    memset (&kev, 0x0, sizeof (struct kevent));
    
    EV_SET (&kev, SIGINT, EVFILT_SIGNAL, EV_ADD | EV_ENABLE, 0, 0, NULL);
    
    // register SIGINT signal
    res = kevent (self->kq, &kev, 1, NULL, 0, NULL);
    if (res < 0)
    {
      return cape_err_lastOSError (err);
    }
    
    // add this signal to the sigset
    res = sigaddset (&sigset, SIGINT);
    if (res < 0)
    {
      return cape_err_lastOSError (err);
    }

    cape_log_msg (CAPE_LL_TRACE, "CAPE", "aio", "set SIGINT interupt");
  }

  if (term)
  {
    struct kevent kev;
    memset (&kev, 0x0, sizeof (struct kevent));
    
    EV_SET (&kev, SIGTERM, EVFILT_SIGNAL, EV_ADD | EV_ENABLE, 0, 0, NULL);
    
    // register SIGTERM signal
    res = kevent (self->kq, &kev, 1, NULL, 0, NULL);
    if (res < 0)
    {
      return cape_err_lastOSError (err);
    }

    // add this signal to the sigset
    res = sigaddset (&sigset, SIGTERM);
    if (res < 0)
    {
      return cape_err_lastOSError (err);
    }

    cape_log_msg (CAPE_LL_TRACE, "CAPE", "aio", "set SIGTERM interupt");
  }

  // we must block the signals in order for signals for event to receive them
  res = sigprocmask (SIG_BLOCK, &sigset, NULL);
  if (res)
  {
    return cape_err_lastOSError (err);    
  }

#else
  
  int res;
  sigset_t sigset;
  
  if (self->sfd != -1)
  {
    return cape_err_set (err, CAPE_ERR_NO_OBJECT, "file-descriptor is not set");
  }

  cape_aio_context_signal_map (self, SIGUSR1, CAPE_AIO_ABORT);

  if (sigint)
  {
    cape_aio_context_signal_map (self, SIGINT, CAPE_AIO_ABORT);
  }
  
  if (term)
  {
    cape_aio_context_signal_map (self, SIGTERM, CAPE_AIO_ABORT);
  }
  
  res = cape_aio_context_sigmask (self, &sigset);
  if (res)
  {
    return cape_err_lastOSError (err);    
  }
  
  // we must block the signals in order for signalfd to receive them
  res = sigprocmask (SIG_BLOCK, &sigset, NULL);
  if (res)
  {
    return cape_err_lastOSError (err);    
  }
  
  // create the signalfd
  self->sfd = signalfd(-1, &sigset, 0);
  
  CapeAioHandle aioh = cape_aio_handle_new (CAPE_AIO_READ, self, cape_aio_context_signal_onEvent, cape_aio_context_signal_onUnref);
  
  // add the signalfd to the event context
  cape_aio_context_add (self, aioh, (void*)self->sfd, 0);
  
#endif

  return CAPE_ERR_NONE;
}

//*****************************************************************************

#elif defined __WINDOWS_OS

#include <windows.h>

//-----------------------------------------------------------------------------

struct CapeAioHandle_s
{
  // Windows header for an OVERLAPPED structure
  DWORD Internal;
  DWORD InternalHigh;
  DWORD Offset;
  DWORD OffsetHigh;
  
  // WSA needs an extra handle here
  HANDLE reserved;

  void* ptr;
  
  int hflags;

  fct_cape_aio_onUnref on_unref;
  
  fct_cape_aio_onEvent on_event;
};

//-----------------------------------------------------------------------------

CapeAioHandle cape_aio_handle_new (int hflags, void* ptr, fct_cape_aio_onEvent on_event, fct_cape_aio_onUnref on_unref)
{
  CapeAioHandle self = CAPE_NEW (struct CapeAioHandle_s);
  
  SecureZeroMemory((PVOID)self, sizeof(struct CapeAioHandle_s));

  self->ptr = ptr;
  
  self->on_event = on_event;
  self->on_unref = on_unref;
  
  self->hflags = hflags;
  
  return self;
}

//-----------------------------------------------------------------------------

void cape_aio_handle_del (CapeAioHandle* p_self)
{
  if (*p_self)
  {
    CAPE_DEL (p_self, struct CapeAioHandle_s);
  }
}

//-----------------------------------------------------------------------------

void cape_aio_handle_unref (CapeAioHandle self, int close)
{
  if (self->on_unref)
  {
    // call the callback to signal the destruction of the handle
    self->on_unref (self->ptr, self, close);
  }
}

//-----------------------------------------------------------------------------

void cape_aio_handle_close (CapeAioHandle self)
{
  
  
}

//-----------------------------------------------------------------------------

struct CapeAioContext_s
{
  HANDLE port;
  
  CapeList events;      // store all events into this list (used only for destruction)
  
  CRITICAL_SECTION* mutex;
};

//-----------------------------------------------------------------------------

static void __STDCALL cape_aio_context__events_on_item_del (void* ptr)
{
  cape_aio_handle_unref (ptr, FALSE);
}

//-----------------------------------------------------------------------------

CapeAioContext cape_aio_context_new (void)
{
  CapeAioContext self = CAPE_NEW (struct CapeAioContext_s);
  
  self->port = NULL;
  self->events = cape_list_new (cape_aio_context__events_on_item_del);
  
  self->mutex = CAPE_NEW (CRITICAL_SECTION);
  
  InitializeCriticalSection (self->mutex);
  
  return self;
}

//-----------------------------------------------------------------------------

void cape_aio_context_del (CapeAioContext* p_self)
{
  if (*p_self)
  {
    CapeAioContext self = *p_self;
    
    DeleteCriticalSection (self->mutex);

    cape_list_del (&(self->events));
    
    CAPE_DEL(p_self, struct CapeAioContext_s);
  }
}

//-----------------------------------------------------------------------------

int cape_aio_context_open (CapeAioContext self, CapeErr err)
{
  // initialize windows io completion port
  self->port = CreateIoCompletionPort (INVALID_HANDLE_VALUE, NULL, 0, 0);
  if (self->port  == NULL)
  {
    return cape_err_lastOSError (err);
  }

  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

static int __STDCALL cape_aio_context_close__on_event (void* ptr, int hflags, unsigned long events, unsigned long extra)
{
  return CAPE_AIO_ABORT;
}

//-----------------------------------------------------------------------------

int cape_aio_context_close (CapeAioContext self, CapeErr err)
{
  DWORD t = 0;
  ULONG_PTR ptr2 = (ULONG_PTR)NULL;
  
  CapeAioHandle aioh = cape_aio_handle_new (0, self, cape_aio_context_close__on_event, NULL);
  
  // add this event to the completion port
  PostQueuedCompletionStatus (self->port, t, ptr2, (LPOVERLAPPED)aioh);
  
  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

int cape_aio_context_wait (CapeAioContext self, CapeErr err)
{
  while (cape_aio_context_next (self, -1, err) == CAPE_ERR_NONE);
  
  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

int cape_aio_context_next__overlapped (OVERLAPPED* ovl, int repeat, unsigned long bytes)
{
  int hflags_result = 0;

  // we can cast into the user defined struct
  CapeAioHandle hobj = (CapeAioHandle)ovl;
  
  if (hobj)
  {
    if (hobj->on_event)
    {
      hflags_result = hobj->on_event (hobj->ptr, hobj->hflags, 0, bytes);
    }
  }
  
  if (hflags_result & CAPE_AIO_DONE)
  {
    // remove the handle from events
    //cape_aio_remove_handle (self, hobj);
    
    return FALSE;
  }
  
  if (hflags_result & CAPE_AIO_ABORT)
  {
    return TRUE;
  }
  
  return FALSE;
}

//-----------------------------------------------------------------------------

int cape_aio_context_next (CapeAioContext self, long timeout_in_ms, CapeErr err)
{
  DWORD numOfBytes;
  ULONG_PTR ptr;
  OVERLAPPED* ovl;
  BOOL iores;

  // wait for any event on the completion port
  iores = GetQueuedCompletionStatus (self->port, &numOfBytes, &ptr, &ovl, timeout_in_ms);
  if (iores)
  {
    if (cape_aio_context_next__overlapped (ovl, TRUE, numOfBytes))
    {
      return cape_err_set (err, CAPE_ERR_CONTINUE, "wait abborted");
    }
    else
    {
      return CAPE_ERR_NONE;
    }
  }
  else
  {
    if (ovl == NULL)  // timeout
    {
      //onTimeout ();
    }
    else
    {
      DWORD lastError = GetLastError ();
      
      /*
      {
        EcErr err = ecerr_create ();
        
        ecerr_formatErrorOS (err, ENTC_LVL_ERROR, lastError);
        
        eclog_fmt (LL_WARN, "ENTC", "wait", "error on io completion port: %s", err->text);
        
        ecerr_destroy (&err);
      }
       */
      
      if (cape_aio_context_next__overlapped (ovl, FALSE, numOfBytes))
      {
        return cape_err_set (err, CAPE_ERR_CONTINUE, "wait abborted");
      }
      
      switch (lastError)
      {
        case ERROR_HANDLE_EOF:
        {
          return CAPE_ERR_NONE;
        }
        case 735: // ERROR_ABANDONED_WAIT_0
        {
          return cape_err_set (err, CAPE_ERR_OS, "wait abborted");
        }
        case ERROR_OPERATION_ABORTED: // ABORT
        {
          return cape_err_set (err, CAPE_ERR_OS, "wait abborted");
        }
        default:
        {
          //onTimeout ();
        }
      }
    }
    
    return CAPE_ERR_NONE;
  }
}

//-----------------------------------------------------------------------------

int cape_aio_context_add (CapeAioContext self, CapeAioHandle aioh, void* handle, number_t option)
{
  // add the handle to the overlapping completion port
  HANDLE cportHandle = CreateIoCompletionPort (handle, self->port, 0, 0);
  
  // cportHandle must return a value
  if (cportHandle == NULL)
  {
    CapeErr err = cape_err_new ();
    
    cape_err_lastOSError (err);
    
    cape_log_fmt (CAPE_LL_ERROR, "CAPE", "aio add", "can't add fd [%li] to completion port: %s", (long)handle, cape_err_text (err));
    
    cape_err_del (&err);

    return FALSE;
  }
  
  EnterCriticalSection (self->mutex);
  
  cape_list_push_back (self->events, aioh);
  
  LeaveCriticalSection (self->mutex);
  
  return TRUE;
}

//-----------------------------------------------------------------------------

void cape_aio_context_mod (CapeAioContext aio, CapeAioHandle aioh, void* handle, int hflags, number_t option)
{


}

//-----------------------------------------------------------------------------

static CapeAioContext g_aio = NULL;

static int cape_aio_context__ctrl_handler (unsigned long ctrlType)
{
  int abort = FALSE;
  
  switch( ctrlType )
  {
    case CTRL_C_EVENT:
    {
      abort = TRUE;
      
      cape_log_fmt (CAPE_LL_TRACE, "CAPE", "aio", "signal seen [%i] -> %s", ctrlType, "ctrl-c");
      break;
    }
    case CTRL_SHUTDOWN_EVENT:
    {
      abort = TRUE;
      
      cape_log_fmt (CAPE_LL_TRACE, "CAPE", "aio", "signal seen [%i] -> %s", ctrlType, "shutdown");
      break;
    }
    case CTRL_CLOSE_EVENT:
    {
      abort = TRUE;
      
      cape_log_fmt (CAPE_LL_TRACE, "CAPE", "aio", "signal seen [%i] -> %s", ctrlType, "close");
      break;
    }
    default:
    {
      abort = FALSE;
      
      cape_log_fmt (CAPE_LL_TRACE, "CAPE", "aio", "unknown signal seen [%i]", ctrlType);
      break;
    }
  }
  
  if (abort)
  {
    int res;
    CapeErr err = cape_err_new ();
    
    res = cape_aio_context_close (g_aio, err);
    
    cape_err_del (&err);
  }
  
  return TRUE;
}

//-----------------------------------------------------------------------------

int cape_aio_context_set_interupts (CapeAioContext self, int sigint, int term, CapeErr err)
{
  g_aio = self;
  
  if (SetConsoleCtrlHandler ((PHANDLER_ROUTINE) cape_aio_context__ctrl_handler, TRUE) == 0)
  {
    return cape_err_lastOSError (err);
  }
  
  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

#endif
