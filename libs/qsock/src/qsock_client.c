#include "qsock_client.h"

// cape includes
#include <sys/cape_socket.h>
#include <sys/cape_log.h>
#include <sys/cape_thread.h>
#include <sys/cape_queue.h>
#include <sys/cape_aio.h>

//-----------------------------------------------------------------------------

struct QSockClient_s
{
    CapeString host;
    int port;

    CapeAio aio;
    CapeAioItem aio_item;
    
    CapeStream buf_recv;
};

//-----------------------------------------------------------------------------

QSockClient qsock_client_new (const CapeString host, int port)
{
    QSockClient self = CAPE_NEW (struct QSockClient_s);

    self->host = cape_str_cp (host);
    self->port = port;

    self->aio = cape_aio_new ();
    self->aio_item = NULL;
    
    self->buf_recv = cape_stream_new ();
    
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

void __STDCALL qsock_client__on_recv (void* user_ptr, void* handle)
{
    QSockClient self = user_ptr;
    
    //int res = cape_sock__recv (handle, cape_stream_data (self->buf_recv), 1024, err);
}

//-----------------------------------------------------------------------------

void __STDCALL qsock_client__on_shutdown (void* user_ptr, void* handle)
{
    QSockClient self = user_ptr;
    
    cape_sock__close (handle);
}

//-----------------------------------------------------------------------------

int qsock_client_run (QSockClient self, CapeErr err)
{
    int res;
    
    void* handle;
    
    // initialize main AIO event handler
    res = cape_aio_init (self->aio, err);
    if (res)
    {
        goto cleanup_and_exit;
    }
    
    handle = cape_sock__tcp__clt_new (self->host, self->port, err);
    if (NULL == handle)
    {
        res = cape_err_code (err);
        goto cleanup_and_exit;
    }
    
    // attach the socket handle to the AIO controller
    self->aio_item = cape_aio_add (self->aio, handle, err);
    
    if (NULL == self->aio_item)
    {
        res = cape_err_code (err);
        goto cleanup_and_exit;
    }
    else
    {
        handle = NULL;
    }

    // set the callbacks
    cape_aio_item_set (self->aio_item, self, qsock_client__on_recv, qsock_client__on_shutdown);

    res = cape_aio_wait (self->aio, err);
    
cleanup_and_exit:
    
    if (handle)
    {
        cape_sock__close (handle);
    }

    return res;
}

//-----------------------------------------------------------------------------
