#include "qsock_client.h"

// cape includes
#include <sys/cape_socket.h>
#include <sys/cape_log.h>
#include <sys/cape_thread.h>
#include <sys/cape_queue.h>
#include <sys/cape_aio.h>

#define QSOCK_BUFFER_RECV_SIZE    1024

//-----------------------------------------------------------------------------

int __STDCALL qsock_client__on_recv (void* user_ptr, CapeAioItem item);
int __STDCALL qsock_client__on_send (void* user_ptr, CapeAioItem item);
void __STDCALL qsock_client__on_shutdown (void* user_ptr, CapeAioItem item);

//-----------------------------------------------------------------------------

struct QSockClient_s
{
    CapeString host;
    int port;

    CapeAio aio;
    CapeAioItem aio_item;
    
    CapeAioItem aio_timer;
    
    CapeStream buf_recv;
    
    void* user_ptr;
    fct_qsock_client__on_recv on_recv;
};

//-----------------------------------------------------------------------------

QSockClient qsock_client_new (const CapeString host, int port)
{
    QSockClient self = CAPE_NEW (struct QSockClient_s);

    self->host = cape_str_cp (host);
    self->port = port;

    self->aio = cape_aio_new ();
    self->aio_item = NULL;
    
    self->aio_timer = NULL;
    
    self->buf_recv = cape_stream_new ();
    cape_stream_cap (self->buf_recv, QSOCK_BUFFER_RECV_SIZE);
    
    self->user_ptr = NULL;
    self->on_recv = NULL;
    
    return self;
}

//-----------------------------------------------------------------------------

void qsock_client_del (QSockClient* p_self)
{
    if (*p_self)
    {
        QSockClient self = *p_self;

        cape_stream_del (&(self->buf_recv));
        
        cape_aio_del (&(self->aio));
        cape_str_del (&(self->host));

        CAPE_DEL (p_self, struct QSockClient_s);
    }
}

//-----------------------------------------------------------------------------

int qsock_client__create_socket (QSockClient self, CapeErr err)
{
    int res;
    
    void* handle;
    
    handle = cape_sock__tcp__clt_new (self->host, self->port, err);
    if (NULL == handle)
    {
        res = cape_err_code (err);
        goto cleanup_and_exit;
    }
    
    // attach the socket handle to the AIO controller
    // start with send mode first
    self->aio_item = cape_aio_add (self->aio, handle, CAPE_AIO_MODE__SEND, err);
    
    if (NULL == self->aio_item)
    {
        res = cape_err_code (err);
        goto cleanup_and_exit;
    }
    else
    {
        res = CAPE_ERR_NONE;
        handle = NULL;
    }
    
    // set the callbacks
    cape_aio_item_set (self->aio_item, self, qsock_client__on_recv, qsock_client__on_send, qsock_client__on_shutdown);
    
cleanup_and_exit:
    
    if (handle)
    {
        cape_sock__close (handle);
    }

    return res;
}

//-----------------------------------------------------------------------------

int __STDCALL qsock_client__on_timer (void* user_ptr, CapeAioItem item)
{
    QSockClient self = user_ptr;

    // local objects
    CapeErr err = cape_err_new ();
    
    cape_log_msg (CAPE_LL_DEBUG, "QSOCK", "timer", "try to reconnect");
    
    if (qsock_client__create_socket (self, err))
    {
        
    }

    cape_err_del (&err);

    return FALSE;
}

//-----------------------------------------------------------------------------

void qsock_client__start_reconnect_timer (QSockClient self)
{
    // local objects
    CapeErr err = cape_err_new ();
    
    cape_log_msg (CAPE_LL_DEBUG, "QSOCK", "client", "start reconnect timer");

    self->aio_timer = cape_aio_add__timer (self->aio, 10000, err);
    if (NULL == self->aio_timer)
    {
        
    }

    // set callback
    cape_aio_item_set (self->aio_timer, self, qsock_client__on_timer, NULL, NULL);
    
    cape_err_del (&err);
}

//-----------------------------------------------------------------------------

int __STDCALL qsock_client__on_recv (void* user_ptr, CapeAioItem item)
{
    QSockClient self = user_ptr;

    // indicates to close the socket by FALSE
    int ret = TRUE;
    
    // indicates to keep on reading
    int run = TRUE;

    // local objects
    CapeErr err = cape_err_new ();
    
    // try to read all data
    while (run)
    {
        switch (cape_sock__recv (cape_aio_item_get (self->aio_item), self->buf_recv, QSOCK_BUFFER_RECV_SIZE, err))
        {
            case CAPE_ERR_NONE:
            {
                if (self->on_recv)
                {
                    self->on_recv (self->user_ptr, cape_stream_data (self->buf_recv), cape_stream_size (self->buf_recv));
                }
                
                break;
            }
            case CAPE_ERR_EOF:
            {
                cape_log_fmt (CAPE_LL_DEBUG, "QSOCK", "read", "connection shutdown detected [%li]", cape_aio_item_get (self->aio_item));
                
                // we don't need to shutdown from our side
                run = FALSE;
                break;
            }
            default:
            {
                cape_log_fmt (CAPE_LL_DEBUG, "QSOCK", "read", "error on connection [%li]", cape_aio_item_get (self->aio_item));

                // we don't need to shutdown from our side
                run = FALSE;
                ret = FALSE;
                break;
            }
        }
    }
    
    cape_err_del (&err);
    return ret;
}

//-----------------------------------------------------------------------------

int __STDCALL qsock_client__on_send (void* user_ptr, CapeAioItem item)
{
    QSockClient self = user_ptr;

    // indicates to close the socket by FALSE
    int ret = TRUE;

    // local objects
    CapeErr err = cape_err_new ();
    
    if (cape_sock__status (cape_aio_item_get (item), err))
    {
        // -> not connected

        // signal to close / destroy the item
        ret = FALSE;
        goto cleanup_and_exit;
    }
        
    cape_log_fmt (CAPE_LL_DEBUG, "QSOCK", "client", "conected socket [%lu]", cape_aio_item_get (item));

    // turn off sending and enable receving
    if (cape_aio_set__mode (self->aio, item, CAPE_AIO_MODE__RECV, err))
    {
        // -> can't set the new mode

        // signal to close / destroy the item
        ret = FALSE;
        goto cleanup_and_exit;
    }

cleanup_and_exit:
    
    cape_err_del (&err);
    return ret;
}

//-----------------------------------------------------------------------------

void __STDCALL qsock_client__on_shutdown (void* user_ptr, CapeAioItem item)
{
    QSockClient self = user_ptr;
    
    void* socket_handle = cape_aio_item_get (item);
    
    cape_log_fmt (CAPE_LL_DEBUG, "QSOCK", "client", "close socket [%lu]", socket_handle);

    cape_sock__close (socket_handle);
    
    // start reconnect timer
    qsock_client__start_reconnect_timer (self);
}

//-----------------------------------------------------------------------------

int qsock_client_run (QSockClient self, CapeErr err)
{
    int res;
    
    // initialize main AIO event handler
    res = cape_aio_init (self->aio, err);
    if (res)
    {
        goto cleanup_and_exit;
    }
    
    res = qsock_client__create_socket (self, err);
    if (res)
    {
        goto cleanup_and_exit;
    }

    res = cape_aio_wait (self->aio, err);
    
cleanup_and_exit:
    
    return res;
}

//-----------------------------------------------------------------------------

void qsock_client_cb (QSockClient self, void* user_ptr, fct_qsock_client__on_recv on_recv)
{
    self->user_ptr = user_ptr;
    self->on_recv = on_recv;
}

//-----------------------------------------------------------------------------
