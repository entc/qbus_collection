#include "qwave.h"

// cape includes
#include <sys/cape_socket.h>
#include <sys/cape_log.h>
#include <sys/cape_thread.h>
#include <sys/cape_queue.h>
#include <sys/cape_aio.h>

// project includes
#include "qwave_config.h"
#include "qwave_response.h"

//-----------------------------------------------------------------------------

struct QWave_s
{
    CapeString host;
    number_t port;
    
    CapeAio aio;
    
    QWaveConfig config;
    QWaveResponse response;
    
    CapeThread thread;
    CapeQueue queue;
    
    CapeAioItem accept_aio_item;
    
    number_t threads;
    
    fct_qwave__on_ws_message ws_on_message;
    fct_qwave__on_ws_upgrade ws_on_upgrade;
    fct_qwave__on_ws_destroy ws_on_destroy;
    void* ws_user_ptr;
};

//-----------------------------------------------------------------------------

QWave qwave_new (CapeUdc* p_parameters)
{
    CapeUdc parameters = *p_parameters;

    QWave self = CAPE_NEW (struct QWave_s);
        
    self->aio = cape_aio_new ();
    self->config = qwave_config_new ();
    self->response = qwave_response_new (cape_udc_get_s (parameters, "identifier", "qwave"), cape_udc_get_s (parameters, "provider", "cape"));
    
    self->thread = NULL;    
    self->queue = cape_queue_new (10000);  // 10 seconds timeout
    
    self->accept_aio_item = NULL;
    
    // fetch host and port configuration
    self->host = cape_str_cp (cape_udc_get_s (parameters, "host", "127.0.0.1"));    
    self->port = cape_udc_get_n (parameters, "port", 8000);
    
    // apply config to config object
    qwave_config_set (self->config, parameters);
    
    // fetch threads from config
    self->threads = cape_udc_get_n (parameters, "threads", 2);
    
    self->ws_on_message = NULL;
    self->ws_user_ptr = NULL;

    cape_udc_del (p_parameters);
    
    return self;
}

//-----------------------------------------------------------------------------

void qwave_del (QWave* p_self)
{
    if (*p_self)
    {
        QWave self = *p_self;
        
        qwave_stop (self);
        
        if (self->thread)
        {
            cape_log_msg (CAPE_LL_TRACE, "QWAVE", "del", "wait for background process ...");
            
            // wait until the thread terminates
            cape_thread_join (self->thread);

            cape_thread_del (&(self->thread));
        }

        cape_aio_del (&(self->aio));
        
        cape_queue_del (&(self->queue));
        
        qwave_config_del (&(self->config));
        qwave_response_del (&(self->response));
        
        cape_str_del (&(self->host));
        
        CAPE_DEL (p_self, struct QWave_s);
    }
}

//-----------------------------------------------------------------------------

int __STDCALL qwave_server__ws_recv (void* user_ptr, CapeAioItem item)
{
    QWaveConctx ctx = user_ptr;
    
    qwave_conctx_ws_read (ctx);

    return TRUE;
}

//-----------------------------------------------------------------------------

void __STDCALL qwave_server__ws_done (void* user_ptr, CapeAioItem item)
{
    QWaveConctx ctx = user_ptr;
    
    void* handle_remote_connection = cape_aio_item_get (item);
    
    cape_log_fmt (CAPE_LL_DEBUG, "QWAVE", "accept", "connection shutdown on fd [%li]", handle_remote_connection);
    
    // close physical tcp connection
    cape_sock__close (handle_remote_connection);
    
    qwave_conctx_reqdec (&ctx);
}

//-----------------------------------------------------------------------------

void __STDCALL qwave_server__on_upgrade (QWaveConctx ctx, CapeAioItem aio_item)
{
    // TODO: use also the send callback
    cape_aio_item_set (aio_item, (void*)ctx, qwave_server__ws_recv, NULL, qwave_server__ws_done);
}

//-----------------------------------------------------------------------------

int __STDCALL qwave_server__on_request (void* user_ptr, CapeAioItem item)
{
    QWaveConctx ctx = user_ptr;
    
    if (qwave_conctx_read (ctx))
    {
        return TRUE;
    }
    else
    {
        // if read failed we don't need a shutdown
        // TODO: use the return value to close connection
        qwave_conctx_close (ctx, FALSE);

        return FALSE;
    }
}

//-----------------------------------------------------------------------------

void __STDCALL qwave_server__on_drop (void* user_ptr, CapeAioItem item)
{
    QWaveConctx ctx = user_ptr;
    
    void* handle_remote_connection = cape_aio_item_get (item);

    cape_log_fmt (CAPE_LL_DEBUG, "QWAVE", "accept", "connection shutdown on fd [%li]", handle_remote_connection);

    // close physical tcp connection
    cape_sock__close (handle_remote_connection);
    
    qwave_conctx_reqdec (&ctx);
}

//-----------------------------------------------------------------------------

void qwave_factory_conctx (QWave self, void* handle_remote_connection, const CapeString remote_address)
{    
    cape_log_fmt (CAPE_LL_DEBUG, "QWAVE", "accept", "new connection from '%s' on fd [%lu]", remote_address, handle_remote_connection);
    
    {
        CapeErr err = cape_err_new();
        
        // handle only receive by the AIO
        CapeAioItem aio_item = cape_aio_add (self->aio, handle_remote_connection, CAPE_AIO_MODE__RECV, err);
        
        if (NULL == aio_item)
        {
            
        }
        else
        {
            QWaveConctx conctx = qwave_conctx_new (self->config, self->response, self->queue, self->aio, aio_item, remote_address, qwave_server__on_upgrade);
        
            // set the callbacks
            // transfer the responsiblity of the ownership of conctx to
            // the AIO system, qwave_server__on_drop will be called
            cape_aio_item_set (aio_item, conctx, qwave_server__on_request, NULL, qwave_server__on_drop);
            
            // set the callbacks
            qwave_conctx_ws_cb (conctx, self->ws_user_ptr, self->ws_on_upgrade, self->ws_on_message, self->ws_on_destroy);
        }
        
        cape_err_del (&err);        
    }
}

//-----------------------------------------------------------------------------

int __STDCALL qwave_server__on_accept (void* user_ptr, CapeAioItem item)
{
    QWave self = user_ptr;

    // local objects
    CapeErr err = cape_err_new ();
    CapeString remote_address = NULL;
    
    while (TRUE)
    {
        // try to gather a new connection handle
        void* handle_remote_connection = cape_sock__accept (cape_aio_item_get (item), &remote_address, err);
        
        if (NULL == handle_remote_connection)
        {
            break;
        }
        else
        {
            // set none blocking
            if (cape_sock__noneblocking (handle_remote_connection, err))
            {
                // error
            }
            else
            {
                qwave_factory_conctx (self, handle_remote_connection, remote_address);
            }
        }
    }
    
    cape_err_del (&err);
    cape_str_del (&remote_address);

    return TRUE;
}

//-----------------------------------------------------------------------------

void __STDCALL qwave_server__on_shutdown (void* user_ptr, CapeAioItem item)
{
    QWave self = user_ptr;
    
    cape_sock__close (cape_aio_item_get (item));
}

//-----------------------------------------------------------------------------

int qwave_init (QWave self, CapeErr err)
{
    int res;
    
    // local objects
    void* socket_handle = NULL;
    
    // open the event file descriptor
    res = cape_aio_init (self->aio, err);
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
    
    res = cape_sock__noneblocking (socket_handle, err);
    if (res)
    {
        goto cleanup_and_exit;
    }
    
    // attach the socket handle to the AIO controller
    self->accept_aio_item = cape_aio_add (self->aio, socket_handle, CAPE_AIO_MODE__RECV, err);
    
    if (NULL == self->accept_aio_item)
    {
        res = cape_err_code (err);
        goto cleanup_and_exit;
    }
    else
    {
        socket_handle = NULL;  
    }

    // set the callbacks
    cape_aio_item_set (self->accept_aio_item, self, qwave_server__on_accept, NULL, qwave_server__on_shutdown);
    
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
    
    return cape_aio_wait (self->aio, err);
}

//-----------------------------------------------------------------------------

int __STDCALL qwave__worker (void* ptr)
{
    QWave self = ptr;
    
    // local objects
    CapeErr err = cape_err_new ();
    
    if (cape_aio_wait (self->aio, err))
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

void qwave_stop (QWave self)
{
    cape_aio_stop (self->aio);
}

//-----------------------------------------------------------------------------

void qwave_reg__path (QWave self, const CapeString path, void* user_ptr, fct_qwave__on_http_request fct)
{
    
}

//-----------------------------------------------------------------------------

void qwave_reg__ws (QWave self, void* user_ptr, fct_qwave__on_ws_upgrade on_upgrade, fct_qwave__on_ws_message on_message, fct_qwave__on_ws_destroy on_destroy)
{
    self->ws_user_ptr = user_ptr;
    
    self->ws_on_upgrade = on_upgrade;
    self->ws_on_message = on_message;
    self->ws_on_destroy = on_destroy;
}

//-----------------------------------------------------------------------------
