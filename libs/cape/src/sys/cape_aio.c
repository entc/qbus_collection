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
  void* handle;
  void* user_ptr;

  fct_cape_aio_item__on_event on_event;
  fct_cape_aio_item__on_event on_done;
};

//-----------------------------------------------------------------------------

CapeAioItem cape_aio_item_new (void* handle)
{
  CapeAioItem self = CAPE_NEW (struct CapeAioItem_s);

  self->handle = handle;
  self->user_ptr = NULL;

  self->on_event = NULL;
  self->on_done = NULL;

  return self;
}

//-----------------------------------------------------------------------------

void cape_aio_item_del (CapeAioItem* p_self)
{
  if (*p_self)
  {
    CapeAioItem self = *p_self;

    if (self->on_done)
    {
      self->on_done (self->user_ptr, self->handle);
    }

    CAPE_DEL (p_self, struct CapeAioItem_s);
  }
}

//-----------------------------------------------------------------------------

void cape_aio_item_set (CapeAioItem self, void* user_ptr, fct_cape_aio_item__on_event on_event, fct_cape_aio_item__on_event on_done)
{
  self->user_ptr = user_ptr;

  self->on_event = on_event;
  self->on_done = on_done;
}

//-----------------------------------------------------------------------------

void* cape_aio_item_get (CapeAioItem self)
{
  return self->handle;
}

//-----------------------------------------------------------------------------

void cape_aio_item__on_event (CapeAioItem self, number_t bytes_affected)
{
  if (self->on_event)
  {
    self->on_event (self->user_ptr, self->handle);
  }
}

//-----------------------------------------------------------------------------

struct CapeAio_s
{
  int running;          // indicates the running status
  
#if defined __LINUX_OS

  int epoll_fd;
  int signal_fd;

  int smap[32];         // map for signal handling
  sigset_t sigset;      // signal handling kernel set

#elif defined __BSD_OS

  int kq;

#elif defined _WIN64 || defined _WIN32

  HANDLE iocp;

#endif
};

//-----------------------------------------------------------------------------

CapeAio cape_aio_new (void)
{
  CapeAio self = CAPE_NEW (struct CapeAio_s);

  self->running = TRUE;
  
#if defined __LINUX_OS

  self->epoll_fd = -1;
  self->signal_fd = -1;

  {
    int i;
    
    for (i = 0; i < 32; i++)
    {
      self->smap[i] = 0;
    }
  }
  
#elif defined __BSD_OS

  self->kq = -1;

#elif defined _WIN64 || defined _WIN32

  self->iocp = NULL;

#endif

  return self;
}

//-----------------------------------------------------------------------------

void cape_aio_del (CapeAio* p_self)
{
  if (*p_self)
  {
    CapeAio self = *p_self;

#if defined __LINUX_OS

    if (self->signal_fd != -1)
    {
      close (self->signal_fd);
    }
    
    if (self->epoll_fd != -1)
    {
        close (self->epoll_fd);
    }

#elif defined __BSD_OS

    if (self->kq != -1)
    {
        close (self->kq);
    }

#elif defined _WIN64 || defined _WIN32


#endif

    CAPE_DEL (p_self, struct CapeAio_s);
  }
}

//-----------------------------------------------------------------------------

#if defined __LINUX_OS

//-----------------------------------------------------------------------------

int cape_aio__sigmask (CapeAio self, CapeErr err)
{
    int ret_code, i;

    // null the sigset
    ret_code = sigemptyset (&(self->sigset));
    if (-1 == ret_code)
    {
        return cape_err_lastOSError (err);
    }

    for (i = 0; i < 32; i++)
    {
        if (self->smap[i])
        {
            // add this signal to the sigset
            ret_code = sigaddset (&(self->sigset), i);
            if (-1 == ret_code)
            {
                return cape_err_lastOSError (err);
            }
        }
    }

    return 0;
}

//-----------------------------------------------------------------------------

void __STDCALL cape_aio__signal__on_event (void* user_ptr, void* handle)
{
  CapeAio self = user_ptr;
  
  // turn of the running status -> terminate wait loop
  self->running = FALSE;
  
  cape_log_msg (CAPE_LL_DEBUG, "CAPE", "aio", "stop AIO loop");
}

//-----------------------------------------------------------------------------

void __STDCALL cape_aio__signal__on_done (void* user_ptr, void* handle)
{
  
}

//-----------------------------------------------------------------------------

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

//-----------------------------------------------------------------------------

#elif defined __BSD_OS

//-----------------------------------------------------------------------------

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

//-----------------------------------------------------------------------------

#endif

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

  // set sigset for handling signals
  self->smap[SIGTERM] = TRUE;
  
  if (cape_aio__sigmask (self, err))
  {
    return cape_err_code (err);
  }
  
  // create the signalfd
  self->signal_fd = signalfd(-1, &(self->sigset), 0);

  if (self->signal_fd == -1)
  {
    close (self->epoll_fd);
    
    return cape_err_lastOSError (err);
  }

  {
    CapeAioItem aio_item = cape_aio_add (self, (void*)(number_t)self->signal_fd, err);

    if (NULL == aio_item)
    {
      // TODO: cleanup file descriptors
    }
    
    cape_aio_item_set (aio_item, self, cape_aio__signal__on_event, cape_aio__signal__on_done);
  }

  // set sigset to ignore handling signals for epoll
  self->smap[SIGTERM] = TRUE;
  self->smap[SIGINT] = TRUE;
  
  if (cape_aio__sigmask (self, err))
  {
    return cape_err_code (err);
  }
  
#elif defined __BSD_OS

  self->kq = kqueue();

  // check if the open was successful
  if (self->kq == -1)
  {
    return cape_err_lastOSError (err);
  }

#elif defined _WIN64 || defined _WIN32

  // initialize windows io completion port
  self->iocp = CreateIoCompletionPort (INVALID_HANDLE_VALUE, NULL, 0, 0);
  if (self->iocp  == NULL)
  {
    return cape_err_lastOSError (err);
  }

#endif

  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

CapeAioItem cape_aio_add (CapeAio self, void* handle, CapeErr err)
{
  CapeAioItem ret;

  // create a new object for the handler
  ret = cape_aio_item_new (handle);

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

  // add the handle to the overlapping completion port
  HANDLE iocp_handle = CreateIoCompletionPort (handle, self->iocp, 0, 0);

  // cportHandle must return a value
  if (NULL == iocp_handle)
  {
    cape_err_lastOSError (err);

    cape_log_fmt (CAPE_LL_ERROR, "CAPE", "aio add", "can't add fd [%li] to completion port: %s", (long)handle, cape_err_text (err));

    cape_aio_item_del (&ret);
  }


#endif

  cape_log_fmt (CAPE_LL_TRACE, "CAPE", "aio add", "new aio item was added {%p} -> %i", ret, (long)handle);

  return ret;
}

//-----------------------------------------------------------------------------

void cape_aio_rm (CapeAio self, CapeAioItem* p_hitem)
{
  CapeAioItem hitem = *p_hitem;

  // remove handle from epoll
  {
    CapeErr err = cape_err_new ();

#if defined __LINUX_OS

    if (FALSE == cape_aio__epoll_ctl (self, EPOLL_CTL_DEL, (int)(number_t)cape_aio_item_get (hitem), 0, NULL, err))
    {

    }

#elif defined __BSD_OS

    if (FALSE == cape_aio__kevent_set (self, EV_DELETE, (int)(number_t)cape_aio_item_get (hitem), 0, NULL, err))
    {

    }

#elif defined _WIN64 || defined _WIN32

#endif

    else
    {
      // call user defined shutdown function and
      // cleanup handle event
      cape_aio_item_del (p_hitem);
    }

    cape_err_del (&err);
  }
}

//-----------------------------------------------------------------------------

int cape_aio_next (CapeAio self, number_t timeout_in_ms, CapeErr err)
{
  int res;

#if defined __LINUX_OS

  // local objects
  struct epoll_event events[MAX_EVENTS];

  // we should also block sigpipe
  //sigaddset (&sigset, SIGPIPE);

  {
    int i;

    // wait for the next event
    int number_of_events = epoll_pwait (self->epoll_fd, events, MAX_EVENTS, timeout_in_ms, &(self->sigset));

    if (number_of_events < 0)
    {
      res = cape_err_lastOSError (err);
      goto cleanup_and_exit;
    }

    for (i = 0; i < number_of_events; i++)
    {
      cape_log_fmt (CAPE_LL_TRACE, "QWAVE", "next", "triggered event = %i/%i", i, number_of_events);

      // this handles the event
      cape_aio_item__on_event (events[i].data.ptr, 0);
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
        cape_aio_item__on_event (event->udata, 0);
      }
    }
  }

#elif defined _WIN64 || defined _WIN32

  OVERLAPPED_ENTRY overlappeds[MAX_EVENTS];
  ULONG count;

  // wait for any event on the completion port
  if (GetQueuedCompletionStatusEx (self->iocp, overlappeds, MAX_EVENTS, &count, timeout_in_ms, TRUE))
  {
    ULONG i;

    for (i = 0; i < count; i++)
    {
      // this handles the event
      cape_aio_item__on_event (overlappeds[i].lpOverlapped, overlappeds[i].dwNumberOfBytesTransferred);
    }
  }

#endif

  return res;
}

//-----------------------------------------------------------------------------

int cape_aio_wait (CapeAio self, CapeErr err)
{
  while (self->running && (cape_aio_next (self, -1, err) == CAPE_ERR_NONE));

  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

void cape_aio_kill (CapeAio self)
{
#if defined __LINUX_OS

  cape_log_fmt (CAPE_LL_TRACE, "CAPE", "aio", "stop loop");
  
  // trigger the terminate signal
  kill (getpid(), SIGTERM);

#elif defined __BSD_OS


#elif defined _WIN64 || defined _WIN32

#endif
}

//-----------------------------------------------------------------------------
