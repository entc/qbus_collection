#include "qwave.h"

// cape includes
#include <sys/cape_socket.h>

// project includes
#include "qwave_aioctx.h"

//-----------------------------------------------------------------------------

struct QWave_s
{
    CapeString host;
    number_t port;
    
    QWaveAioctx aioctx;
};

//-----------------------------------------------------------------------------

QWave qwave_new (const CapeString host, number_t port, CapeUdc parameters)
{
    QWave self = CAPE_NEW (struct QWave_s);
    
    self->host = cape_str_cp (host);
    self->port = port;
    
    self->aioctx = qwave_aioctx_new ();
    
    return self;
}

//-----------------------------------------------------------------------------

void qwave_del (QWave* p_self)
{
    if (*p_self)
    {
        QWave self = *p_self;
        
        qwave_aioctx_del (&(self->aioctx));
        
        cape_str_del (&(self->host));
        
        CAPE_DEL (p_self, struct QWave_s);
    }
}

//-----------------------------------------------------------------------------

/*
int qwave__events__sigmask (QWave self, sigset_t* sigset)
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
*/

//-----------------------------------------------------------------------------

int qwave_run (QWave self, CapeErr err)
{
    int res;
    
    // local objects
    void* socket_handle = NULL;
    
    // open the event file descriptor
    res = qwave_aioctx_open (self->aioctx, err);
    if (res)
    {
        goto cleanup_and_exit;
    }

    // open the server socket
    socket_handle = cape_sock__tcp__srv_new (self->host, self->port, err);
    if (NULL == socket_handle)
    {        
        goto cleanup_and_exit;
    }
    
    res = qwave_aioctx_add (self->aioctx, &socket_handle, err);
    if (res)
    {
        goto cleanup_and_exit;
    }
    
    res = qwave_aioctx_wait (self->aioctx, err);
    if (res)
    {
        goto cleanup_and_exit;
    }
    
    res = CAPE_ERR_NONE;
    
cleanup_and_exit:
        
    if (socket_handle)
    {
        cape_sock__close (socket_handle);
    }
    
    return res;
}

//-----------------------------------------------------------------------------

int qwave_run__d (QWave self, CapeErr err)
{
    
}

//-----------------------------------------------------------------------------

int qwave_wait (QWave self, CapeErr err)
{
    
}

//-----------------------------------------------------------------------------

void qwave_reg__path (QWave self, const CapeString path, void* user_ptr, fct_qwave__on_http_request fct)
{
    
}

//-----------------------------------------------------------------------------
