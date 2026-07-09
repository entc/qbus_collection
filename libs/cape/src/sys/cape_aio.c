#include "cape_aio.h"

// cape includes
#include <sys/cape_log.h>
#include <stc/cape_map.h>

//-----------------------------------------------------------------------------

#if defined __LINUX_OS

#include <signal.h>
#include <errno.h>
#include <sys/epoll.h>
#include <sys/signalfd.h>
#include <sys/eventfd.h>
#include <sys/timerfd.h>

#elif defined __BSD_OS

#include <unistd.h>
#include <sys/event.h>
#include <errno.h>
#include <signal.h>

#elif defined _WIN64 || defined _WIN32

#include <ws2tcpip.h>
#include <winsock2.h>
#include <windows.h>
#include <stdio.h>

#endif

#define MAX_EVENTS 64
#define EVENT_IDENT_STOP 1

//-----------------------------------------------------------------------------

#define CAPE_FDTYPE__USER_MANAGED              0
#define CAPE_FDTYPE__TIMER_FD                  1
//-----------------------------------------------------------------------------

struct CapeAioItem_s
{
    void* handle;
    void* user_ptr;

    fct_cape_aio_item__on_event on_event;
    fct_cape_aio_item__on_event on_done;

    int fd_type;
};

//-----------------------------------------------------------------------------

CapeAioItem cape_aio_item_new (void* handle, int fd_type)
{
    CapeAioItem self = CAPE_NEW (struct CapeAioItem_s);

    self->handle = handle;
    self->user_ptr = NULL;

    self->on_event = NULL;
    self->on_done = NULL;

    // this will set the item to be fully user managed
    self->fd_type = fd_type;
    
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

        switch (self->fd_type)
        {
            case CAPE_FDTYPE__TIMER_FD:
            {
    #if defined __LINUX_OS

                close ((int)(number_t)self->handle);
    #endif
                break;
            }
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

int cape_aio_item__handle_fdtype (CapeAioItem self)
{
    switch (self->fd_type)
    {
        case CAPE_FDTYPE__TIMER_FD:
        {
#if defined __LINUX_OS

            uint64_t expirations;

            // we need to read from the timerfd
            ssize_t n = read ((int)(number_t)self->handle, &expirations, sizeof(expirations));

            if (n == sizeof(expirations))
            {
                return TRUE;
            }

            if (n == -1)
            {
                if ((errno == EINTR) || (errno == EAGAIN))
                {
                    return FALSE;
                }
                else
                {
                    CapeErr err = cape_err_new ();

                    // get the last error from the system
                    cape_err_lastOSError (err);

                    // print only the error message
                    cape_log_fmt (CAPE_LL_ERROR, "CAPE", "aio", "can't read from timer FD: %s", cape_err_text (err));

                    cape_err_del (&err);

                    return FALSE;
                }
            }

#endif
            break;
        }
    }

    // default
    return TRUE;
}

//-----------------------------------------------------------------------------

void cape_aio_item__on_event (CapeAioItem self, number_t bytes_affected)
{
    if (self)
    {
        if (cape_aio_item__handle_fdtype (self) && self->on_event)
        {
            self->on_event (self->user_ptr, self->handle);
        }
    }
    else
    {
        cape_log_msg (CAPE_LL_WARN, "CAPE", "aio event", "aio item is NULL");
    }
}

//-----------------------------------------------------------------------------

struct CapeAio_s
{
  int running;          // indicates the running status
  CapeMap items;        // map of all added AIO items
  
#if defined __LINUX_OS

  int epoll_fd;
  int signal_fd;

  int smap[32];         // map for signal handling
  sigset_t sigset;      // signal handling kernel set

#elif defined __BSD_OS

    int kq;
    CapeAioItem stop_item;    // fix a BUG in macosx kevent for NOTE_TRIGGER

#elif defined _WIN64 || defined _WIN32

  HANDLE iocp;

#endif
};

//-----------------------------------------------------------------------------

void __STDCALL cape_aio__items__on_del (void* key, void* val)
{
  CapeAioItem item = key;
  
  cape_aio_item_del (&item);
}

//-----------------------------------------------------------------------------

CapeAio cape_aio_new (void)
{
  CapeAio self = CAPE_NEW (struct CapeAio_s);

  self->running = TRUE;
  self->items = cape_map_new (cape_map__compare__n, cape_aio__items__on_del, NULL);
  
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
    self->stop_item = NULL;

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

    cape_map_del (&(self->items));
    
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

void __STDCALL cape_aio__internal_event_stop__on_event (void* user_ptr, void* handle)
{
    CapeAio self = user_ptr;
  
    // turn of the running status -> terminate wait loop
    self->running = FALSE;
  
    cape_log_msg (CAPE_LL_DEBUG, "CAPE", "aio", "stop AIO loop");
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

int cape_aio__kevent_set (CapeAio self, int fd, int filter, int flags, intptr_t data, void* udata, CapeErr err)
{
    int res;
    struct kevent change;

    // set all options
    EV_SET (&change, fd, filter, flags, 0, data, udata);

    // apply the set by the systemcall to the kevent subsystem
    res = kevent (self->kq, &change, 1, NULL, 0, NULL);

    if (res < 0)
    {
        return cape_err_lastOSError (err);
    }
    else
    {
        return CAPE_ERR_NONE;
    }
}

//-----------------------------------------------------------------------------

int cape_aio__kevent_trigger (CapeAio self, int fd, int filter, void* udata, CapeErr err)
{
    int res;
    struct kevent change;

    // set all options
    EV_SET (&change, fd, filter, 0, NOTE_TRIGGER, 0, udata);

    // apply the set by the systemcall to the kevent subsystem
    res = kevent (self->kq, &change, 1, NULL, 0, NULL);

    if (res < 0)
    {
        return cape_err_lastOSError (err);
    }
    else
    {
        return CAPE_ERR_NONE;
    }
}

//-----------------------------------------------------------------------------

int cape_aio__kevent_add_userevt_handler (CapeAio self, CapeErr err)
{
    // create a new object for the handler
    self->stop_item = cape_aio_item_new (EVENT_IDENT_STOP, CAPE_FDTYPE__USER_MANAGED);
    
    // add for user defined filter
    if (cape_aio__kevent_set (self, EVENT_IDENT_STOP, EVFILT_USER, EV_ADD | EV_CLEAR, 0, self->stop_item, err))
    {
        cape_aio_item_del (&(self->stop_item));
        
        return cape_err_code (err);
    }

    // set callbacks
    cape_aio_item_set (self->stop_item, self, cape_aio__internal_event_stop__on_event, NULL);
    
    // register in map
    cape_map_insert (self->items, (void*)self->stop_item, NULL);

    return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

int cape_aio__kevent_add_signal_handler (CapeAio self, int signal, CapeErr err)
{
    // create a new object for the handler
    CapeAioItem aio_item = cape_aio_item_new (signal, CAPE_FDTYPE__USER_MANAGED);

    // add handler for term signal
    if (cape_aio__kevent_set (self, signal, EVFILT_SIGNAL, EV_ADD, 0, aio_item, err))
    {
        cape_aio_item_del (&aio_item);
        
        return cape_err_code (err);
    }

    cape_aio_item_set (aio_item, self, cape_aio__internal_event_stop__on_event, NULL);
    
    // register in map
    cape_map_insert (self->items, (void*)aio_item, NULL);
    
    return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

#endif

//-----------------------------------------------------------------------------

int cape_aio_block_signals (CapeAio self, CapeErr err)
{
#if defined __LINUX_OS

    int res;

    // we must block the signals for the current thread in order for signals for event to receive them
    res = pthread_sigmask (SIG_BLOCK, &(self->sigset), NULL);
    if (res)
    {
        return cape_err_lastOSError (err);
    }

    return CAPE_ERR_NONE;

#elif defined __BSD_OS

    int res;
    sigset_t sigset;

    // null the sigset
    res = sigemptyset (&sigset);
    if (res == -1)
    {
        return cape_err_lastOSError (err);
    }

    // add this signal to the sigset
    res = sigaddset (&sigset, SIGTERM);
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

    // we must block the signals for the current thread in order for signals for event to receive them
    res = pthread_sigmask (SIG_BLOCK, &sigset, NULL);
    if (res)
    {
        return cape_err_lastOSError (err);
    }
    
    return CAPE_ERR_NONE;

#elif defined _WIN64 || defined _WIN32

    
#endif
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

    // set sigset for handling signals
    self->smap[SIGTERM] = TRUE;
    self->smap[SIGINT] = TRUE;

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

        cape_aio_item_set (aio_item, self, cape_aio__internal_event_stop__on_event, NULL);
    }

#elif defined __BSD_OS

    self->kq = kqueue();

    // check if the open was successful
    if (self->kq == -1)
    {
        return cape_err_lastOSError (err);
    }

    // add user defined events
    if (cape_aio__kevent_add_userevt_handler (self, err))
    {
        return cape_err_code (err);
    }
    
    // add signal handling
    if (cape_aio__kevent_add_signal_handler (self, SIGTERM, err))
    {
        return cape_err_code (err);
    }

    // add signal handling
    if (cape_aio__kevent_add_signal_handler (self, SIGINT, err))
    {
        return cape_err_code (err);
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
  ret = cape_aio_item_new (handle, CAPE_FDTYPE__USER_MANAGED);

#if defined __LINUX_OS

  if (FALSE == cape_aio__epoll_ctl (self, EPOLL_CTL_ADD, (number_t)handle, EPOLLET | EPOLLIN, ret, err))
  {
    cape_aio_item_del (&ret);
  }

#elif defined __BSD_OS

  if (cape_aio__kevent_set (self, (int)(number_t)handle, EVFILT_READ, EV_ADD, 0, ret, err))
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

  if (ret)
  {
    cape_map_insert (self->items, (void*)ret, NULL);
  }

  cape_log_fmt (CAPE_LL_TRACE, "CAPE", "aio add", "new aio item was added {%p} -> %i", ret, (long)handle);

  return ret;
}

//-----------------------------------------------------------------------------

void cape_aio_rm (CapeAio self, CapeAioItem* p_hitem)
{
  CapeAioItem hitem = *p_hitem;

  // remove handle from epoll
  CapeMapNode n = cape_map_find (self->items, (void*)hitem);
  
  if (n)
  {
    CapeErr err = cape_err_new ();

    //cape_log_fmt (CAPE_LL_TRACE, "CAPE", "aio rm", "remove aio item {%p} -> %i", hitem, (long)cape_aio_item_get(hitem));

#if defined __LINUX_OS

    if (FALSE == cape_aio__epoll_ctl (self, EPOLL_CTL_DEL, (int)(number_t)cape_aio_item_get (hitem), 0, NULL, err))
    {

    }

#elif defined __BSD_OS

    if (cape_aio__kevent_set (self, (int)(number_t)cape_aio_item_get (hitem), EVFILT_READ, EV_DELETE, 0, NULL, err))
    {
      cape_log_fmt (CAPE_LL_ERROR, "CAPE", "aio rm", "can't remove kevent item: %s", cape_err_text (err));
    }

#elif defined _WIN64 || defined _WIN32

    if (FALSE)
    {

    }

#endif

    else
    {
      // call user defined shutdown function and
      // cleanup handle event
      cape_map_erase (self->items, n);
      
      // set the return value to NULL
      *p_hitem = NULL;
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
        if (errno == EINTR)
        {
            res = CAPE_ERR_NONE;
            goto cleanup_and_exit;
        }
        else
        {
            res = cape_err_lastOSError (err);
            goto cleanup_and_exit;
        }
    }

    for (i = 0; i < number_of_events; i++)
    {
      //cape_log_fmt (CAPE_LL_TRACE, "QWAVE", "next", "triggered event = %i/%i", i, number_of_events);

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
        //cape_log_fmt (CAPE_LL_TRACE, "QWAVE", "next", "triggered event = %i/%i", i, number_of_events);

        // this handles the event
        cape_aio_item__on_event (event->udata, 0);
      }
    }
  }

  res = CAPE_ERR_NONE;

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
    // deactivate signals
    if (cape_aio_block_signals (self, err))
    {
        return cape_err_code (err);
    }

    while (self->running && (cape_aio_next (self, -1, err) == CAPE_ERR_NONE));

    return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

void cape_aio_stop (CapeAio self)
{
    CapeErr err = cape_err_new ();
    
    // trigger event
#if defined __LINUX_OS

    
#elif defined __BSD_OS

    if (cape_aio__kevent_trigger (self, EVENT_IDENT_STOP, EVFILT_USER, self->stop_item, err))
    {
        cape_log_fmt (CAPE_LL_ERROR, "CAPE", "aio stop", "can't trigger kevent item: %s", cape_err_text (err));
    }

#elif defined _WIN64 || defined _WIN32

#endif
    
    cape_err_del (&err);
}

//-----------------------------------------------------------------------------

void cape_aio_kill (CapeAio self)
{
#if defined __LINUX_OS

  cape_log_fmt (CAPE_LL_TRACE, "CAPE", "aio", "stop loop");
  
  // trigger the terminate signal
  kill (getpid(), SIGTERM);

#elif defined __BSD_OS

  cape_log_fmt (CAPE_LL_TRACE, "CAPE", "aio", "stop loop");
  
  // trigger the terminate signal
  kill (getpid(), SIGTERM);

#elif defined _WIN64 || defined _WIN32

#endif
}

//-----------------------------------------------------------------------------

CapeAioItem cape_aio_add__timer (CapeAio self, number_t interval_in_ms, CapeErr err)
{
    CapeAioItem ret = NULL;
    
#if defined __LINUX_OS

    int fd = timerfd_create (CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    if (fd == -1)
    {
        cape_err_lastOSError (err);
        return NULL;
    }
    
    {
        struct itimerspec its = {0};

        its.it_value.tv_sec  = interval_in_ms / 1000;
        its.it_value.tv_nsec = (interval_in_ms % 1000) * 1000000;

        its.it_interval = its.it_value;   // periodic

        // start it
        if (timerfd_settime (fd, 0, &its, NULL) == -1)
        {
            cape_err_lastOSError (err);
            return NULL;
        }
    }

    // create a new object for the handler
    ret = cape_aio_item_new ((void*)(number_t)fd, CAPE_FDTYPE__TIMER_FD);

    if (FALSE == cape_aio__epoll_ctl (self, EPOLL_CTL_ADD, (number_t)fd, EPOLLET | EPOLLIN, ret, err))
    {
      cape_aio_item_del (&ret);
    }

#elif defined __BSD_OS

    static uintptr_t g_timer_id = 1;
    
    g_timer_id++;
    
    // create a new object for the handler
    ret = cape_aio_item_new ((void*)g_timer_id, CAPE_FDTYPE__TIMER_FD);

    // add for user defined filter
    if (cape_aio__kevent_set (self, (int)g_timer_id, EVFILT_TIMER, EV_ADD | EV_ENABLE, interval_in_ms, ret, err))
    {
        cape_aio_item_del (&ret);
        
        return NULL;
    }

#elif defined _WIN64 || defined _WIN32

    HANDLE timer_handle = CreateWaitableTimer (NULL, FALSE, NULL);
    
    LARGE_INTEGER due;

    // relative, 100-ns units
    due.QuadPart = -(LONGLONG)interval_in_ms * 10000;

    SetWaitableTimer (timer_handle, &due, interval_in_ms, NULL, NULL, FALSE);

    // create a new object for the handler
    ret = cape_aio_item_new ((void*)timer_handle, CAPE_FDTYPE__TIMER_FD);
    
#endif
    
    if (ret)
    {
        cape_map_insert (self->items, (void*)ret, NULL);
    }

    cape_log_fmt (CAPE_LL_TRACE, "CAPE", "aio add", "new aio item was added {%p}", ret);

    return ret;
}

//-----------------------------------------------------------------------------
