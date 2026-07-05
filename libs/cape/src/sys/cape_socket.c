#include "cape_socket.h"
#include "cape_net.h"

// cape includes
#include "sys/cape_log.h"
#include "sys/cape_thread.h"

//-----------------------------------------------------------------------------

#if defined __LINUX_OS || defined __BSD_OS

// c includes
#include <memory.h>
#include <sys/socket.h>	// basic socket definitions
#include <sys/types.h>
#include <arpa/inet.h>	// inet(3) functions
#include <fcntl.h>
#include <unistd.h>
#include <netdb.h>
#include <errno.h>

#elif defined _WIN64 || defined _WIN32

#include <ws2tcpip.h>
#include <winsock2.h>

#include <windows.h>
#include <stdio.h>

#endif

//-----------------------------------------------------------------------------

#if defined __LINUX_OS || defined __BSD_OS

#define CAPE_SOCKET_INVALID -1

//-----------------------------------------------------------------------------

void* cape_sock__tcp__clt_new (const char* host, long port, CapeErr err)
{
  void* ret = NULL;

  // local objects
  struct sockaddr_in* addr = cape_net__resolve_os (host, (u_short)port, FALSE, err);
  int sock = CAPE_SOCKET_INVALID;

  if (NULL == addr)
  {
    return NULL;
  }

  // create socket
  sock = socket (AF_INET, SOCK_STREAM, 0);
  if (sock < 0)
  {
    // save the last system error into the error object
    cape_err_lastOSError (err);

    goto cleanup_and_exit;
  }

  // make the socket none-blocking
  {
    int flags = fcntl(sock, F_GETFL, 0);

    if (flags == -1)
    {
      // save the last system error into the error object
      cape_err_lastOSError (err);

      goto cleanup_and_exit;
    }

    flags |= O_NONBLOCK;

    if (fcntl(sock, F_SETFL, flags) != 0)
    {
      // save the last system error into the error object
      cape_err_lastOSError (err);

      goto cleanup_and_exit;
    }
  }

  // connect, don't check result because it is none-blocking
  connect (sock, (const struct sockaddr*)addr, sizeof(struct sockaddr_in));

  cape_log_msg (CAPE_LL_TRACE, "CAPE", "clt new", "connected");
  
  ret = (void*)(number_t)sock;
  sock = CAPE_SOCKET_INVALID;

cleanup_and_exit:

  CAPE_FREE (addr);

  if (sock >= 0)
  {
    close(sock);
  }

  return ret;
}

//-----------------------------------------------------------------------------

void* cape_sock__tcp__srv_new  (const char* host, long port, CapeErr err)
{
  void* ret = NULL;

  // local objects
  struct sockaddr_in* addr = cape_net__resolve_os (host, port, FALSE, err);
  number_t sock1 = CAPE_SOCKET_INVALID;
  number_t sock2 = CAPE_SOCKET_INVALID;
  int opt = 1;

  if (NULL == addr)
  {
    return NULL;
  }

  // try to create a TCP IPV4 socket
  sock1 = socket (AF_INET, SOCK_STREAM, 0);
  if (sock1 < 0)
  {
    // save the last system error into the error object
    cape_err_lastOSError (err);

    goto cleanup_and_exit;
  }
    
  // avoid to have the socket with FD = 0
  if (sock1 == 0)
  {
    sock2 = 0;
    
    sock1 = socket (AF_INET, SOCK_STREAM, 0);
    if (sock1 < 0)
    {
      // save the last system error into the error object
      cape_err_lastOSError (err);

      goto cleanup_and_exit;
    }
  }
  
  cape_log_fmt (CAPE_LL_TRACE, "CAPE", "socket", "socket created -> fd [%i]", sock1);

  if (setsockopt ((int)sock1, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt)) < 0)
  {
    // save the last system error into the error object
    cape_err_lastOSError (err);

    goto cleanup_and_exit;
  }

  if (bind ((int)sock1, (const struct sockaddr*)addr, sizeof(struct sockaddr_in)) < 0)
  {
    // save the last system error into the error object
    cape_err_lastOSError (err);

    goto cleanup_and_exit;
  }

  // cannot fail
  listen((int)sock1, SOMAXCONN);

  cape_log_fmt (CAPE_LL_TRACE, "CAPE", "cape_socket", "listen on [%s:%li]", host, port);

  ret = (void*)sock1;
  sock1 = CAPE_SOCKET_INVALID;

cleanup_and_exit:

  CAPE_FREE (addr);

  if (sock1 >= 0)
  {
    close((int)sock1);
  }

  if (sock2 >= 0)
  {
    close((int)sock2);
  }

  return ret;
}

//-----------------------------------------------------------------------------

void* cape_sock__udp__clt_new (CapeErr err)
{
  void* ret = NULL;

  long sock = CAPE_SOCKET_INVALID;

  // create socket as datagram
#if defined __LINUX_OS
  sock = socket (AF_INET, SOCK_DGRAM | SOCK_NONBLOCK, IPPROTO_UDP);
#else
  sock = socket (AF_INET, SOCK_DGRAM, IPPROTO_UDP);
#endif

  if (sock < 0)
  {
    // save the last system error into the error object
    cape_err_lastOSError (err);

    goto cleanup_and_exit;
  }

  // make the socket none-blocking
  {
    int flags = fcntl(sock, F_GETFL, 0);

    if (flags == -1)
    {
      // save the last system error into the error object
      cape_err_lastOSError (err);

      goto cleanup_and_exit;
    }

    flags |= O_NONBLOCK;

    if (fcntl(sock, F_SETFL, flags) != 0)
    {
      // save the last system error into the error object
      cape_err_lastOSError (err);

      goto cleanup_and_exit;
    }
  }

  cape_log_fmt (CAPE_LL_DEBUG, "CAPE", "socket clt UDP", "socket created");

  ret = (void*)sock;
  sock = CAPE_SOCKET_INVALID;

cleanup_and_exit:

  if (sock >= 0)
  {
    close(sock);
  }

  return ret;
}

//-----------------------------------------------------------------------------

void* cape_sock__udp__srv_new (const char* host, long port, CapeErr err)
{
  void* ret = NULL;

  int sock = CAPE_SOCKET_INVALID;
  struct sockaddr_in* addr = cape_net__resolve_os (host, port, FALSE, err);

  // create socket
#if defined __LINUX_OS
  sock = socket (AF_INET, SOCK_DGRAM | SOCK_NONBLOCK, IPPROTO_UDP);
#else
  sock = socket (AF_INET, SOCK_DGRAM, IPPROTO_UDP);
#endif

  if (sock < 0)
  {
    // save the last system error into the error object
    cape_err_lastOSError (err);

    goto cleanup_and_exit;
  }

  {
    int opt = 1;

    // set the socket option to reuse the address
    if (setsockopt (sock, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0)
    {
      // save the last system error into the error object
      cape_err_lastOSError (err);

      goto cleanup_and_exit;
    }

    // try to bind the socket to an address
    if (bind (sock, (const struct sockaddr*)addr, sizeof(struct sockaddr_in)) < 0)
    {
      // save the last system error into the error object
      cape_err_lastOSError (err);

      goto cleanup_and_exit;
    }
  }

  // make the socket none-blocking
  {
    // get current flags
    int flags = fcntl (sock, F_GETFL, 0);

    if (flags == -1)
    {
      // save the last system error into the error object
      cape_err_lastOSError (err);

      goto cleanup_and_exit;
    }

    // add noneblocking flag
    flags |= O_NONBLOCK;

    // set flags
    if (fcntl ((int)sock, F_SETFL, flags) != 0)
    {
      // save the last system error into the error object
      cape_err_lastOSError (err);

      goto cleanup_and_exit;
    }
  }

  cape_log_fmt (CAPE_LL_DEBUG, "CAPE", "socket srv UDP", "open socket on %s:%i", host, port);

  ret = (void*)(number_t)sock;
  sock = CAPE_SOCKET_INVALID;

cleanup_and_exit:

  CAPE_FREE (addr);

  if (sock >= 0)
  {
    close((int)sock);
  }

  return ret;
}

//-----------------------------------------------------------------------------

int cape_sock__udp__send_to (void* handle, CapeStream buf, const char* host, long port, CapeErr err)
{
  int res = CAPE_ERR_NONE;
  number_t bufpos = 0;

  if (host && port)
  {
    struct sockaddr_in send_addr;
    
    memset (&send_addr, 0, sizeof(struct sockaddr_in));
    
    send_addr.sin_family = AF_INET;      // set the network type
    send_addr.sin_port = htons (port);    // set the port
    send_addr.sin_addr.s_addr = inet_addr(host);
    
    res = cape_sock__udp__send_to_nr (handle, buf, &send_addr, err);
  }
      
  return res;
}

//-----------------------------------------------------------------------------

int cape_sock__udp__send_to_nr (void* handle, CapeStream buf, CapeSockaddr addr, CapeErr err)
{
  int res = CAPE_ERR_NONE;
  number_t bufpos = 0;

  // file descriptor
  int fd = (int)(number_t)handle;
  
  ssize_t bytes_send = sendto (fd, cape_stream_data (buf) + bufpos, cape_stream_size (buf) - bufpos, MSG_DONTWAIT, (const struct sockaddr*)addr, sizeof(struct sockaddr_in));
  if (bytes_send == -1)
  {
    res = cape_err_lastOSError (err);
  }

  return res;
}

//-----------------------------------------------------------------------------

void* cape_sock__icmp__new (CapeErr err)
{
  long sock = CAPE_SOCKET_INVALID;

  // create socket
#if defined __LINUX_OS
  sock = socket (AF_INET, SOCK_RAW | SOCK_NONBLOCK, IPPROTO_ICMP);
#else
  sock = socket (AF_INET, SOCK_RAW, IPPROTO_ICMP);
#endif

  if (sock < 0)
  {
    goto exit_and_cleanup;
  }

  // return the socket
  return (void*)sock;

exit_and_cleanup:

  // save the last system error into the error object
  cape_err_lastOSError (err);

  if (sock >= 0)
  {
    close(sock);
  }

  cape_log_fmt (CAPE_LL_ERROR, "CAPE", "icmp new", "can't create ICMP socket: %s", cape_err_text(err));

  return NULL;
}

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

int cape_sock__noneblocking (void* sock, CapeErr err)
{
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
}

//-----------------------------------------------------------------------------

#elif defined _WIN64 || defined _WIN32

//-----------------------------------------------------------------------------

void* cape_sock__tcp__clt_new (const char* host, long port, CapeErr err)
{
  // TODO: needs to be done

  return NULL;
}

//-----------------------------------------------------------------------------

void* cape_sock__tcp__srv_new (const char* host, long port, CapeErr err)
{
    // local variables
    struct addrinfo* addr = NULL;
    SOCKET sock = INVALID_SOCKET;

    // in windows the WSA system must be initialized first
    if (!cape_net__init())
    {
        goto exit_and_cleanup;
    }

    // resolve host and port
    addr = (struct addrinfo*)cape_net__resolve_os (host, port, FALSE, err);

    // create the socket using the resolved address
    sock = WSASocket (addr->ai_family, addr->ai_socktype, addr->ai_protocol, NULL, 0, WSA_FLAG_OVERLAPPED);
    if (sock == INVALID_SOCKET)
    {
        goto exit_and_cleanup;
    }

    {
        BOOL opt = TRUE;
        setsockopt (sock, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, (const char*)&opt, sizeof(opt));
    }

    if (bind (sock, addr->ai_addr, (int)addr->ai_addrlen) == SOCKET_ERROR)
    {
        goto exit_and_cleanup;
    }

    // in windows this can fail
    if (listen(sock, SOMAXCONN) == SOCKET_ERROR)
    {
        goto exit_and_cleanup;
    }

    cape_log_fmt(CAPE_LL_TRACE, "CAPE", "cape_socket", "listen on [%s:%li]", host ? host : "0.0.0.0", port);

    cape_net__resolve_del(&addr);

    return (void*)sock;

exit_and_cleanup:

    cape_net__resolve_del(&addr);

    // save the last system error into the error object
    cape_err_formatErrorOS(err, WSAGetLastError());

    if (sock != INVALID_SOCKET)
    {
        closesocket (sock);
    }

    return NULL;
}

//-----------------------------------------------------------------------------

void* cape_sock__udp__clt_new (CapeErr err)
{
    SOCKET sock = INVALID_SOCKET;

    // in windows the WSA system must be initialized first
    if (!cape_net__init())
    {
        goto exit_and_cleanup;
    }

    // create socket as datagram
    sock = WSASocket (AF_INET, SOCK_DGRAM, IPPROTO_UDP, NULL, 0, WSA_FLAG_OVERLAPPED);
    if (sock == INVALID_SOCKET)
    {
        goto exit_and_cleanup;
    }

    {
        u_long mode = 1;  // 1 to enable non-blocking socket
        ioctlsocket(sock, FIONBIO, &mode);
    }

    // return the socket
    return (void*)sock;

exit_and_cleanup:

    // save the last system error into the error object
    cape_err_lastOSError (err);

    if (sock >= 0)
    {
        closesocket(sock);
    }

    return NULL;
}

//-----------------------------------------------------------------------------

void* cape_sock__udp__srv_new (const char* host, long port, CapeErr err)
{
  SOCKET sock = INVALID_SOCKET;
  struct sockaddr_in* addr = NULL;

  // in windows the WSA system must be initialized first
  if (!cape_net__init())
  {
    goto exit_and_cleanup;
  }

  sock = WSASocket (AF_INET, SOCK_DGRAM, IPPROTO_UDP, NULL, 0, WSA_FLAG_OVERLAPPED);
  if (sock == INVALID_SOCKET)
  {
    goto exit_and_cleanup;
  }

  addr = cape_net__resolve_os (host, port, FALSE, err);

  {
    int opt = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0)
    {
      goto exit_and_cleanup;
    }

    if (bind(sock, (SOCKADDR*)&(addr), sizeof(addr)) != 0)
    {
      goto exit_and_cleanup;
    }
  }

  return (void*)sock;

exit_and_cleanup:

  // save the last system error into the error object
  cape_err_lastOSError (err);
  cape_net__resolve_del (&addr);

  if (sock != INVALID_SOCKET)
  {
    closesocket (sock);
  }

  return NULL;
}

//-----------------------------------------------------------------------------

void* cape_sock__icmp__new (CapeErr err)
{
  return NULL;
}

//-----------------------------------------------------------------------------

void cape_sock__close (void* sock)
{
  closesocket ((SOCKET)sock);
}

//-----------------------------------------------------------------------------

int cape_sock__noneblocking (void* sock, CapeErr err)
{
  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

#endif
