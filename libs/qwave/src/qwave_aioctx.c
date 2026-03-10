#include "qwave_aioctx.h"

// cape includes
#include <sys/cape_log.h>

#ifdef __WINDOWS_OS

#elif defined __LINUX_OS

#include <signal.h>
#include <errno.h>
#include <sys/epoll.h>
#include <sys/signalfd.h>
#include <sys/eventfd.h>

#define QWAVE_EPOLL_INVALID_FD           -1
#define QWAVE_EPOLL_MAX_EVENTS            1

//-----------------------------------------------------------------------------

struct QWaveAioctxEvent_s
{
    number_t fd;
    void* user_ptr;
    
    fct_qwave__on_aio_event on_recv;
    fct_qwave__on_aio_event on_done;    
}; 

//-----------------------------------------------------------------------------

QWaveAioctxEvent qwave_aioctx_event_new (number_t fd)
{
    QWaveAioctxEvent self = CAPE_NEW (struct QWaveAioctxEvent_s);
    
    self->fd = fd;
    self->user_ptr = NULL;
    
    self->on_recv = NULL;
    self->on_done = NULL;
    
    return self;
}

//-----------------------------------------------------------------------------

void qwave_aioctx_event_del (QWaveAioctxEvent* p_self)
{
    if (*p_self)
    {
        QWaveAioctxEvent self = *p_self;
        
        if (self->on_done)
        {
            self->on_done (self->user_ptr, (void*)(self->fd));
        }
                
        CAPE_DEL (p_self, struct QWaveAioctxEvent_s);
    }
}

//-----------------------------------------------------------------------------

void qwave_aioctx_event_set (QWaveAioctxEvent self, void* user_ptr, fct_qwave__on_aio_event on_recv, fct_qwave__on_aio_event on_done)
{
    self->user_ptr = user_ptr;

    self->on_recv = on_recv;
    self->on_done = on_done;
}

//-----------------------------------------------------------------------------

void* qwave_aioctx_event_get (QWaveAioctxEvent self)
{
    return (void*)self->fd;
}

//-----------------------------------------------------------------------------

int qwave_aioctx_handle__recv (QWaveAioctxEvent self)
{
    if (self->on_recv)
    {
        return self->on_recv (self->user_ptr, (void*)(self->fd));
    }
    else
    {
        return QWAVE_EVENT_RESULT__CONTINUE;
    }
}

//-----------------------------------------------------------------------------

struct QWaveAioctx_s
{
        
    int epoll_fd;         // epoll file descriptor
    int event_fd;         // eventfd file descriptior
    long sfd;             // eventfd file descriptor
    int smap[32];         // map for signal handling
};

//-----------------------------------------------------------------------------

QWaveAioctx qwave_aioctx_new ()
{
    QWaveAioctx self = CAPE_NEW (struct QWaveAioctx_s);
    
    self->epoll_fd = QWAVE_EPOLL_INVALID_FD;
    self->event_fd = QWAVE_EPOLL_INVALID_FD;
    
    return self;
}

//-----------------------------------------------------------------------------

void qwave_aioctx_del (QWaveAioctx* p_self)
{
    if (*p_self)
    {
        QWaveAioctx self = *p_self;
        
        if (QWAVE_EPOLL_INVALID_FD != self->epoll_fd)
        {
            close (self->epoll_fd);
        }
                
        if (QWAVE_EPOLL_INVALID_FD != self->event_fd)
        {
          close (self->event_fd);
        }
                
        CAPE_DEL (p_self, struct QWaveAioctx_s);
    }
}

//-----------------------------------------------------------------------------

int qwave_aioctx_epoll_ctl (QWaveAioctx self, int mode, int fd, int flags, void* data, CapeErr err)
{
    struct epoll_event event;
    
    // use the data.ptr part of the union to store 
    // a pointer to the QWaveAioctxEvent object
    event.data.ptr = data;
    
    // set the events on which the epoll should return
    event.events = flags;
    
    int s = epoll_ctl (self->epoll_fd, mode, fd, &event);
    if (s < 0)
    {
        int errCode = errno;
        
        if (errCode == EPERM)
        {
            cape_err_set (err, CAPE_ERR_OS, "this filedescriptor is not supported by epoll");            
            cape_log_msg (CAPE_LL_ERROR, "QWAVE", "epoll", cape_err_text (err));
        }
        else
        {
            cape_err_lastOSError (err);
            cape_log_fmt (CAPE_LL_ERROR, "QWAVE", "epoll", "can't use fd [%li] in epoll: %s", fd, cape_err_text (err));
        }
        
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}

//-----------------------------------------------------------------------------

int __STDCALL qwave_aioctx__on_event (void* user_ptr, void* handle)
{
    QWaveAioctx self = user_ptr;
  
    uint64_t u;
    
    ssize_t read_bytes = read (self->event_fd, &u, sizeof(uint64_t));
    
    if (read_bytes != sizeof(uint64_t))
    {      
        cape_log_fmt (CAPE_LL_ERROR, "QWAVE", "event", "error on internal event, not enough bytes");
    }
    else
    {
        QWaveAioctxEvent aio_event = (QWaveAioctxEvent)u;
      
        cape_log_fmt (CAPE_LL_TRACE, "QWAVE", "event", "shutdown event received {%p}", aio_event);
        
        // remove handle from epoll
        {
          CapeErr err = cape_err_new ();
          
          // try to remove the handle from the epoll
          if (qwave_aioctx_epoll_ctl (self, EPOLL_CTL_DEL, aio_event->fd, 0, NULL, err))
          {
            
          }
          else
          {
            
            
          }
          
          cape_err_del (&err);
        }

        // call user defined shutdown function and
        // cleanup handle event
        qwave_aioctx_event_del (&aio_event);
    }
    
    return QWAVE_EVENT_RESULT__CONTINUE;
}

//-----------------------------------------------------------------------------

int qwave_aioctx_open (QWaveAioctx self, CapeErr err)
{
    // create an event fd
    self->event_fd = eventfd (0, 0);

    // check if the open was successful
    if (self->event_fd == -1)
    {
      return cape_err_lastOSError (err);
    }
  
    // create a new epoll
    self->epoll_fd = epoll_create1 (0);
    
    // check if the open was successful
    if (self->epoll_fd == -1)
    {
        return cape_err_lastOSError (err);
    }
    
    {
        void* handle = (void*)(number_t)self->event_fd;
      
        QWaveAioctxEvent event = qwave_aioctx_add (self, &handle, err);
        
        if (NULL == event)
        {
            return cape_err_code (err);          
        }
        
        qwave_aioctx_event_set (event, self, qwave_aioctx__on_event, NULL);
    }
    
    return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

QWaveAioctxEvent qwave_aioctx_add (QWaveAioctx self, void** p_handle, CapeErr err)
{
    QWaveAioctxEvent ret;
    
    // create a new object for the handler
    ret = qwave_aioctx_event_new ((number_t)*p_handle);
    
    if (qwave_aioctx_epoll_ctl (self, EPOLL_CTL_ADD, (number_t)*p_handle, EPOLLET | EPOLLIN, ret, err))
    {
        *p_handle = NULL;
    }
    else
    {
        qwave_aioctx_event_del (&ret);
    }
    
    return ret;
}

//-----------------------------------------------------------------------------

void qwave_aioctx_rm (QWaveAioctx self, QWaveAioctxEvent* p_event)
{
    QWaveAioctxEvent aio_event = *p_event;
    
    cape_log_fmt (CAPE_LL_TRACE, "QWAVE", "event", "shutdown event received {%p}", aio_event);
    
    // remove handle from epoll
    {
      CapeErr err = cape_err_new ();
      
      // try to remove the handle from the epoll
      if (qwave_aioctx_epoll_ctl (self, EPOLL_CTL_DEL, aio_event->fd, 0, NULL, err))
      {
        
      }
      else
      {
        
        
      }
      
      cape_err_del (&err);
    }
    
    // call user defined shutdown function and
    // cleanup handle event
    qwave_aioctx_event_del (p_event);
  
  /*
  
    uint64_t u = (uint64_t)(*p_event);
  
    // to avoid race conditions, we trigger an internal event
    // this event will be handled by the same thread responsible to handle all input events
    ssize_t written_bytes = write (self->event_fd, &u, sizeof(uint64_t));
    
    cape_log_fmt (CAPE_LL_TRACE, "QWAVE", "event", "shutdown event added {%p}", *p_event);
    
    if (written_bytes != sizeof(uint64_t))
    {
        
      
    }
  */
}

//-----------------------------------------------------------------------------

int qwave_aioctx__handle_event (QWaveAioctx self, struct epoll_event* event, CapeErr err)
{
    QWaveAioctxEvent aio_event = event->data.ptr;

    switch (qwave_aioctx_handle__recv (aio_event))
    {
        case QWAVE_EVENT_RESULT__CONTINUE:
        {
            
         
            break;
        }
        case QWAVE_EVENT_RESULT__TRYAGAIN:
        {
            
            
            break;
        }
        case QWAVE_EVENT_RESULT__ERROR_CLOSED:
        {
            
            
            break;
        }
    }
    
    return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

int qwave_aioctx__sigmask (QWaveAioctx self, sigset_t* sigset, CapeErr err)
{
    int ret_code, i;
    
    // null the sigset
    ret_code = sigemptyset (sigset);
    if (-1 == ret_code)
    {
        return cape_err_lastOSError (err);
    }
    
    for (i = 0; i < 32; i++)
    {
        if (self->smap[i])
        {
            // add this signal to the sigset
            ret_code = sigaddset (sigset, i);
            if (-1 == ret_code)
            {
                return cape_err_lastOSError (err);
            }
        }
    }
    
    return 0;
}

//-----------------------------------------------------------------------------

int qwave_aioctx_next (QWaveAioctx self, number_t timeout_in_ms, CapeErr err)
{
    int res;
    
    // local objects
    struct epoll_event* events = NULL;
    sigset_t sigset;
    
    res = qwave_aioctx__sigmask (self, &sigset, err);
    if (res)
    {
      
      goto cleanup_and_exit;
    }
    
    // we should also block sigpipe
    sigaddset (&sigset, SIGPIPE);
    
    events = calloc (QWAVE_EPOLL_MAX_EVENTS, sizeof(struct epoll_event));
    if (NULL == events)
    {
        res = cape_err_set (err, CAPE_ERR_NO_OBJECT, "ERR.ALLOCATE_EVENTS");
        goto cleanup_and_exit;
    }
    
    {
        int i;
        
        // wait for the next event
        int number_of_events = epoll_pwait (self->epoll_fd, events, QWAVE_EPOLL_MAX_EVENTS, timeout_in_ms, &sigset);

        if (number_of_events < 0)
        {
            res = cape_err_lastOSError (err);
            goto cleanup_and_exit;        
        }   
                
        for (i = 0; i < number_of_events; i++)
        {
            cape_log_fmt (CAPE_LL_TRACE, "QWAVE", "next", "triggered event = %i/%i", i, number_of_events);

            res = qwave_aioctx__handle_event (self, &(events[i]), err);
            if (res)
            {
                goto cleanup_and_exit;
            }
        }
    }
    
    res = CAPE_ERR_NONE;
    
cleanup_and_exit:
    
    free (events);
    return res;
}

//-----------------------------------------------------------------------------

int qwave_aioctx_kill (QWaveAioctx self, CapeErr err)
{
    // trigger a kill event
    kill (getpid(), SIGTERM);
  
    return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

#elif defined __BSD_OS

#include <sys/event.h>
#include <unistd.h>
#include <errno.h>

//-----------------------------------------------------------------------------

struct QWaveAioctxEvent_s
{
    number_t fd;
    void* user_ptr;
  
    fct_qwave__on_aio_event on_recv;
    fct_qwave__on_aio_event on_done;

}; typedef struct QWaveAioctxEvent_s* QWaveAioctxEvent;

//-----------------------------------------------------------------------------

QWaveAioctxEvent qwave_aioctx_event_new (number_t fd)
{
    QWaveAioctxEvent self = CAPE_NEW (struct QWaveAioctxEvent_s);
    
    self->fd = fd;
    self->user_ptr = NULL;
  
    self->on_recv = NULL;
    self->on_done = NULL;

    return self;
}

//-----------------------------------------------------------------------------

void qwave_aioctx_event_del (QWaveAioctxEvent* p_self)
{
    if (*p_self)
    {
        QWaveAioctxEvent self = *p_self;
        
        
        
        
        CAPE_DEL (p_self, struct QWaveAioctxEvent_s);
    }
}

//-----------------------------------------------------------------------------

void qwave_aioctx_event_set (QWaveAioctxEvent self, void* user_ptr, fct_qwave__on_aio_event on_recv, fct_qwave__on_aio_event on_done)
{
    self->user_ptr = user_ptr;

    self->on_recv = on_recv;
    self->on_done = on_done;
}

//-----------------------------------------------------------------------------

void* qwave_aioctx_event_get (QWaveAioctxEvent self)
{
    return (void*)self->fd;
}

//-----------------------------------------------------------------------------

void qwave_aioctx_event (QWaveAioctxEvent self, void* user_ptr, fct_qwave__on_aio_event fct)
{
    self->user_ptr = user_ptr;
    self->on_recv = fct;
}

//-----------------------------------------------------------------------------

int qwave_aioctx_handle (QWaveAioctxEvent self)
{
    if (self->on_recv)
    {
        return self->on_recv (self->user_ptr, (void*)(self->fd));
    }
    else
    {
        return QWAVE_EVENT_RESULT__CONTINUE;
    }
}

//-----------------------------------------------------------------------------

struct QWaveAioctx_s
{
    int kevent_fd;         // kevent file descriptor
};

//-----------------------------------------------------------------------------

QWaveAioctx qwave_aioctx_new ()
{
    QWaveAioctx self = CAPE_NEW (struct QWaveAioctx_s);

    self->kevent_fd = -1;
    
    return self;
}

//-----------------------------------------------------------------------------

void qwave_aioctx_del (QWaveAioctx* p_self)
{
    if (*p_self)
    {
        QWaveAioctx self = *p_self;
        
        if (-1 != self->kevent_fd)
        {
            close (self->kevent_fd);
            self->kevent_fd = -1;
        }
                
        CAPE_DEL (p_self, struct QWaveAioctx_s);
    }
}

//-----------------------------------------------------------------------------

int qwave_aioctx_open (QWaveAioctx self, CapeErr err)
{
    // create a new kevent
    self->kevent_fd = kqueue ();
    
    // check if the open was successful
    if (self->kevent_fd == -1)
    {
      return cape_err_lastOSError (err);
    }
    
    return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

int qwave_aioctx_kill (QWaveAioctx self, CapeErr err)
{
  
}

//-----------------------------------------------------------------------------

QWaveAioctxEvent qwave_aioctx_add (QWaveAioctx self, void** p_handle, CapeErr err)
{
  QWaveAioctxEvent ret;

  int res;
  int i = 0;
  
  void* handle = *p_handle;
  struct kevent change_event;
    
  // create a new object for the handler
  ret = qwave_aioctx_event_new ((number_t)*p_handle);

  EV_SET (&change_event, (number_t)handle, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, ret);
  
  if (-1 == kevent (self->kevent_fd, &change_event, 1, NULL, 0, NULL))
  {
    qwave_aioctx_event_del (&ret);

    
  }

  *p_handle = NULL;
  
  res = CAPE_ERR_NONE;
  
cleanup_and_exit:
  
  return ret;
}

//-----------------------------------------------------------------------------

void qwave_aioctx_rm (QWaveAioctx self, QWaveAioctxEvent* p_event)
{
  
}

//-----------------------------------------------------------------------------

int qwave_aioctx_next (QWaveAioctx self, number_t timeout_in_ms, CapeErr err)
{
  int res;
  struct timespec tmout;
  
  struct kevent event;
  memset (&event, 0x0, sizeof(struct kevent));
  
  if (timeout_in_ms == -1)
  {
    res = kevent (self->kevent_fd, NULL, 0, &event, 1, NULL);
  }
  else
  {
    // calculate the correct tmout values
    tmout.tv_sec = timeout_in_ms / 1000;
    tmout.tv_nsec = (timeout_in_ms % 1000) * 1000;
    
    res = kevent (self->kevent_fd, NULL, 0, &event, 1, &tmout);
  }
  
  //cape_log_fmt (CAPE_LL_TRACE, "CAPE", "aio", "event %i", res);
  
  if( res == -1 )
  {
    if (errno == EINTR)
    {
      return CAPE_ERR_NONE;
    }
    
    res = cape_err_lastOSError (err);
    
    cape_log_fmt (CAPE_LL_ERROR, "CAPE", "aio next", "aio error: %s", cape_err_text (err));
    
    return res;
  }
  else if (event.flags & EV_ERROR)
  {
    res = cape_err_lastOSError (err);
    
    cape_log_fmt (CAPE_LL_ERROR, "CAPE", "aio next", "aio error: %s", cape_err_text (err));
    
    return res;
  }
  else if (res == 0)
  {
    return CAPE_ERR_NONE;  // timeout
  }
  else
  {
    /*
    // retrieve the handle object from the userdata of the epoll event
    QWaveAioctxEvent event = (QWaveAioctxEvent)event.udata;
    if (event)
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
        cape_log_fmt (CAPE_LL_TRACE, "CAPE", "aio next", "remove handle %p", hobj->ptr);

        // remove the event from the kqueue
        cape_aio_delete_event (self, hobj, (void*)event.ident);

        // remove the handle from events
        cape_aio_remove_handle (self, hobj);

        if (hflags_result & CAPE_AIO_ABORT)
        {
          cape_log_fmt (CAPE_LL_TRACE, "CAPE", "aio next", "abort");
          return CAPE_ERR_CONTINUE;
        }
      }
      else
      {
        if (hflags_result & CAPE_AIO_ABORT)
        {
          cape_log_fmt (CAPE_LL_TRACE, "CAPE", "aio next", "abort");
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
     */

    return CAPE_ERR_NONE;
  }
}

//-----------------------------------------------------------------------------

#endif

//-----------------------------------------------------------------------------

int qwave_aioctx_wait (QWaveAioctx self, CapeErr err)
{
    while (qwave_aioctx_next (self, -1, err) == CAPE_ERR_NONE);
    
    return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------


