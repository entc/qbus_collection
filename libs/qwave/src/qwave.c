#include "qwave.h"

// cape includes
#include <sys/cape_socket.h>
#include <sys/cape_log.h>

// project includes
#include "qwave_aioctx.h"
#include "qwave_config.h"

//-----------------------------------------------------------------------------

struct QWave_s
{
    CapeString host;
    number_t port;
    
    QWaveAioctx aioctx;
    QWaveConfig config;
};

//-----------------------------------------------------------------------------

QWave qwave_new (const CapeString host, number_t port, CapeUdc parameters)
{
    QWave self = CAPE_NEW (struct QWave_s);
    
    self->host = cape_str_cp (host);
    self->port = port;
    
    self->aioctx = qwave_aioctx_new ();
    self->config = qwave_config_new ();
    
    return self;
}

//-----------------------------------------------------------------------------

void qwave_del (QWave* p_self)
{
    if (*p_self)
    {
        QWave self = *p_self;
        
        qwave_config_del (&(self->config));
        qwave_aioctx_del (&(self->aioctx));
        
        cape_str_del (&(self->host));
        
        CAPE_DEL (p_self, struct QWave_s);
    }
}

//-----------------------------------------------------------------------------

int __STDCALL qwave_server__on_request (void* user_ptr, void* handle)
{
    qwave_conctx_read (user_ptr, handle);
    
    return QWAVE_EVENT_RESULT__CONTINUE;
}

//-----------------------------------------------------------------------------

void qwave_factory_conctx (QWave self, void* handle_sock, const CapeString remote_address)
{
    QWaveConctx conctx = qwave_conctx_new (self->config, remote_address);
    
    {
        CapeErr err = cape_err_new();
        
        int res = qwave_aioctx_add (self->aioctx, &handle_sock, conctx, qwave_server__on_request, err);
        if (res)
        {
            
        }
        
        cape_err_del (&err);        
    }
}

//-----------------------------------------------------------------------------

#ifdef __WINDOWS_OS

#elif defined __LINUX_OS

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>

//-----------------------------------------------------------------------------

int __STDCALL qwave_server__on_accept (void* user_ptr, void* handle)
{
    QWave self = user_ptr;
    
    struct sockaddr addr;
    socklen_t addrlen = 0;
    
    const char* remoteAddr = NULL;
    
    memset (&addr, 0x00, sizeof(addr));
    
    number_t sock = accept ((number_t)handle, &addr, &addrlen);
    if (sock < 0)
    {
        if( (errno != EWOULDBLOCK) && (errno != EINPROGRESS) && (errno != EAGAIN))
        {
            CapeErr err = cape_err_new ();
            
            cape_err_lastOSError (err);

            cape_log_fmt (CAPE_LL_ERROR, "QWAVE", "accept", "error in accept: %s", cape_err_text (err));
            
            return QWAVE_EVENT_RESULT__ERROR_CLOSED;
        }
        else
        {
            return QWAVE_EVENT_RESULT__TRYAGAIN;
        }
    }
    
    remoteAddr = inet_ntoa(((struct sockaddr_in*)&addr)->sin_addr);
    
    // set the socket to none blocking
    {
        int flags = fcntl(sock, F_GETFL, 0);
        if (flags == -1)
        {
            
        }
        
        flags |= O_NONBLOCK;
        
        if (fcntl(sock, F_SETFL, flags) != 0)
        {
            
        }
    }
    
    cape_log_fmt (CAPE_LL_DEBUG, "QWAVE", "accept", "new connection from '%s' on fd [%lu]", remoteAddr, sock);

    qwave_factory_conctx (self, (void*)sock, remoteAddr);
    
    return QWAVE_EVENT_RESULT__CONTINUE;
}

#endif

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
    
    res = qwave_aioctx_add (self->aioctx, &socket_handle, self, qwave_server__on_accept, err);
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
