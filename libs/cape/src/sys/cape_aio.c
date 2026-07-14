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

//-----------------------------------------------------------------------------

#endif

#define MAX_EVENTS 64
#define EVENT_IDENT_STOP 1

//-----------------------------------------------------------------------------

#define CAPE_FDTYPE__USER_MANAGED              0
#define CAPE_FDTYPE__TIMER_FD                  1

//-----------------------------------------------------------------------------

typedef struct CapeAioTimerCtx_s* CapeAioTimerCtx;

void  cape_aio_timer__del (CapeAioTimerCtx* p_self);

//-----------------------------------------------------------------------------

struct CapeAioItem_s
{
    void* handle;
    void* user_ptr;

    fct_cape_aio_item__on_event on_recv;
    fct_cape_aio_item__on_event on_send;

    fct_cape_aio_item__on_done on_done;

    int fd_type;
    int mode_applied;
};

//-----------------------------------------------------------------------------

CapeAioItem cape_aio_item_new (void* handle, int fd_type)
{
    CapeAioItem self = CAPE_NEW (struct CapeAioItem_s);

    self->handle = handle;
    self->user_ptr = NULL;

    self->on_recv = NULL;
    self->on_send = NULL;
    self->on_done = NULL;

    // this will set the item to be fully user managed
    self->fd_type = fd_type;
    self->mode_applied = 0;
    
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
          self->on_done (self->user_ptr, self);
        }

        switch (self->fd_type)
        {
            case CAPE_FDTYPE__TIMER_FD:
            {
 #if defined __LINUX_OS

                close ((int)(number_t)self->handle);

 #elif defined _WIN64 || defined _WIN32

                cape_aio_timer__del(&(self->handle));

 #endif
                break;
            }
        }

        CAPE_DEL (p_self, struct CapeAioItem_s);
    }
}

//-----------------------------------------------------------------------------

void cape_aio_item_set (CapeAioItem self, void* user_ptr, fct_cape_aio_item__on_event on_recv, fct_cape_aio_item__on_event on_send, fct_cape_aio_item__on_done on_done)
{
  self->user_ptr = user_ptr;

  self->on_recv = on_recv;
  self->on_send = on_send;
  self->on_done = on_done;
}

//-----------------------------------------------------------------------------

void* cape_aio_item_get (CapeAioItem self)
{
  return self->handle;
}

//-----------------------------------------------------------------------------

int cape_aio_item__handle_fdtype__recv (CapeAioItem self)
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

int cape_aio_item__on_event (CapeAioItem self, int mode, number_t bytes_affected)
{
    if (self)
    {
        switch (mode)
        {
            case CAPE_AIO_MODE__RECV:
            {
                if (cape_aio_item__handle_fdtype__recv (self) && self->on_recv)
                {
                    return self->on_recv (self->user_ptr, self);
                }

                break;
            }
            case CAPE_AIO_MODE__SEND:
            {
                if (self->on_send)
                {
                    return self->on_send (self->user_ptr, self);
                }

                break;
            }
            case CAPE_AIO_MODE__TIMER:
            {
                if (cape_aio_item__handle_fdtype__recv (self) && self->on_recv)
                {
                    return self->on_recv (self->user_ptr, self);
                }

                break;
            }
        }
    }
    else
    {
        cape_log_msg (CAPE_LL_WARN, "CAPE", "aio event", "aio item is NULL");
    }

    return TRUE;
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

    {
        // transfer ownership to local variable
        // deleting of members of the map might result in new aio_add calls
        // to prevent that events will be added in destructor phase
        // there is a check for self->items
        CapeMap items = cape_map_mv (&(self->items));

        cape_map_del (&items);
    }

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

int __STDCALL cape_aio__internal_event_stop__on_event (void* user_ptr, CapeAioItem item)
{
    CapeAio self = user_ptr;
  
    // turn of the running status -> terminate wait loop
    self->running = FALSE;
  
    cape_log_msg (CAPE_LL_DEBUG, "CAPE", "aio", "stop AIO loop");

    return TRUE;
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
    int res;
    
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
            res = cape_err_set (err, CAPE_ERR_OS, "this filedescriptor is not supported by epoll");
          
            cape_log_msg (CAPE_LL_ERROR, "QWAVE", "epoll", cape_err_text (err));
        }
        else
        {
            res = cape_err_lastOSError (err);
            
            cape_log_fmt (CAPE_LL_ERROR, "QWAVE", "epoll", "can't use fd [%li] in epoll: %s", fd, cape_err_text (err));
        }
    }
    else
    {
        res = CAPE_ERR_NONE;
    }

    return res;
}

//-----------------------------------------------------------------------------

int cape_aio__epoll_convert_mode (int mode)
{
    // enable edge triggered events
    //int ret = EPOLLET;

    // don't use edge triggered events to have same behaviour as for kevent on BSD
    // all write operations must be removed after complete
    int ret = 0;
    
    if (mode & CAPE_AIO_MODE__RECV)
    {
        ret |= EPOLLIN;
    }

    if (mode & CAPE_AIO_MODE__SEND)
    {
        ret |= EPOLLOUT;
    }

    if (mode & CAPE_AIO_MODE__TIMER)
    {
        ret |= EPOLLIN;
    }

    return ret;
}

//-----------------------------------------------------------------------------

void cape_aio__event_process (CapeAio self, struct epoll_event* event)
{
    CapeAioItem item = event->data.ptr;

    if (event->events & EPOLLERR)
    {
        cape_aio_rm__item (self, &item);

        cape_log_fmt (CAPE_LL_ERROR, "CAPE", "aio next", "error on handler");
        return;
    }

    //cape_log_fmt (CAPE_LL_TRACE, "CAPE", "event", "process filter = (%d)", event->events);

    {
        int marked_for_remove = FALSE;
        int i;

        // helper struct to iterate through all event types
        static const struct {uint32_t epoll_event; int aio_mode; } handlers[] = {{EPOLLIN, CAPE_AIO_MODE__RECV}, {EPOLLOUT, CAPE_AIO_MODE__SEND}};

        // run a small loop to check all possible event types
        for (i = 0; i < 2; ++i)
        {
            // is this event type part of the event?
            if (event->events & handlers[i].epoll_event)
            {
                // this handles the event
                if (FALSE == cape_aio_item__on_event (item, handlers[i].aio_mode, 0))
                {
                    marked_for_remove = TRUE;
                }
            }
        }

        if (marked_for_remove)
        {
            cape_aio_rm__item (self, &item);
        }
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
    cape_aio_item_set (self->stop_item, self, cape_aio__internal_event_stop__on_event, NULL, NULL);
    
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

    cape_aio_item_set (aio_item, self, cape_aio__internal_event_stop__on_event, NULL, NULL);
    
    // register in map
    cape_map_insert (self->items, (void*)aio_item, NULL);
    
    return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

int cape_aio__kevent_convert_filter (int filter)
{
    switch (filter)
    {
        case EVFILT_READ:
        {
            return CAPE_AIO_MODE__RECV;
        }
        case EVFILT_WRITE:
        {
            return CAPE_AIO_MODE__SEND;
        }
        case EVFILT_TIMER:
        {
            return CAPE_AIO_MODE__TIMER;
        }
        case EVFILT_USER:
        case EVFILT_SIGNAL:
        {
            return CAPE_AIO_MODE__RECV;
        }
    }
    
    return 0;
}

//-----------------------------------------------------------------------------

void cape_aio__event_process (CapeAio self, struct kevent* event)
{
    if (event->flags & EV_ERROR)
    {
        CapeErr err = cape_err_new ();
        
        cape_err_formatErrorOS (err, (int)event->data);

        cape_log_fmt (CAPE_LL_ERROR, "CAPE", "aio next", "aio error: %s", cape_err_text (err));

        cape_err_del (&err);
        return;
    }

    cape_log_fmt (CAPE_LL_TRACE, "CAPE", "event", "process filter = (%d) flags = {0x%x}", event->filter, event->flags);

    {
        CapeAioItem item = event->udata;
        
        // convert from kevent filter to cape mode
        int mode = cape_aio__kevent_convert_filter (event->filter);
        
        // trigger the event
        int marked_for_remove = mode && (FALSE == cape_aio_item__on_event(item, mode, 0));

        if (marked_for_remove || ((event->filter == EVFILT_READ) && (event->flags & EV_EOF)))
        {
            cape_log_msg (CAPE_LL_TRACE, "CAPE", "aio next", "close event item");

            cape_aio_rm__item (self, &item);
        }
    }
}

//-----------------------------------------------------------------------------

#elif defined _WIN64 || defined _WIN32

//-----------------------------------------------------------------------------

struct CapeAioTimerCtx_s
{
    CapeAio aio;           // reference
    PTP_TIMER timer;       // owned

};

//-----------------------------------------------------------------------------

CapeAioTimerCtx cape_aio_timer__new(CapeAio aio)
{
    CapeAioTimerCtx self = CAPE_NEW(struct CapeAioTimerCtx_s);

    self->aio = aio;
    self->timer = NULL;

    return self;
}

//-----------------------------------------------------------------------------

void  cape_aio_timer__del(CapeAioTimerCtx* p_self)
{
    if (*p_self)
    {
        CapeAioTimerCtx self = *p_self;

        if (self->timer)
        {
            // stop
            SetThreadpoolTimer(self->timer, NULL, 0, 0);

            // wait for callbacks
            WaitForThreadpoolTimerCallbacks(self->timer, TRUE);

            // close
            CloseThreadpoolTimer(self->timer);
        }

        CAPE_DEL(p_self, struct CapeAioTimerCtx_s);
    }
}

//-----------------------------------------------------------------------------

VOID CALLBACK cape_aio_timer__on_threadpool(PTP_CALLBACK_INSTANCE instance, PVOID context, PTP_TIMER timer)
{
    CapeAioItem item = (CapeAioItem)context;

    CapeAioTimerCtx self = cape_aio_item_get(item);

    if (!PostQueuedCompletionStatus(self->aio->iocp, 0, (ULONG_PTR)item, NULL))
    {
        CapeErr err = cape_err_new();

        // get the last error from the system
        cape_err_lastOSError(err);

        // print only the error message
        cape_log_fmt(CAPE_LL_ERROR, "CAPE", "aio", "can't queue timer event for IOCP: %s", cape_err_text(err));

        cape_err_del(&err);
    }
}

//-----------------------------------------------------------------------------

int cape_aio_timer__init(CapeAioTimerCtx self, CapeAioItem item, number_t interval_in_ms, CapeErr err)
{
    // create a new background timer
    self->timer = CreateThreadpoolTimer(cape_aio_timer__on_threadpool, item, NULL);

    if (NULL == self->timer)
    {
        return cape_err_lastOSError(err);
    }

    {
        FILETIME due;

        ULONGLONG t = (ULONGLONG)-((LONGLONG)interval_in_ms * 10000);

        due.dwLowDateTime = (DWORD)t;
        due.dwHighDateTime = (DWORD)(t >> 32);

        SetThreadpoolTimer(self->timer, &due, (DWORD)interval_in_ms, 0);
    }

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

    return CAPE_ERR_NONE;

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
        CapeAioItem aio_item = cape_aio_add (self, (void*)(number_t)self->signal_fd, CAPE_AIO_MODE__RECV, err);

        if (NULL == aio_item)
        {
          // TODO: cleanup file descriptors
        }

        cape_aio_item_set (aio_item, self, cape_aio__internal_event_stop__on_event, NULL, NULL);
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

CapeAioItem cape_aio_add (CapeAio self, void* handle, int inital_mode, CapeErr err)
{
    CapeAioItem ret;

    if (NULL == self->items)
    {
        return NULL;
    }

    // create a new object for the handler
    ret = cape_aio_item_new (handle, CAPE_FDTYPE__USER_MANAGED);

    if (cape_aio_set__mode (self, ret, inital_mode, err))
    {
        cape_aio_item_del (&ret);
    }
    
    if (ret)
    {
      cape_map_insert (self->items, (void*)ret, NULL);
    }

    cape_log_fmt (CAPE_LL_TRACE, "CAPE", "aio add", "new aio item was added {%p} -> %lu", ret, (number_t)handle);

    return ret;
}

//-----------------------------------------------------------------------------

CapeAioItem cape_aio_add__timer (CapeAio self, number_t interval_in_ms, CapeErr err)
{
    CapeAioItem item = NULL;
    
    if (NULL == self->items)
    {
        return NULL;
    }

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
    item = cape_aio_item_new ((void*)(number_t)fd, CAPE_FDTYPE__TIMER_FD);

    if (cape_aio_set__mode (self, item, CAPE_AIO_MODE__TIMER, err))
    {
        cape_aio_item_del (&item);
        return NULL;
    }

#elif defined __BSD_OS

    static uintptr_t g_timer_id = 1;
    
    g_timer_id++;
    
    // create a new object for the handler
    item = cape_aio_item_new ((void*)g_timer_id, CAPE_FDTYPE__TIMER_FD);

    // add
    if (cape_aio__kevent_set (self, (int)(number_t)item->handle, EVFILT_TIMER, EV_ADD | EV_ENABLE, interval_in_ms, item, err))
    {
        // return the error
        return cape_err_code (err);
    }
    
    item->mode_applied = CAPE_AIO_MODE__TIMER;

#elif defined _WIN64 || defined _WIN32

    // create a new timer
    CapeAioTimerCtx ctx = cape_aio_timer__new(self);

    // create a new object for the handler
    ret = cape_aio_item_new ((void*)ctx, CAPE_FDTYPE__TIMER_FD);

    if (cape_aio_timer__init (ctx, ret, interval_in_ms, err))
    {
        cape_log_fmt(CAPE_LL_ERROR, "CAPE", "aio", "can't initialize timer: %s", cape_err_text(err));

        // takes care of the ctx instance
        cape_aio_item_del(&ret);

        return NULL;
    }

#endif
    
    if (item)
    {
        cape_map_insert (self->items, (void*)item, NULL);
    }

    cape_log_fmt (CAPE_LL_TRACE, "CAPE", "aio add", "new aio item was added {%p}", item);

    return item;
}

//-----------------------------------------------------------------------------

void cape_aio_rm__item (CapeAio self, CapeAioItem* p_hitem)
{
    CapeAioItem hitem = *p_hitem;

    // remove handle from epoll
    CapeMapNode n = cape_map_find (self->items, (void*)hitem);
    
    if (n)
    {
        CapeErr err = cape_err_new ();

        //cape_log_fmt (CAPE_LL_TRACE, "CAPE", "aio rm", "remove aio item {%p} -> %i", hitem, (long)cape_aio_item_get(hitem));

        // remove all event handling
        if (cape_aio_set__mode (self, hitem, 0, err))
        {
            // error
        }
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

int cape_aio_set__mode (CapeAio self, CapeAioItem item, int mode, CapeErr err)
{
    // already all modes where applied
    if (item->mode_applied == mode)
    {
        return CAPE_ERR_NONE;
    }

#if defined __LINUX_OS

    if (cape_aio__epoll_ctl (self, mode == 0 ? EPOLL_CTL_DEL : (item->mode_applied == 0 ? EPOLL_CTL_ADD : EPOLL_CTL_MOD), (int)(number_t)item->handle, cape_aio__epoll_convert_mode (mode), item, err))
    {
        // return the error
        return cape_err_code (err);
    }

    item->mode_applied = mode;
    
#elif defined __BSD_OS

    if ((mode & CAPE_AIO_MODE__RECV) && !(item->mode_applied & CAPE_AIO_MODE__RECV))
    {
        // add
        if (cape_aio__kevent_set (self, (int)(number_t)item->handle, EVFILT_READ, EV_ADD, 0, item, err))
        {
            // return the error
            return cape_err_code (err);
        }
        
        item->mode_applied |= CAPE_AIO_MODE__RECV;
    }
    
    if ((mode & CAPE_AIO_MODE__SEND) && !(item->mode_applied & CAPE_AIO_MODE__SEND))
    {
        // add
        if (cape_aio__kevent_set (self, (int)(number_t)item->handle, EVFILT_WRITE, EV_ADD, 0, item, err))
        {
            // return the error
            return cape_err_code (err);
        }
        
        item->mode_applied |= CAPE_AIO_MODE__SEND;
    }
    
    if ((mode & CAPE_AIO_MODE__TIMER) && !(item->mode_applied & CAPE_AIO_MODE__TIMER))
    {
        return cape_err_set (err, CAPE_ERR_NOT_SUPPORTED, "timer can not be changed");
    }

    if (!(mode & CAPE_AIO_MODE__RECV) && (item->mode_applied & CAPE_AIO_MODE__RECV))
    {
        // delete
        if (cape_aio__kevent_set (self, (int)(number_t)item->handle, EVFILT_READ, EV_DELETE, 0, item, err))
        {
            // return the error
            return cape_err_code (err);
        }
        
        item->mode_applied &= ~CAPE_AIO_MODE__RECV;
    }
    
    if (!(mode & CAPE_AIO_MODE__SEND) && (item->mode_applied & CAPE_AIO_MODE__SEND))
    {
        // delete
        if (cape_aio__kevent_set (self, (int)(number_t)item->handle, EVFILT_WRITE, EV_DELETE, 0, item, err))
        {
            // return the error
            return cape_err_code (err);
        }

        item->mode_applied &= ~CAPE_AIO_MODE__SEND;
    }

    if (!(mode & CAPE_AIO_MODE__TIMER) && (item->mode_applied & CAPE_AIO_MODE__TIMER))
    {
        // delete
        if (cape_aio__kevent_set (self, (int)(number_t)item->handle, EVFILT_TIMER, EV_DELETE, 0, item, err))
        {
            // return the error
            return cape_err_code (err);
        }

        item->mode_applied &= ~CAPE_AIO_MODE__TIMER;
    }

#elif defined _WIN64 || defined _WIN32

    if (item->mode_applied == 0 && mode != 0)
    {
        // returns the handle, the handle is only used for error identification
        // the handle don't have to be stored for a free later, this is managed internally in winapi
        if (NULL == CreateIoCompletionPort (item->handle, self->iocp, (ULONG_PTR)item, 0))
        {
            return cape_err_lastOSError (err);
        }
    }
    else if (item->mode_applied != 0 && mode != 0)
    {
        // do nothing here
    }
    else
    {
        // cannot be removed
    }
    
    item->mode_applied = mode;
    
#endif
    
    return CAPE_ERR_NONE;
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
        cape_aio__event_process (self, &events[i]);
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
                        
            cape_aio__event_process (self, event);
        }
    }

    res = CAPE_ERR_NONE;

#elif defined _WIN64 || defined _WIN32

    OVERLAPPED_ENTRY overlappeds[MAX_EVENTS];
    ULONG count;

    // wait for any event on the completion port
    if (GetQueuedCompletionStatusEx(self->iocp, overlappeds, MAX_EVENTS, &count, (DWORD)timeout_in_ms, TRUE))
    {
        ULONG i;

        for (i = 0; i < count; ++i)
        {
            cape_aio_item__on_event ((CapeAioItem)overlappeds[i].lpCompletionKey, overlappeds[i].dwNumberOfBytesTransferred);
        }

        res = CAPE_ERR_NONE;
    }
    else
    {
        DWORD last_err_code = GetLastError();

        if (last_err_code == WAIT_TIMEOUT)
        {
            res = CAPE_ERR_NONE;
        }
        else
        {
            res = cape_err_formatErrorOS(err, last_err_code);

            cape_log_fmt(CAPE_LL_ERROR, "CAPE", "aio next", "aio error: %s", cape_err_text(err));
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
