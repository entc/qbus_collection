#include "qwave.h"

// cape includes
#include <sys/cape_socket.h>
#include <sys/cape_log.h>
#include <sys/cape_thread.h>
#include <sys/cape_queue.h>

// project includes
#include "qwave_aioctx.h"
#include "qwave_config.h"
#include "qwave_response.h"

//-----------------------------------------------------------------------------

struct QWave_s
{
    CapeString host;
    number_t port;
    
    QWaveAioctx aioctx;
    QWaveConfig config;
    QWaveResponse response;
    
    CapeThread thread;
    CapeQueue queue;
    
    QWaveAioctxEvent accept_event_handler;
    
    number_t threads;
};

//-----------------------------------------------------------------------------

QWave qwave_new (CapeUdc parameters)
{
    QWave self = CAPE_NEW (struct QWave_s);
        
    self->aioctx = qwave_aioctx_new ();
    self->config = qwave_config_new ();
    self->response = qwave_response_new ();
    
    self->thread = NULL;    
    self->queue = cape_queue_new (10000);  // 10 seconds timeout
    
    self->accept_event_handler = NULL;
    
    // fetch host and port configuration
    self->host = cape_str_cp (cape_udc_get_s (parameters, "h", "127.0.0.1"));    
    self->port = cape_udc_get_n (parameters, "p", 8000);
    
    // apply config to config object
    qwave_config_set (self->config, parameters);
    
    // fetch threads from config
    self->threads = cape_udc_get_n (parameters, "threads", 2);
    
    return self;
}

//-----------------------------------------------------------------------------

void qwave_del (QWave* p_self)
{
    if (*p_self)
    {
        QWave self = *p_self;
        
        cape_queue_del (&(self->queue));
        
        if (self->thread)
        {
            // wait until the thread terminates
            cape_thread_join (self->thread);

            cape_thread_del (&(self->thread));
        }
        
        qwave_config_del (&(self->config));
        qwave_aioctx_del (&(self->aioctx));
        qwave_response_del (&(self->response));
        
        cape_str_del (&(self->host));
        
        CAPE_DEL (p_self, struct QWave_s);
    }
}

//-----------------------------------------------------------------------------

int __STDCALL qwave_server__on_request (void* user_ptr, void* handle_remote_connection)
{
    QWaveConctx ctx = user_ptr;
    
    if (qwave_conctx_read (ctx))
    {
        
    }
    else
    {
        // terminate connection
        cape_log_fmt (CAPE_LL_DEBUG, "QWAVE", "accept", "drop connection on fd [%lu]", handle_remote_connection);

        qwave_conctx_close (ctx);
    }
    
    return QWAVE_EVENT_RESULT__CONTINUE;
}

//-----------------------------------------------------------------------------

void qwave_factory_conctx (QWave self, void* handle_remote_connection, const CapeString remote_address)
{    
    cape_log_fmt (CAPE_LL_DEBUG, "QWAVE", "accept", "new connection from '%s' on fd [%lu]", remote_address, handle_remote_connection);
    
    {
        CapeErr err = cape_err_new();
        
        QWaveAioctxEvent eh = qwave_aioctx_add (self->aioctx, &handle_remote_connection, err);
        if (NULL == eh)
        {
            
        }
        else
        {
            QWaveConctx conctx = qwave_conctx_new (self->config, self->response, self->queue, eh, remote_address);
        
            // set the callbacks
            qwave_aioctx_event_set (eh, conctx, qwave_server__on_request);
        }
        
        cape_err_del (&err);        
    }
}

//-----------------------------------------------------------------------------

int __STDCALL qwave_server__on_accept (void* user_ptr, void* handle)
{
    int ret;
    QWave self = user_ptr;

    // local objects
    CapeErr err = cape_err_new ();
    CapeString remote_address = NULL;
    
    // try to gather a new connection handle
    void* handle_remote_connection = cape_sock__accept (handle, &remote_address, err);

    if (NULL == handle_remote_connection)
    {
        ret = (cape_err_code (err) == CAPE_ERR_CONTINUE) ? QWAVE_EVENT_RESULT__TRYAGAIN : QWAVE_EVENT_RESULT__ERROR_CLOSED;
    }
    else
    {
        qwave_factory_conctx (self, handle_remote_connection, remote_address);
        
        ret = QWAVE_EVENT_RESULT__CONTINUE;
    }

    cape_err_del (&err);
    cape_str_del (&remote_address);
    
    return ret;
}

//-----------------------------------------------------------------------------

int qwave_init (QWave self, CapeErr err)
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
    
    // attach the socket handle to the AIO controller
    self->accept_event_handler = qwave_aioctx_add (self->aioctx, &socket_handle, err);
    
    // set the callbacks
    qwave_aioctx_event_set (self->accept_event_handler, self, qwave_server__on_accept);
        
    if (NULL == self->accept_event_handler)
    {
        res = cape_err_code (err);
        goto cleanup_and_exit;
    }
    
    res = cape_queue_start (self->queue, self->threads, err);
    
cleanup_and_exit:
    
    if (socket_handle)
    {
        cape_sock__close (socket_handle);
    }
    
    return res;    
}

//-----------------------------------------------------------------------------

int qwave_run (QWave self, CapeErr err)
{
    int res;
    
    res = qwave_init (self, err);
    if (res)
    {
        return res;
    }
    
    return qwave_aioctx_wait (self->aioctx, err);
}

//-----------------------------------------------------------------------------

int __STDCALL qwave__worker (void* ptr)
{
    QWave self = ptr;
    
    // local objects
    CapeErr err = cape_err_new ();
    
    if (qwave_aioctx_wait (self->aioctx, err))
    {
        cape_log_fmt (CAPE_LL_WARN, "QWAVE", "wait", "stopped waiting: %s", cape_err_text (err));
    }

    cape_err_del (&err);
    
    // terminate thread
    return FALSE;
}

//-----------------------------------------------------------------------------

int qwave_run__d (QWave self, CapeErr err)
{
    // initialize qwebs
    {
        int res;
        
        res = qwave_init (self, err);
        if (res)
        {
            return res;
        }
    }
    
    // allocate memory for the thread
    self->thread = cape_thread_new ();
    
    // start the thread
    cape_thread_start (self->thread, qwave__worker, self);
    
    return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

int qwave_stop (QWave self, CapeErr err)
{
    return qwave_aioctx_kill (self->aioctx, err);
}

//-----------------------------------------------------------------------------

void qwave_reg__path (QWave self, const CapeString path, void* user_ptr, fct_qwave__on_http_request fct)
{
    
}

//-----------------------------------------------------------------------------
