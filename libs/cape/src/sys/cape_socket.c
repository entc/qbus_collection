#include "cape_socket.h"
#include "cape_net.h"

// cape includes
#include "sys/cape_log.h"
#include "sys/cape_thread.h"

//-----------------------------------------------------------------------------
// HEADERS AND DEFINITIONS
//-----------------------------------------------------------------------------

#if defined(CAPE_USE_FREERTOS)

// ESP-IDF lwIP BSD socket API
#include <lwip/sockets.h>
#include <lwip/netdb.h>
#include <lwip/inet.h>
#include <sys/types.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#if defined(CAPE_USE_FREERTOS)
#define CAPE_SOCKET__LISTEN_BACKLOG 4
#else
#define CAPE_SOCKET__LISTEN_BACKLOG SOMAXCONN
#endif

#define CAPE_SOCKET_INVALID -1

#elif defined(__LINUX_OS) || defined(__BSD_OS)

// POSIX socket API
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <netdb.h>
#include <errno.h>

#define CAPE_SOCKET__LISTEN_BACKLOG SOMAXCONN
#define CAPE_SOCKET_INVALID -1

#elif defined(__WINDOWS_OS)

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <ws2tcpip.h>
#include <winsock2.h>

#include <windows.h>
#include <stdio.h>

#endif

//-----------------------------------------------------------------------------
// PLATFORM DEPENDEND STATIC FUNCTIONS
//-----------------------------------------------------------------------------

#if defined(CAPE_USE_FREERTOS) || defined(__LINUX_OS) || defined(__BSD_OS)

//-----------------------------------------------------------------------------

static int cape_sock__socket_make_reusable(int sock, CapeErr err)
{
    int opt = 1;

    if (setsockopt((int)sock, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt)) < 0)
    {
        close(sock);

        // save the last system error into the error object
        cape_err_lastOSError(err);
        return CAPE_SOCKET_INVALID;
    }

    return sock;
}

//-----------------------------------------------------------------------------

static int cape_sock__socket (int domain, int type, int protocol, CapeErr err)
{
    int sock;
    
    // try to create a TCP IPV4 socket
    sock = socket (domain, type, protocol);
    if (sock < 0)
    {
        // save the last system error into the error object
        cape_err_lastOSError (err);
        return CAPE_SOCKET_INVALID;
    }

    // avoid to have the socket with FD = 0
    // special case if socket FD is 0
    if (sock != 0)
    {
        return cape_sock__socket_make_reusable (sock, err);
    }
    
    // -> create another socket and close socket with FD 0
    sock = socket (domain, type, protocol);
    if (sock < 0)
    {
        // save the last system error into the error object
        cape_err_lastOSError (err);
        return CAPE_SOCKET_INVALID;
    }

    close (0);
    return cape_sock__socket_make_reusable (sock, err);
}

//-----------------------------------------------------------------------------

#elif defined(__WINDOWS_OS)


#endif

//-----------------------------------------------------------------------------
// PUBLIC FUNCTIONS
//-----------------------------------------------------------------------------

int cape_sock__noneblocking (void* handle, CapeErr err)
{
#if defined(CAPE_USE_FREERTOS) || defined(__LINUX_OS) || defined(__BSD_OS)

    int sock = (int)(number_t)handle;
    
    // save the current flags
    int flags = fcntl ((int)(number_t)sock, F_GETFL, 0);
    if (flags == -1)
    {
        return cape_err_lastOSError (err);
    }

    // add noneblocking
    flags |= O_NONBLOCK;

    // apply the flags
    if (fcntl ((int)(number_t)sock, F_SETFL, flags) != 0)
    {
        return cape_err_lastOSError (err);
    }

    return CAPE_ERR_NONE;

#elif defined(__WINDOWS_OS)

    u_long mode = 1;

    if (ioctlsocket ((SOCKET)handle, FIONBIO, &mode) == SOCKET_ERROR)
    {
        return cape_err_formatErrorOS (err, WSAGetLastError ());
    }

    return CAPE_ERR_NONE;
    
#endif
}

//-----------------------------------------------------------------------------

void* cape_sock__tcp__clt_new (const char* host, long port, CapeErr err)
{
#if defined(CAPE_USE_FREERTOS) || defined(__LINUX_OS) || defined(__BSD_OS)
    
    void* ret = NULL;
    
    // local objects
    struct addrinfo* addr = NULL;
    int sock = CAPE_SOCKET_INVALID;
    
    // resolve host and port into an address object
    addr = cape_net__resolve_os (host, (int)port, FALSE, err);
    if (NULL == addr)
    {
        goto cleanup_and_exit;
    }
    
    // create socket
    sock = cape_sock__socket (addr->ai_family, addr->ai_socktype, addr->ai_protocol, err);
    if (sock < 0)
    {
        goto cleanup_and_exit;
    }
    
    // configure nonblocking mode
    if (cape_sock__noneblocking ((void*)(number_t)sock, err))
    {
        goto cleanup_and_exit;
    }
    
    cape_log_fmt (CAPE_LL_TRACE, "CAPE", "socket", "socket created -> fd [%i]", sock);
    
    // initiate connection
    int rc = connect (sock, addr->ai_addr, addr->ai_addrlen);
    
    if (rc == 0)
    {
        // connection established
    }
    else if (errno == EINPROGRESS || errno == EWOULDBLOCK)
    {
        // connection is pending
        // completion must be checked by the AIO handler
    }
    else
    {
        cape_err_lastOSError (err);
        goto cleanup_and_exit;
    }
    
    ret = (void*)(number_t)sock;
    sock = CAPE_SOCKET_INVALID;
    
cleanup_and_exit:
    
    // free the address information
    cape_net__resolve_del (&addr);
    
    if (sock >= 0)
    {
        close (sock);
    }
    
    return ret;
    
#elif defined(__WINDOWS_OS)
    
    void* ret = NULL;
    
    // local objects
    struct addrinfo* addr = NULL;
    SOCKET sock = INVALID_SOCKET;
    
    // initialize Winsock
    if (!cape_net__init ())
    {
        goto cleanup_and_exit;
    }
    
    // resolve host and port
    addr = cape_net__resolve_os (host, port, FALSE, err);
    if (NULL == addr)
    {
        goto cleanup_and_exit;
    }
    
    // create socket
    sock = cape_sock__socket (addr->ai_family, addr->ai_socktype, addr->ai_protocol, err);
    if (sock == INVALID_SOCKET)
    {
        goto cleanup_and_exit;
    }
    
    // connect synchronously
    if (connect (sock, addr->ai_addr, (int)addr->ai_addrlen) == SOCKET_ERROR)
    {
        cape_err_lastOSError (err);
        goto cleanup_and_exit;
    }
    
    // configure nonblocking mode after connecting
    if (cape_sock__noneblocking ((void*)(number_t)sock, err))
    {
        goto cleanup_and_exit;
    }
    
    cape_log_fmt (CAPE_LL_TRACE, "CAPE", "socket", "socket connected -> [%llu]", (unsigned long long)sock);
    
    ret = (void*)(number_t)sock;
    sock = INVALID_SOCKET;
    
cleanup_and_exit:
    
    cape_net__resolve_del (&addr);
    
    if (sock != INVALID_SOCKET)
    {
        closesocket (sock);
    }
    
    return ret;
    
#endif
}

//-----------------------------------------------------------------------------

void* cape_sock__tcp__srv_new  (const char* host, long port, CapeErr err)
{
#if defined(CAPE_USE_FREERTOS) || defined(__LINUX_OS) || defined(__BSD_OS)

    void* ret = NULL;

    // local objects
    struct addrinfo* addr = NULL;
    int sock = CAPE_SOCKET_INVALID;

    // resolve host and port into an address object
    addr = cape_net__resolve_os (host, (int)port, FALSE, err);
    if (NULL == addr)
    {
        goto cleanup_and_exit;
    }

    // create socket
    sock = cape_sock__socket (addr->ai_family, addr->ai_socktype, addr->ai_protocol, err);
    if (sock < 0)
    {
        goto cleanup_and_exit;
    }

    cape_log_fmt (CAPE_LL_TRACE, "CAPE", "socket", "socket created -> fd [%i]", sock);

    // bind socket
    if (bind (sock, addr->ai_addr, addr->ai_addrlen) < 0)
    {
        cape_err_lastOSError (err);
        goto cleanup_and_exit;
    }

    // start listening
    if (listen (sock, CAPE_SOCKET__LISTEN_BACKLOG) < 0)
    {
        cape_err_lastOSError (err);
        goto cleanup_and_exit;
    }

    // configure nonblocking mode
    if (cape_sock__noneblocking ((void*)(number_t)sock, err))
    {
        goto cleanup_and_exit;
    }

    cape_log_fmt (CAPE_LL_TRACE, "CAPE", "socket", "listen on [%s:%li]", host ? host : "0.0.0.0", port);

    ret = (void*)(number_t)sock;
    sock = CAPE_SOCKET_INVALID;

cleanup_and_exit:

    // free the address information
    cape_net__resolve_del (&addr);

    if (sock >= 0)
    {
        close (sock);
    }

    return ret;

#elif defined(__WINDOWS_OS)

    void* ret = NULL;

    // local objects
    struct addrinfo* addr = NULL;
    SOCKET sock = INVALID_SOCKET;

    // initialize Winsock
    if (!cape_net__init ())
    {
        goto cleanup_and_exit;
    }

    // resolve host and port
    addr = cape_net__resolve_os (host, port, FALSE, err);
    if (NULL == addr)
    {
        goto cleanup_and_exit;
    }

    // create socket
    sock = cape_sock__socket (addr->ai_family, addr->ai_socktype, addr->ai_protocol, err);
    if (sock == INVALID_SOCKET)
    {
        goto cleanup_and_exit;
    }

    // bind socket
    if (bind (sock, addr->ai_addr, (int)addr->ai_addrlen) == SOCKET_ERROR)
    {
        cape_err_lastOSError (err);
        goto cleanup_and_exit;
    }

    // start listening
    if (listen (sock, SOMAXCONN) == SOCKET_ERROR)
    {
        cape_err_lastOSError (err);
        goto cleanup_and_exit;
    }

    cape_log_fmt (CAPE_LL_TRACE, "CAPE", "socket", "listen on [%s:%li]", host ? host : "0.0.0.0", port);

    ret = (void*)(number_t)sock;
    sock = INVALID_SOCKET;

cleanup_and_exit:

    cape_net__resolve_del (&addr);

    if (sock != INVALID_SOCKET)
    {
        closesocket (sock);
    }

    return ret;

#endif
}

//-----------------------------------------------------------------------------

void* cape_sock__udp__clt_new (CapeErr err)
{
#if defined(CAPE_USE_FREERTOS) || defined(__LINUX_OS) || defined(__BSD_OS)

    void* ret = NULL;

    int sock = CAPE_SOCKET_INVALID;

    // create socket as datagram
    sock = cape_sock__socket (AF_INET, SOCK_DGRAM, IPPROTO_UDP, err);
    if (sock < 0)
    {
        goto cleanup_and_exit;
    }

    // configure nonblocking mode
    if (cape_sock__noneblocking ((void*)(number_t)sock, err))
    {
        goto cleanup_and_exit;
    }

    cape_log_fmt (CAPE_LL_DEBUG, "CAPE", "socket clt UDP", "socket created");

    ret = (void*)(number_t)sock;
    sock = CAPE_SOCKET_INVALID;

cleanup_and_exit:

    if (sock >= 0)
    {
        close (sock);
    }

    return ret;

#elif defined(__WINDOWS_OS)

    void* ret = NULL;

    SOCKET sock = INVALID_SOCKET;

    // initialize Winsock
    if (!cape_net__init ())
    {
        goto cleanup_and_exit;
    }

    // create socket as datagram
    sock = cape_sock__socket (AF_INET, SOCK_DGRAM, IPPROTO_UDP, err);
    if (sock == INVALID_SOCKET)
    {
        goto cleanup_and_exit;
    }

    // configure nonblocking mode
    if (cape_sock__noneblocking ((void*)(number_t)sock, err))
    {
        goto cleanup_and_exit;
    }

    cape_log_fmt (CAPE_LL_DEBUG, "CAPE", "socket clt UDP", "socket created");

    ret = (void*)(number_t)sock;
    sock = INVALID_SOCKET;

cleanup_and_exit:

    if (sock != INVALID_SOCKET)
    {
        closesocket (sock);
    }

    return ret;

#endif
}

//-----------------------------------------------------------------------------

void* cape_sock__udp__srv_new (const char* host, long port, CapeErr err)
{
#if defined(CAPE_USE_FREERTOS) || defined(__LINUX_OS) || defined(__BSD_OS)

    void* ret = NULL;

    // local objects
    struct addrinfo* addr = NULL;
    int sock = CAPE_SOCKET_INVALID;

    // resolve host and port into an address object
    addr = cape_net__resolve_os (host, (int)port, FALSE, err);
    if (NULL == addr)
    {
        goto cleanup_and_exit;
    }

    // create socket as datagram
    sock = cape_sock__socket (addr->ai_family, addr->ai_socktype, addr->ai_protocol, err);
    if (sock < 0)
    {
        goto cleanup_and_exit;
    }

    // configure nonblocking mode
    if (cape_sock__noneblocking ((void*)(number_t)sock, err))
    {
        goto cleanup_and_exit;
    }

    // bind the socket to the address
    if (bind (sock, addr->ai_addr, addr->ai_addrlen) < 0)
    {
        cape_err_lastOSError (err);
        goto cleanup_and_exit;
    }

    cape_log_fmt (CAPE_LL_DEBUG, "CAPE", "socket srv UDP", "open socket on %s:%li", host ? host : "*", port);

    ret = (void*)(number_t)sock;
    sock = CAPE_SOCKET_INVALID;

cleanup_and_exit:

    // free the address information
    cape_net__resolve_del (&addr);

    if (sock >= 0)
    {
        close (sock);
    }

    return ret;

#elif defined(__WINDOWS_OS)

    void* ret = NULL;

    // local objects
    struct addrinfo* addr = NULL;
    SOCKET sock = INVALID_SOCKET;

    // initialize Winsock
    if (!cape_net__init ())
    {
        goto cleanup_and_exit;
    }

    // resolve host and port into an address object
    addr = cape_net__resolve_os (host, (int)port, FALSE, err);
    if (NULL == addr)
    {
        goto cleanup_and_exit;
    }

    // create socket as datagram
    sock = cape_sock__socket (addr->ai_family, addr->ai_socktype, addr->ai_protocol, err);
    if (sock == INVALID_SOCKET)
    {
        goto cleanup_and_exit;
    }

    // configure nonblocking mode
    if (cape_sock__noneblocking ((void*)(number_t)sock, err))
    {
        goto cleanup_and_exit;
    }

    // bind the socket to the address
    if (bind (sock, addr->ai_addr, (int)addr->ai_addrlen) == SOCKET_ERROR)
    {
        cape_err_formatErrorOS (err, WSAGetLastError ());
        goto cleanup_and_exit;
    }

    cape_log_fmt (CAPE_LL_DEBUG, "CAPE", "socket srv UDP", "open socket on %s:%li", host ? host : "*", port);

    ret = (void*)(number_t)sock;
    sock = INVALID_SOCKET;

cleanup_and_exit:

    // free the address information
    cape_net__resolve_del (&addr);

    if (sock != INVALID_SOCKET)
    {
        closesocket (sock);
    }

    return ret;

#endif
}

//-----------------------------------------------------------------------------

void* cape_sock__icmp__new (CapeErr err)
{
#if defined(CAPE_USE_FREERTOS) || defined(__LINUX_OS) || defined(__BSD_OS)

    void* ret = NULL;

    int sock = CAPE_SOCKET_INVALID;

    // create ICMP raw socket
    sock = cape_sock__socket (AF_INET, SOCK_RAW, IPPROTO_ICMP, err);
    if (sock < 0)
    {
        goto cleanup_and_exit;
    }

    // configure nonblocking mode
    if (cape_sock__noneblocking ((void*)(number_t)sock, err))
    {
        goto cleanup_and_exit;
    }

    cape_log_fmt (CAPE_LL_TRACE, "CAPE", "socket ICMP", "socket created -> fd [%i]", sock);

    ret = (void*)(number_t)sock;
    sock = CAPE_SOCKET_INVALID;

cleanup_and_exit:

    if (sock >= 0)
    {
        close (sock);
    }

    return ret;

#elif defined(__WINDOWS_OS)

    void* ret = NULL;

    SOCKET sock = INVALID_SOCKET;

    // initialize Winsock
    if (!cape_net__init ())
    {
        goto cleanup_and_exit;
    }

    // create ICMP raw socket
    sock = cape_sock__socket (AF_INET, SOCK_RAW, IPPROTO_ICMP, err);
    if (sock == INVALID_SOCKET)
    {
        goto cleanup_and_exit;
    }

    // configure nonblocking mode
    if (cape_sock__noneblocking ((void*)(number_t)sock, err))
    {
        goto cleanup_and_exit;
    }

    cape_log_fmt (CAPE_LL_TRACE, "CAPE", "socket ICMP", "socket created");

    ret = (void*)(number_t)sock;
    sock = INVALID_SOCKET;

cleanup_and_exit:

    if (sock != INVALID_SOCKET)
    {
        closesocket (sock);
    }

    return ret;

#endif
}

//-----------------------------------------------------------------------------

int cape_sock__udp__send_to_nr (void* handle, CapeStream buf, struct addrinfo* addr, CapeErr err)
{
#if defined(CAPE_USE_FREERTOS) || defined(__LINUX_OS) || defined(__BSD_OS)

    // file descriptor
    int fd = (int)(number_t)handle;

    ssize_t bytes_send = sendto (fd, cape_stream_data (buf), cape_stream_size (buf), 0, addr->ai_addr, addr->ai_addrlen);

    if (bytes_send == -1)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
        {
            return CAPE_ERR_CONTINUE;
        }

        return cape_err_lastOSError (err);
    }

    return CAPE_ERR_NONE;

#elif defined(__WINDOWS_OS)

    SOCKET sock = (SOCKET)(number_t)handle;

    int bytes_send = sendto (sock, (const char*)cape_stream_data (buf), (int)cape_stream_size (buf), 0, addr->ai_addr, (int)addr->ai_addrlen);

    if (bytes_send == SOCKET_ERROR)
    {
        int error_code = WSAGetLastError ();

        if (error_code == WSAEWOULDBLOCK || error_code == WSAEINTR)
        {
            return CAPE_ERR_CONTINUE;
        }

        return cape_err_formatErrorOS (err, error_code);
    }

    return CAPE_ERR_NONE;

#endif
}

//-----------------------------------------------------------------------------

int cape_sock__udp__send_to (void* handle, CapeStream buf, const char* host, long port, CapeErr err)
{
    int res = CAPE_ERR_NONE;
    number_t bufpos = 0;

    if (host && port)
    {
        struct addrinfo* addr = cape_net__new_simple (0, AF_INET, SOCK_DGRAM, IPPROTO_UDP, host, (int)port, NULL);

        if (NULL == addr)
        {
            return cape_err_set (err, CAPE_ERR_RUNTIME, "can't resolve address");
        }
        
        res = cape_sock__udp__send_to_nr (handle, buf, addr, err);
        
        cape_net__resolve_del (&addr);
    }
        
    return res;
}

//-----------------------------------------------------------------------------

#if defined __LINUX_OS || defined __BSD_OS




//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------



//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------

void* cape_sock__accept (void* handle, CapeString* p_remote_addr, CapeErr err)
{
  struct sockaddr addr;
  socklen_t addrlen = 0;
  
  const char* remote_addr = NULL;
  
  memset (&addr, 0x00, sizeof(addr));
  
  int sock = accept ((int)(number_t)handle, &addr, &addrlen);
  
  if (sock < 0)
  {
    if( (errno != EWOULDBLOCK) && (errno != EINPROGRESS) && (errno != EAGAIN))
    {
      cape_err_lastOSError (err);
      
      cape_log_fmt (CAPE_LL_ERROR, "CAPE", "accept", "error in accept: %s", cape_err_text (err));
    }

    return NULL;
  }
  
  remote_addr = inet_ntoa(((struct sockaddr_in*)&addr)->sin_addr);
    
  if (p_remote_addr)
  {
    cape_str_replace_cp (p_remote_addr, remote_addr);
  }
  
  return (void*)(number_t)sock;
}

//-----------------------------------------------------------------------------

int cape_sock__recv (void* handle, CapeStream bufdat, number_t buflen, CapeErr err)
{
  int res;
  ssize_t bytes_read;
  
  // reserve bytes
  cape_stream_cap (bufdat, buflen);
    
  // try to read bytes from FD
  bytes_read = recv ((int)(number_t)handle, cape_stream_pos (bufdat), buflen, 0);
    
  if (-1 == bytes_read)
  {
    if ((errno != EAGAIN) && (errno != EWOULDBLOCK))
    {
      res = cape_err_lastOSError (err);        
    }
    else
    {
      res = CAPE_ERR_CONTINUE;      
    }
  }
  else if (0 == bytes_read)
  {
    // When a stream socket peer has performed an orderly shutdown, the
    // return value will be 0 (the traditional "end-of-file" return).
    //
    // The value 0 may also be returned if the requested number of bytes
    // to receive from a stream socket was 0.
    res = CAPE_ERR_EOF;
  }
  else
  {
    // set new position
    cape_stream_set (bufdat, bytes_read);

    res = CAPE_ERR_NONE;
  }
      
  return res;
}

//-----------------------------------------------------------------------------

int cape_sock__send (void* handle, CapeStream buffer, CapeErr err)
{
  ssize_t bytes_sent;
  number_t total_sent = 0;
  number_t buflen = cape_stream_size (buffer);
  
  while (total_sent < buflen)
  {
    bytes_sent = send ((int)(number_t)handle, cape_stream_data (buffer) + total_sent, buflen - total_sent, MSG_NOSIGNAL);
    
    if (-1 == bytes_sent)
    {
      if ((errno == EAGAIN) || (errno == EWOULDBLOCK) || (errno == EINTR))
      {
        continue;
      }
      else if (errno == EPIPE)
      {
        return CAPE_ERR_EOF;
      }
      else
      {
        // exit
        return cape_err_lastOSError (err);
      }
    }
    else if (0 == bytes_sent)
    {
      return CAPE_ERR_EOF;
    }
    else
    {
      // calculate the new position
      total_sent = total_sent + bytes_sent;
    }
  }
  
  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

int cape_sock__touch (void* handle, CapeErr err)
{
  send ((int)(number_t)handle, NULL, 0, MSG_NOSIGNAL);
  
  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

void cape_sock__close (void* handle)
{  
  if (-1 == close ((int)(number_t)handle))
  {
    CapeErr err = cape_err_new ();
    
    cape_err_lastOSError (err);

    cape_log_fmt (CAPE_LL_ERROR, "CAPE", "socket", "error on closing socket [%lu]: %s", (number_t)handle, cape_err_text (err));
    
    cape_err_del (&err);
  }
  else
  {
    cape_log_fmt (CAPE_LL_TRACE, "CAPE", "socket", "socket closed <- fd [%lu]", (number_t)handle);
  }
}

//-----------------------------------------------------------------------------

int cape_sock__status (void* handle, CapeErr err)
{
    int error_code = 0;
    socklen_t len = sizeof(error_code);

    if (getsockopt ((int)(number_t)handle, SOL_SOCKET, SO_ERROR, &error_code, &len) == -1)
    {
        return cape_err_lastOSError (err);
    }
    else if (error_code == 0)
    {
        return CAPE_ERR_NONE;
    }
    else
    {
        return cape_err_formatErrorOS (err, error_code);
    }
}

//-----------------------------------------------------------------------------

void cape_sock__shutdown (void* handle)
{
  if (-1 == shutdown ((int)(number_t)handle, SHUT_RDWR))
  {
    CapeErr err = cape_err_new ();
    
    cape_err_lastOSError (err);
    
    cape_log_fmt (CAPE_LL_ERROR, "CAPE", "socket", "error on shutdown socket [%lu]: %s", (number_t)handle, cape_err_text (err));
    
    cape_err_del (&err);
  }
  else
  {
    cape_log_fmt (CAPE_LL_TRACE, "CAPE", "socket", "socket shutdown [rdwr] <- fd [%lu]", (number_t)handle);
  }
}

//-----------------------------------------------------------------------------

void cape_sock__shutdown__rd (void* handle)
{
  if (-1 == shutdown ((int)(number_t)handle, SHUT_RD))
  {
    CapeErr err = cape_err_new ();
    
    cape_err_lastOSError (err);
    
    cape_log_fmt (CAPE_LL_ERROR, "CAPE", "socket", "error on shutdown socket [%lu]: %s", (number_t)handle, cape_err_text (err));
    
    cape_err_del (&err);
  }
  else
  {
    cape_log_fmt (CAPE_LL_TRACE, "CAPE", "socket", "socket shutdown [rd] <- fd [%lu]", (number_t)handle);
  }
}

//-----------------------------------------------------------------------------

void cape_sock__shutdown__wr (void* handle)
{
  if (-1 == shutdown ((int)(number_t)handle, SHUT_WR))
  {
    CapeErr err = cape_err_new ();
    
    cape_err_lastOSError (err);
    
    cape_log_fmt (CAPE_LL_ERROR, "CAPE", "socket", "error on shutdown socket [%lu]: %s", (number_t)handle, cape_err_text (err));
    
    cape_err_del (&err);
  }
  else
  {
    cape_log_fmt (CAPE_LL_TRACE, "CAPE", "socket", "socket shutdown [wr] <- fd [%lu]", (number_t)handle);
  }
}

//-----------------------------------------------------------------------------

#elif defined _WIN64 || defined _WIN32

//-----------------------------------------------------------------------------

SOCKET cape_sock__socket_make_exclusive (SOCKET sock, CapeErr err)
{
    BOOL opt = TRUE;

    if (setsockopt((int)sock, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, (char*)&opt, sizeof(opt)) < 0)
    {
        closesocket (sock);

        // save the last system error into the error object
        cape_err_lastOSError(err);
        return INVALID_SOCKET;
    }

    return sock;
}

//-----------------------------------------------------------------------------

SOCKET cape_sock__socket (int domain, int type, int protocol, CapeErr err)
{
    SOCKET sock;

    // try to create a TCP IPV4 socket
    sock = WSASocketA (domain, type, protocol, NULL, 0, WSA_FLAG_OVERLAPPED);
    if (sock < 0)
    {
        // save the last system error into the error object
        cape_err_lastOSError(err);
        return INVALID_SOCKET;
    }

    return cape_sock__socket_make_exclusive (sock, err);
}

//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------

void cape_sock__close (void* sock)
{
    closesocket ((SOCKET)sock);
}

//-----------------------------------------------------------------------------

int cape_sock__status (void* handle, CapeErr err)
{
    int error_code = 0;
    socklen_t len = sizeof(error_code);

    if (getsockopt((SOCKET)handle, SOL_SOCKET, SO_ERROR, (char *)&error_code, &len) == SOCKET_ERROR)
    {
        return cape_err_lastOSError(err);
    }

    if (error_code == 0)
    {
        return CAPE_ERR_NONE;
    }

    return cape_err_formatErrorOS (err, error_code);
}

//-----------------------------------------------------------------------------

#endif
