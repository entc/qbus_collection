#include "cape_socket.h"

// cape includes
#include "sys/cape_log.h"

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

struct sockaddr_in* cape_net__resolve_os (const CapeString host, u_short port, int ipv6, CapeErr err);

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

void* cape_sock__udp__clt_new (const char* host, long port, CapeErr err)
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

  cape_log_fmt (CAPE_LL_DEBUG, "CAPE", "socket clt UDP", "open socket on %s:%i", host, port);

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

  long sock = CAPE_SOCKET_INVALID;
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
    if (fcntl (sock, F_SETFL, flags) != 0)
    {
      // save the last system error into the error object
      cape_err_lastOSError (err);

      goto cleanup_and_exit;
    }
  }

  cape_log_fmt (CAPE_LL_DEBUG, "CAPE", "socket srv UDP", "open socket on %s:%i", host, port);

  ret = (void*)sock;
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

void cape_sock__close (void* sock)
{
  close ((long)sock);
}

//-----------------------------------------------------------------------------

int cape_sock__noneblocking (void* sock, CapeErr err)
{
  // save the current flags
  int flags = fcntl ((long)sock, F_GETFL, 0);
  if (flags == -1)
  {
    return cape_err_lastOSError (err);
  }

  // add noneblocking
  flags |= O_NONBLOCK;

  // apply the flags
  if (fcntl ((long)sock, F_SETFL, flags) != 0)
  {
    return cape_err_lastOSError (err);
  }

  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

#elif defined _WIN64 || defined _WIN32

//-----------------------------------------------------------------------------

static int init_wsa (void)
{
  static WSADATA wsa;

  return (WSAStartup (MAKEWORD(2,2), &wsa) == 0);
}

//-----------------------------------------------------------------------------

void* cape_sock__tcp__clt_new (const char* host, long port, CapeErr err)
{
  // TODO: needs to be done

  return NULL;
}

//-----------------------------------------------------------------------------

void* cape_sock__tcp__srv_new (const char* host, long port, CapeErr err)
{
  struct addrinfo hints;

  // local variables
  struct addrinfo* addr = NULL;
  SOCKET sock = INVALID_SOCKET;

  // in windows the WSA system must be initialized first
  if (!init_wsa ())
  {
    goto exit_and_cleanup;
  }

  ZeroMemory(&hints, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;
  hints.ai_flags = AI_PASSIVE;

  {
    char buffer[10];
    sprintf_s (buffer, 10, "%u", port);

    if (getaddrinfo (host, buffer, &hints, &addr) != 0)
    {
      goto exit_and_cleanup;
    }
  }

  sock = socket (addr->ai_family, addr->ai_socktype, addr->ai_protocol);
  if (sock == INVALID_SOCKET)
  {
    goto exit_and_cleanup;
  }

  {
    int optVal = TRUE;
    int optLen = sizeof(int);

    if (getsockopt (sock, SOL_SOCKET, SO_REUSEADDR, (char*)&optVal, &optLen) != 0)
    {
      goto exit_and_cleanup;
    }
  }

  if (bind (sock, addr->ai_addr, (int)addr->ai_addrlen) == SOCKET_ERROR)
  {
    goto exit_and_cleanup;
  }

  // in windows this can fail
  if (listen (sock, SOMAXCONN) == SOCKET_ERROR)
  {
    goto exit_and_cleanup;
  }

  cape_log_fmt (CAPE_LL_TRACE, "CAPE", "cape_socket", "listen on [%s:%li]", host, port);

  freeaddrinfo (addr);

  return (void*)sock;

exit_and_cleanup:

  // save the last system error into the error object
  cape_err_formatErrorOS (err, WSAGetLastError());

  if (addr)
  {
    freeaddrinfo (addr);
  }

  if (sock != INVALID_SOCKET)
  {
    closesocket (sock);
  }

  return NULL;
}

//-----------------------------------------------------------------------------

void* cape_sock__udp__clt_new (const char* host, long port, CapeErr err)
{
  struct sockaddr_in addr;
  long sock = -1;

  // in windows the WSA system must be initialized first
  if (!init_wsa ())
  {
    goto exit_and_cleanup;
  }

  cape_sock__set_host (&addr, host, port);

  // create socket as datagram
  sock = socket (AF_INET, SOCK_DGRAM, 0);
  if (sock < 0)
  {
    goto exit_and_cleanup;
  }

  // make the socket none-blocking
  /*
  {
    int flags = fcntl(sock, F_GETFL, 0);

    if (flags == -1)
    {
      goto exit_and_cleanup;
    }

    flags |= O_NONBLOCK;

    if (fcntl(sock, F_SETFL, flags) != 0)
    {
      goto exit_and_cleanup;
    }
  }
  */

  {
    u_long mode = 1;  // 1 to enable non-blocking socket
    ioctlsocket (sock, FIONBIO, &mode);
  }

  cape_log_fmt (CAPE_LL_TRACE, "CAPE", "socket", "UDP socket clt on %s:%i", host, port);

  // return the socket
  return (void*)sock;

exit_and_cleanup:

  // save the last system error into the error object
  cape_err_lastOSError (err);

  if (sock >= 0)
  {
    closesocket (sock);
  }

  return NULL;
}

//-----------------------------------------------------------------------------

void* cape_sock__udp__srv_new (const char* host, long port, CapeErr err)
{
  SOCKET sock = INVALID_SOCKET;
  struct sockaddr_in addr;

  // in windows the WSA system must be initialized first
  if (!init_wsa ())
  {
    goto exit_and_cleanup;
  }

  sock = WSASocketA (AF_INET, SOCK_DGRAM, IPPROTO_UDP, NULL, 0, WSA_FLAG_OVERLAPPED);
  if (sock == INVALID_SOCKET)
  {
    goto exit_and_cleanup;
  }

  cape_sock__set_host (&addr, host, port);

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
