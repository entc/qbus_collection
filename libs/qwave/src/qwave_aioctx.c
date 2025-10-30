#include "qwave_aioctx.h"

// cape includes
#include <sys/cape_log.h>

#ifdef __WINDOWS_OS

#elif defined __LINUX_OS

#include <signal.h>
#include <errno.h>
#include <sys/epoll.h>
#include <sys/signalfd.h>

#define QWAVE_EPOLL_INVALID_FD           -1
#define QWAVE_EPOLL_MAX_EVENTS            10

//-----------------------------------------------------------------------------

struct QWaveAioctxEvent_s
{
    number_t fd;
    void* user_ptr;
    fct_qwave__on_aio_event on_event;
    
}; typedef struct QWaveAioctxEvent_s* QWaveAioctxEvent;

//-----------------------------------------------------------------------------

QWaveAioctxEvent qwave_aioctx_event_new (number_t fd, void* user_ptr, fct_qwave__on_aio_event fct)
{
    QWaveAioctxEvent self = CAPE_NEW (struct QWaveAioctxEvent_s);
    
    self->fd = fd;
    self->user_ptr = user_ptr;
    self->on_event = fct;

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

int qwave_aioctx_handle (QWaveAioctxEvent self)
{
    if (self->on_event)
    {
        return self->on_event (self->user_ptr, (void*)(self->fd));
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
    long sfd;             // eventfd file descriptor
    int smap[32];         // map for signal handling
};

//-----------------------------------------------------------------------------

QWaveAioctx qwave_aioctx_new ()
{
    QWaveAioctx self = CAPE_NEW (struct QWaveAioctx_s);
    
    self->epoll_fd = QWAVE_EPOLL_INVALID_FD;
    
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
                
        CAPE_DEL (p_self, struct QWaveAioctx_s);
    }
}

//-----------------------------------------------------------------------------

int qwave_aioctx_open (QWaveAioctx self, CapeErr err)
{
    // create a new epoll
    self->epoll_fd = epoll_create1 (0);
    
    // check if the open was successful
    if (self->epoll_fd == -1)
    {
        return cape_err_lastOSError (err);
    }
    
    return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

int qwave_aioctx_add (QWaveAioctx self, void** p_handle, void* user_ptr, fct_qwave__on_aio_event fct, CapeErr err)
{
    int res;
    
    struct epoll_event event;
    number_t handle_fd = (number_t)*p_handle;
    
    cape_log_fmt (CAPE_LL_TRACE, "QWAVE", "add", "append event with fd [%lu]", handle_fd);

    // use the data.ptr part of the union to store 
    // a pointer to the QWaveAioctxEvent object
    event.data.ptr = qwave_aioctx_event_new (handle_fd, user_ptr, fct);
    
    // set the events on which the epoll should return
    event.events = EPOLLET | EPOLLIN;
        
    int s = epoll_ctl (self->epoll_fd, EPOLL_CTL_ADD, handle_fd, &event);
    if (s < 0)
    {
        int errCode = errno;
        if (errCode == EPERM)
        {
            res = cape_err_set (err, CAPE_ERR_OS, "this filedescriptor is not supported by epoll");
            
            cape_log_msg (CAPE_LL_ERROR, "QWAVE", "add []", cape_err_text (err));
        }
        else
        {
            res = cape_err_lastOSError (err);

            cape_log_fmt (CAPE_LL_ERROR, "QWAVE", "add []", "can't add fd [%li] to epoll: %s", (number_t)*p_handle, cape_err_text (err));
        }
    }
    else
    {
        *p_handle = NULL;
        res = CAPE_ERR_NONE;
    }
    
    return res;
}

//-----------------------------------------------------------------------------

int qwave_aioctx__handle_event (QWaveAioctx self, struct epoll_event* event, CapeErr err)
{
    QWaveAioctxEvent aio_event = event->data.ptr;

    switch (qwave_aioctx_handle (aio_event))
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

int qwave_aioctx_next (QWaveAioctx self, number_t timeout_in_ms, CapeErr err)
{
    int res;
    
    // local objects
    struct epoll_event* events = NULL;
    sigset_t sigset;
    
    //  res = qwave__events__sigmask (self, &sigset);
    // we must block the signals in order for signalfd to receive them
    //res = sigprocmask (SIG_BLOCK, &sigset, NULL);
    
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

#elif defined __BSD_OS

#include <sys/event.h>
#include <unistd.h>

//-----------------------------------------------------------------------------

struct QWaveAioctxEvent_s
{
    number_t fd;
    void* user_ptr;
    fct_qwave__on_aio_event on_event;
    
}; typedef struct QWaveAioctxEvent_s* QWaveAioctxEvent;

//-----------------------------------------------------------------------------

QWaveAioctxEvent qwave_aioctx_event_new (number_t fd, void* user_ptr, fct_qwave__on_aio_event fct)
{
    QWaveAioctxEvent self = CAPE_NEW (struct QWaveAioctxEvent_s);
    
    self->fd = fd;
    self->user_ptr = user_ptr;
    self->on_event = fct;

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

int qwave_aioctx_handle (QWaveAioctxEvent self)
{
    if (self->on_event)
    {
        return self->on_event (self->user_ptr, (void*)(self->fd));
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

int qwave_aioctx_add (QWaveAioctx self, void** p_handle, void* user_ptr, fct_qwave__on_aio_event fct, CapeErr err)
{
  int res;
  int i = 0;
  
  void* handle = *p_handle;
  struct kevent change_event;
  
  // local objects
  QWaveAioctxEvent event = qwave_aioctx_event_new (handle, user_ptr, fct);

  EV_SET(&change_event, (number_t)handle, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, event);
  
  if (-1 == kevent (self->kevent_fd, &change_event, 1, NULL, 0, NULL))
  {
    
    
  }

  event = NULL;
  *p_handle = NULL;
  
  res = CAPE_ERR_NONE;
  
cleanup_and_exit:
  
  qwave_aioctx_event_del (&event);
  return res;
}

//-----------------------------------------------------------------------------

int qwave_aioctx_next (QWaveAioctx self, number_t timeout_in_ms, CapeErr err)
{
  
  
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


