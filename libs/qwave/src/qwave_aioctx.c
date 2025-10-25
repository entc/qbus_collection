#include "qwave_aioctx.h"

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
    
}; typedef struct QWaveAioctxEvent_s* QWaveAioctxEvent;

//-----------------------------------------------------------------------------

QWaveAioctxEvent qwave_aioctx_event_new ()
{
    QWaveAioctxEvent self = CAPE_NEW (struct QWaveAioctxEvent_s);
    


    
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

int qwave_aioctx_add (QWaveAioctx self, void** p_handle, CapeErr err)
{
    struct epoll_event event;
    
    // create a new event structure
    event.data.ptr = qwave_aioctx_event_new ();
        
  //  cape_aio_update_events (&event, aioh->hflags);
    
    int s = epoll_ctl (self->epoll_fd, EPOLL_CTL_ADD, (number_t)*p_handle, &event);
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
            
            printf ("can't add fd [%li] to epoll: %s\n", (number_t)*p_handle, cape_err_text (err));
            
            cape_err_del (&err);
        }
        
        return CAPE_ERR_OS;
    }
    
    return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

int qwave_aioctx__handle_event (QWaveAioctx self, struct epoll_event* event, CapeErr err)
{
    
    printf ("EVENT!\n");
    
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

int qwave_aioctx_wait (QWaveAioctx self, CapeErr err)
{
    while (qwave_aioctx_next (self, -1, err) == CAPE_ERR_NONE);
    
    return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

#elif defined __BSD_OS

#include <sys/event.h>

#endif

