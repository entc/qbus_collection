#include "qsock_client.h"

// cape includes
#include <sys/cape_socket.h>
#include <sys/cape_log.h>
#include <sys/cape_thread.h>
#include <sys/cape_queue.h>
#include <stc/cape_list.h>
#include <sys/cape_mutex.h>

#define QSOCK_BUFFER_RECV_SIZE    1024

//-----------------------------------------------------------------------------

int __STDCALL qsock_client__on_recv (void* user_ptr, CapeAioItem item);
int __STDCALL qsock_client__on_send (void* user_ptr, CapeAioItem item);
void __STDCALL qsock_client__on_shutdown (void* user_ptr, CapeAioItem item);

void qsock_client__start_reconnect_timer (QSockClient);

//-----------------------------------------------------------------------------

struct QSockClient_s
{
    CapeString host;
    int port;

    CapeAio aio;
    int aio_owned;

    CapeAioItem aio_item;
    
    CapeStream buf_recv;
    
    void* user_ptr;
    fct_qsock_client__on_conn on_conn;
    fct_qsock_client__on_recv on_recv;

    // for sending - cache
    CapeList cached_buffers;

    CapeMutex mutex;
};

//-----------------------------------------------------------------------------

void __STDCALL qsock_client__cached_buffers__on_del (void* ptr)
{
    CapeStream h = ptr; cape_stream_del (&h);
}

//-----------------------------------------------------------------------------

QSockClient qsock_client_new (const CapeString host, int port, CapeAio aio)
{
    QSockClient self = CAPE_NEW (struct QSockClient_s);

    self->host = cape_str_cp (host);
    self->port = port;

    if (aio)
    {
        self->aio = aio;
        self->aio_owned = FALSE;
    }
    else
    {
        self->aio = cape_aio_new ();
        self->aio_owned = TRUE;
    }

    self->aio_item = NULL;
    
    self->buf_recv = cape_stream_new ();
    cape_stream_cap (self->buf_recv, QSOCK_BUFFER_RECV_SIZE);
    
    self->user_ptr = NULL;
    self->on_conn = NULL;
    self->on_recv = NULL;

    self->cached_buffers = cape_list_new (qsock_client__cached_buffers__on_del);
    self->mutex = cape_mutex_new ();

    return self;
}

//-----------------------------------------------------------------------------

void qsock_client_del (QSockClient* p_self)
{
    if (*p_self)
    {
        QSockClient self = *p_self;

        if (self->aio_owned)
        {
            cape_aio_del (&(self->aio));
        }

        cape_str_del (&(self->host));

        cape_stream_del (&(self->buf_recv));

        cape_list_del (&(self->cached_buffers));
        cape_mutex_del (&(self->mutex));

        CAPE_DEL (p_self, struct QSockClient_s);
    }
}

//-----------------------------------------------------------------------------

int qsock_client__create_socket (QSockClient self, CapeErr err)
{
    int res;
    
    void* handle;
    CapeAioItem item;
    
    handle = cape_sock__tcp__clt_new (self->host, self->port, err);
    if (NULL == handle)
    {
        // this can happen if the pear is not available
        qsock_client__start_reconnect_timer (self);

        // return no error -> reconnect activated
        res = CAPE_ERR_NONE;
        goto cleanup_and_exit;
    }
    
    cape_log_msg (CAPE_LL_DEBUG, "QSOCK", "client", "socket created -> add to event handler");

    // attach the socket handle to the AIO controller
    // start with send mode first
    item = cape_aio_add (self->aio, handle, CAPE_AIO_MODE__SEND, err);
    
    if (NULL == item)
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
    cape_aio_item_set (item, self, qsock_client__on_recv, qsock_client__on_send, qsock_client__on_shutdown);
    
cleanup_and_exit:
    
    if (handle)
    {
        cape_sock__close (handle);
    }

    return res;
}

//-----------------------------------------------------------------------------

int __STDCALL qsock_client__on_timer_event (void* user_ptr, CapeAioItem item)
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

void __STDCALL qsock_client__on_timer_done (void* user_ptr, CapeAioItem item)
{
    cape_log_msg (CAPE_LL_TRACE, "QSOCK", "client", "timer has been stopped");
}

//-----------------------------------------------------------------------------

void qsock_client__start_reconnect_timer (QSockClient self)
{
    // local objects
    CapeErr err = cape_err_new ();

    CapeAioItem aio_timer;

    cape_log_msg (CAPE_LL_DEBUG, "QSOCK", "client", "start reconnect timer");

    // the instance of the timer is managed within cape_aio
    aio_timer = cape_aio_add__timer (self->aio, 10000, err);
    if (NULL == aio_timer)
    {

    }
    else
    {
        // set callback
        cape_aio_item_set (aio_timer, self, qsock_client__on_timer_event, NULL, qsock_client__on_timer_done);
    }

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
        switch (cape_sock__recv (cape_aio_item_get (item), self->buf_recv, QSOCK_BUFFER_RECV_SIZE, err))
        {
            case CAPE_ERR_NONE:
            {
                if (self->on_recv)
                {
                    self->on_recv (self->user_ptr, cape_stream_data (self->buf_recv), cape_stream_size (self->buf_recv));
                }
                
                break;
            }
            case CAPE_ERR_CONTINUE:
            {
                run = FALSE;
                break;
            }
            case CAPE_ERR_EOF:
            {
                cape_log_fmt (CAPE_LL_WARN, "QSOCK", "read", "connection shutdown detected [%li]", cape_aio_item_get (item));
                
                // we don't need to shutdown from our side
                run = FALSE;
                ret = FALSE;
                break;
            }
            default:
            {
                cape_log_fmt (CAPE_LL_ERROR, "QSOCK", "read", "error on connection [%li]", cape_aio_item_get (item));

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

int qsock_client__finish_connect (QSockClient self, CapeAioItem item, CapeErr err)
{
    if (cape_sock__status (cape_aio_item_get (item), err))
    {
        // -> not connected

        // signal to close / destroy the item
        return FALSE;
    }

    cape_log_fmt (CAPE_LL_DEBUG, "QSOCK", "client", "conected socket [%lu]", cape_aio_item_get (item));

    // turn off sending and enable receving
    if (cape_aio_set__mode (self->aio, item, CAPE_AIO_MODE__RECV, err))
    {
        // -> can't set the new mode

        // signal to close / destroy the item
        return FALSE;
    }

    cape_mutex_lock (self->mutex);

    // set the connected state
    self->aio_item = item;

    cape_mutex_unlock (self->mutex);

    // TODO: race condition, here another send from another thread might flush the buffer

    if (self->on_conn)
    {
        // call the user defined on connect callback
        self->on_conn (self->user_ptr);
    }

    // run again to send initial buffers (if already added)
    return qsock_client__on_send (self, item);
}

//-----------------------------------------------------------------------------

CapeStream qsock_client__next_buffer (QSockClient self)
{
    CapeStream ret;

    cape_mutex_lock (self->mutex);

    ret = cape_list_pop_front (self->cached_buffers);

    cape_mutex_unlock (self->mutex);

    return ret;
}

//-----------------------------------------------------------------------------

int qsock_client__flush_send_queue (QSockClient self, CapeAioItem item, CapeErr err)
{
    CapeStream buffer;

    // get the socket handle from the current AIO item
    void* socket_handle = cape_aio_item_get (item);

    while ((buffer = qsock_client__next_buffer (self)) != NULL)
    {
        if (cape_sock__send (socket_handle, buffer, err))
        {
            cape_stream_del(&buffer);

            // signal to close the socket
            return FALSE;
        }

        cape_log_fmt (CAPE_LL_TRACE, "QSOCK", "on send", "buffer sent with %lu bytes", cape_stream_size (buffer));

        cape_stream_del (&buffer);
    }

    // turn off sending and enable receving
    return CAPE_ERR_NONE == cape_aio_set__mode (self->aio, item, CAPE_AIO_MODE__RECV, err);
}

//-----------------------------------------------------------------------------

int __STDCALL qsock_client__on_send (void* user_ptr, CapeAioItem item)
{
    QSockClient self = user_ptr;

    // indicates to close the socket by FALSE
    int ret = TRUE;

    // local objects
    CapeErr err = cape_err_new ();

    // local version of the item
    CapeAioItem aio_item;

    cape_mutex_lock (self->mutex);

    aio_item = self->aio_item;

    cape_mutex_unlock (self->mutex);

    if (aio_item)
    {
        ret = qsock_client__flush_send_queue (self, aio_item, err);
    }
    else
    {
        ret = qsock_client__finish_connect (self, item, err);
    }

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
    
    cape_mutex_lock (self->mutex);

    // disable connected status
    self->aio_item = NULL;

    cape_list_clr (self->cached_buffers);

    cape_mutex_unlock (self->mutex);

    // start reconnect timer
    qsock_client__start_reconnect_timer (self);
}

//-----------------------------------------------------------------------------

int qsock_client_init (QSockClient self, CapeErr err)
{
    if (TRUE == self->aio_owned)
    {
        return cape_err_set (err, CAPE_ERR_WRONG_STATE, "init can only be used if the AIO IS NOT owned");
    }

    return qsock_client__create_socket (self, err);
}

//-----------------------------------------------------------------------------

int qsock_client_run (QSockClient self, CapeErr err)
{
    int res;
    
    if (FALSE == self->aio_owned)
    {
        return cape_err_set (err, CAPE_ERR_WRONG_STATE, "run can only be used if the AIO IS owned");
    }

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

void qsock_client_cb (QSockClient self, void* user_ptr, fct_qsock_client__on_conn on_conn, fct_qsock_client__on_recv on_recv)
{
    self->user_ptr = user_ptr;
    self->on_conn = on_conn;
    self->on_recv = on_recv;
}

//-----------------------------------------------------------------------------

void qsock_client_send (QSockClient self, CapeStream* p_buffer, int clear_buffer)
{
    // local objects
    CapeErr err = cape_err_new ();

    cape_mutex_lock (self->mutex);

    if (clear_buffer)
    {
        // to ensure that our new message is the next message sent
        cape_list_clr (self->cached_buffers);
    }

    // move the buffer into the list
    cape_list_push_back (self->cached_buffers, (void*)cape_stream_mv (p_buffer));

    if (self->aio_item)
    {
        // change event handling
        // turn off sending and enable receving
        if (cape_aio_set__mode (self->aio, self->aio_item, CAPE_AIO_MODE__RECV | CAPE_AIO_MODE__SEND, err))
        {
            // -> can't set the new mode

        }
    }

    cape_mutex_unlock (self->mutex);

    cape_err_del (&err);
}

//-----------------------------------------------------------------------------
