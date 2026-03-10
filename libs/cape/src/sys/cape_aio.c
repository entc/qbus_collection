#include "cape_aio.h"

// cape includes
#include <sys/cape_log.h>

//-----------------------------------------------------------------------------

#if defined __LINUX_OS

#include <signal.h>
#include <errno.h>
#include <sys/epoll.h>
#include <sys/signalfd.h>
#include <sys/eventfd.h>

#elif defined __BSD_OS

#include <sys/event.h>
#include <errno.h>

#elif defined _WIN64 || defined _WIN32

#include <ws2tcpip.h>
#include <winsock2.h>

#include <windows.h>
#include <stdio.h>

#endif

#define MAX_EVENTS 64

//-----------------------------------------------------------------------------

struct CapeAioItem_s
{
  number_t fd;
  void* user_ptr;
  
  fct_cape_aio_item__on_event on_recv;
  fct_cape_aio_item__on_event on_done;
};

//-----------------------------------------------------------------------------

CapeAioItem cape_aio_item_new (number_t fd)
{
  CapeAioItem self = CAPE_NEW (struct CapeAioItem_s);
  
  self->fd = fd;
  self->user_ptr = NULL;
  
  self->on_recv = NULL;
  self->on_done = NULL;
  
  return self;
}

//-----------------------------------------------------------------------------

void cape_aio_item_del (CapeAioItem* p_self)
{
  if (*p_self)
  {
    CapeAioItem self = *p_self;
    
    
    
    
    CAPE_DEL (p_self, struct CapeAioItem_s);
  }
}

//-----------------------------------------------------------------------------

void cape_aio_item_set (CapeAioItem self, void* user_ptr, fct_cape_aio_item__on_event on_recv, fct_cape_aio_item__on_event on_done)
{
  
  
}

//-----------------------------------------------------------------------------

void* cape_aio_item_get (CapeAioItem self)
{
  
  
}

//-----------------------------------------------------------------------------

void cape_aio_item__on_event (CapeAioItem self)
{
  
}

//-----------------------------------------------------------------------------

struct CapeAio_s
{
#if defined __LINUX_OS

  int epoll_fd;

#elif defined __BSD_OS

  int kq;

#elif defined _WIN64 || defined _WIN32


#endif
};

//-----------------------------------------------------------------------------

CapeAio cape_aio_new (void)
{  
  CapeAio self = CAPE_NEW (struct CapeAio_s);
  
#if defined __LINUX_OS
  
  self->epoll_fd = 0;
  
#elif defined __BSD_OS
  
  self->kq = 0;
  
#elif defined _WIN64 || defined _WIN32
  
  
#endif
  
  return self;
}

//-----------------------------------------------------------------------------

void cape_aio_del (CapeAio* p_self)
{
  if (*p_self)
  {
    CapeAio self = *p_self;
    
    
    
    
    CAPE_DEL (p_self, struct CapeAio_s);
  }
}

//-----------------------------------------------------------------------------

int cape_aio_init (CapeAio self, CapeErr err)
{
#if defined __LINUX_OS

  self->epoll_fd = epoll_create1 (0);

  // check if the open was successful
  if (self->epoll_fd == -1)
  {
    return cape_err_lastOSError (err);
  }
  
#elif defined __BSD_OS

  self->kq = kqueue();

  // check if the open was successful
  if (self->kq == -1)
  {
    return cape_err_lastOSError (err);
  }

#elif defined _WIN64 || defined _WIN32


#endif
  
  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

#if defined __LINUX_OS

int cape_aio__epoll_ctl (CapeAio self, int mode, int fd, int flags, void* data, CapeErr err)
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

#elif defined __BSD_OS

int cape_aio__kevent_set (CapeAio self, int mode, int fd, int flags, void* data, CapeErr err)
{
  int res;
  struct kevent change;
  
  // set all options
  EV_SET (&change, fd, flags, mode, 0, 0, data);
  
  // apply the set by the systemcall to the kevent subsystem
  res = kevent (self->kq, &change, 1, NULL, 0, NULL);

  if (res < 0)
  {
    cape_err_lastOSError (err);
    return FALSE;
  }
  else
  {
    return TRUE;
  }
}

#endif

//-----------------------------------------------------------------------------

CapeAioItem cape_aio_add (CapeAio self, void* handle, CapeErr err)
{
  CapeAioItem ret;
  
  // create a new object for the handler
  ret = cape_aio_item_new ((number_t)handle);
  
#if defined __LINUX_OS

  if (FALSE == cape_aio__epoll_ctl (self, EPOLL_CTL_ADD, (number_t)handle, EPOLLET | EPOLLIN, ret, err))
  {
    cape_aio_item_del (&ret);
  }
  
#elif defined __BSD_OS

  if (FALSE == cape_aio__kevent_set (self, EV_ADD, (int)(number_t)handle, EVFILT_READ, ret, err))
  {
    cape_aio_item_del (&ret);
  }

#elif defined _WIN64 || defined _WIN32


#endif

  return ret;
}

//-----------------------------------------------------------------------------

void cape_aio_rm (CapeAio self, CapeAioItem* p_hitem)
{
  
}

//-----------------------------------------------------------------------------

int cape_aio_next (CapeAio self, number_t timeout_in_ms, CapeErr err)
{
  int res;
  
#if defined __LINUX_OS

  // local objects
  struct epoll_event events[MAX_EVENTS];
  sigset_t sigset;
  
  /*
  res = qwave_aioctx__sigmask (self, &sigset, err);
  if (res)
  {
    
    goto cleanup_and_exit;
  }
  */
  
  // we should also block sigpipe
  //sigaddset (&sigset, SIGPIPE);
    
  {
    int i;
          
    // wait for the next event
    int number_of_events = epoll_wait (self->epoll_fd, events, MAX_EVENTS, timeout_in_ms);

    if (number_of_events < 0)
    {
      res = cape_err_lastOSError (err);
      goto cleanup_and_exit;
    }

    for (i = 0; i < number_of_events; i++)
    {
      cape_log_fmt (CAPE_LL_TRACE, "QWAVE", "next", "triggered event = %i/%i", i, number_of_events);
      
      // this handles the event
      cape_aio_item__on_event (events[i].data.ptr);
    }
  }
  
  res = CAPE_ERR_NONE;
  
cleanup_and_exit:
  
#elif defined __BSD_OS

  struct kevent events[MAX_EVENTS];
  
  {
    int i;
    
    // wait for the next event
    int number_of_events = kevent (self->kq, NULL, 0, events, MAX_EVENTS, NULL);

    if (number_of_events == -1)
    {
      if (errno == EINTR)
      {
        return CAPE_ERR_NONE;
      }
      
      res = cape_err_lastOSError (err);
      
      cape_log_fmt (CAPE_LL_ERROR, "CAPE", "aio next", "aio error: %s", cape_err_text (err));
      
      return res;
    }
    
    for (i = 0; i < number_of_events; i++)
    {
      struct kevent* event = &(events[i]);
      
      if (event->flags & EV_ERROR)
      {
        res = cape_err_lastOSError (err);
        
        cape_log_fmt (CAPE_LL_ERROR, "CAPE", "aio next", "aio error: %s", cape_err_text (err));
        
        return res;
      }
      else
      {
        cape_log_fmt (CAPE_LL_TRACE, "QWAVE", "next", "triggered event = %i/%i", i, number_of_events);

        // this handles the event
        cape_aio_item__on_event (event->udata);
      }
    }
  }
    
#elif defined _WIN64 || defined _WIN32


#endif

  return res;
}

//-----------------------------------------------------------------------------

int cape_aio_wait (CapeAio self, CapeErr err)
{
  while (cape_aio_next (self, -1, err) == CAPE_ERR_NONE);
  
  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------
