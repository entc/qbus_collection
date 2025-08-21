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

int cape_sock__set_host (struct sockaddr_in* addr, const char* host, long port, CapeErr err)
{
  int res;
  const struct hostent* server;
  
  memset (addr, 0, sizeof(struct sockaddr_in));

  addr->sin_family = AF_INET;      // set the network type
  addr->sin_port = htons((u_short)port);    // set the port

  // set the address
  if (host == NULL)
  {
    addr->sin_addr.s_addr = INADDR_ANY;
    
    res = CAPE_ERR_NONE;
    goto cleanup_and_exit;
  }

  // try to resolve the DNS entry
  server = gethostbyname (host);
  
  if (NULL == server)
  {
    cape_err_lastOSError (err);
    
    cape_log_fmt (CAPE_LL_ERROR, "CAPE", "socket", "can't resolve hostname ['%s']: %s", host, cape_err_text (err));

    res = cape_err_code (err);
    goto cleanup_and_exit;
  }
    
  memcpy (&(addr->sin_addr.s_addr), server->h_addr, server->h_length);
  res = CAPE_ERR_NONE;
  
cleanup_and_exit:
  
  return res;
}

//-----------------------------------------------------------------------------

#if defined __LINUX_OS || defined __BSD_OS

//-----------------------------------------------------------------------------

void* cape_sock__tcp__clt_new (const char* host, long port, CapeErr err)
{
  struct sockaddr_in addr;
  long sock;

  if (cape_sock__set_host (&addr, host, port, err))
  {
    goto exit;
  }

  // create socket
  sock = socket (AF_INET, SOCK_STREAM, 0);
  if (sock < 0)
  {
    goto exit;
  }

  // make the socket none-blocking
  {
    int flags = fcntl(sock, F_GETFL, 0);

    if (flags == -1)
    {
      goto exit;
    }

    flags |= O_NONBLOCK;

    if (fcntl(sock, F_SETFL, flags) != 0)
    {
      goto exit;
    }
  }

  // connect, don't check result because it is none-blocking
  connect (sock, (const struct sockaddr*)&(addr), sizeof(addr));

  return (void*)sock;

  exit:

  // save the last system error into the error object
  cape_err_lastOSError (err);

  if (sock >= 0)
  {
    close(sock);
  }

  return NULL;
}

//-----------------------------------------------------------------------------

void* cape_sock__tcp__srv_new  (const char* host, long port, CapeErr err)
{
  struct sockaddr_in addr;
  long sock = -1;

  if (cape_sock__set_host (&addr, host, port, err))
  {
    goto exit;
  }

  // create socket
  sock = socket (AF_INET, SOCK_STREAM, 0);
  if (sock < 0)
  {
    goto exit;
  }

  {
    int opt = 1;
    if(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0)
    {
      goto exit;
    }

    if (bind(sock, (const struct sockaddr*)&(addr), sizeof(addr)) < 0)
    {
      goto exit;
    }
  }

  // cannot fail
  listen(sock, SOMAXCONN);

  cape_log_fmt (CAPE_LL_TRACE, "CAPE", "cape_socket", "listen on [%s:%li]", host, port);

  return (void*)sock;

exit:

  // save the last system error into the error object
  cape_err_lastOSError (err);

  if (sock >= 0)
  {
    close(sock);
  }

  return NULL;
}

//-----------------------------------------------------------------------------

void* cape_sock__udp__clt_new (const char* host, long port, CapeErr err)
{
  struct sockaddr_in addr;
  long sock = -1;

  if (cape_sock__set_host (&addr, host, port, err))
  {
    goto exit_and_cleanup;
  }

  // create socket as datagram
#if defined __LINUX_OS
  sock = socket (AF_INET, SOCK_DGRAM | SOCK_NONBLOCK, IPPROTO_UDP);
#else
  sock = socket (AF_INET, SOCK_DGRAM, IPPROTO_UDP);
#endif

  if (sock < 0)
  {
    goto exit_and_cleanup;
  }

  // make the socket none-blocking
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

  cape_log_fmt (CAPE_LL_DEBUG, "CAPE", "socket clt UDP", "open socket on %s:%i", host, port);

  // return the socket
  return (void*)sock;

exit_and_cleanup:

  // save the last system error into the error object
  cape_err_lastOSError (err);

  if (sock >= 0)
  {
    close(sock);
  }

  return NULL;
}

//-----------------------------------------------------------------------------

void* cape_sock__udp__srv_new (const char* host, long port, CapeErr err)
{
  struct sockaddr_in addr;
  long sock = -1;

  if (cape_sock__set_host (&addr, host, port, err))
  {
    goto exit_and_cleanup;
  }

  // create socket
#if defined __LINUX_OS
  sock = socket (AF_INET, SOCK_DGRAM | SOCK_NONBLOCK, IPPROTO_UDP);
#else
  sock = socket (AF_INET, SOCK_DGRAM, IPPROTO_UDP);
#endif

  if (sock < 0)
  {
    goto exit_and_cleanup;
  }

  {
    int opt = 1;

    // set the socket option to reuse the address
    if (setsockopt (sock, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0)
    {
      goto exit_and_cleanup;
    }

    // try to bind the socket to an address
    if (bind (sock, (const struct sockaddr*)&(addr), sizeof(addr)) < 0)
    {
      goto exit_and_cleanup;
    }
  }

  // make the socket none-blocking
  {
    // get current flags
    int flags = fcntl (sock, F_GETFL, 0);

    if (flags == -1)
    {
      goto exit_and_cleanup;
    }

    // add noneblocking flag
    flags |= O_NONBLOCK;

    // set flags
    if (fcntl (sock, F_SETFL, flags) != 0)
    {
      goto exit_and_cleanup;
    }
  }

  cape_log_fmt (CAPE_LL_DEBUG, "CAPE", "socket srv UDP", "open socket on %s:%i", host, port);

  // return the socket
  return (void*)sock;

exit_and_cleanup:

  // save the last system error into the error object
  cape_err_lastOSError (err);

  if (sock >= 0)
  {
    close(sock);
  }

  return NULL;
}

//-----------------------------------------------------------------------------

void* cape_sock__icmp__new (CapeErr err)
{
  long sock = -1;

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

void cape_net__ntop (struct sockaddr* sa, char* bufdat, number_t buflen)
{
  switch (sa->sa_family) 
  {
    case AF_INET:
    {
      inet_ntop (AF_INET, &(((struct sockaddr_in *)sa)->sin_addr), bufdat, buflen);
      break;
    }
    case AF_INET6:
    {
      inet_ntop (AF_INET6, &(((struct sockaddr_in6 *)sa)->sin6_addr), bufdat, buflen);
      break;
    }
  }
}

//-----------------------------------------------------------------------------

CapeString cape_net__resolve (const CapeString host, int ipv6, CapeErr err)
{
  CapeString ret = NULL;

  int res;
  struct addrinfo* addr_result;
  
  res = getaddrinfo (host, 0, 0, &addr_result);
  
  if (res != 0)
  {
    cape_err_lastOSError (err);
    goto exit_and_cleanup;
  }
  else
  {
    /*
    struct addrinfo *a;
    char address[64];
    
    for (a = addr; a; a = a->ai_next)
    {
      cape_net__ntop (a->ai_addr, address, 64);
      
      cape_log_fmt (CAPE_LL_TRACE, "CAPE", "resolve", "%s", address);
    }
    */
  }
  
  {
    struct addrinfo* addr_current = addr_result;
    
    while (addr_current && addr_current->ai_family != AF_INET)
    {
      addr_current = addr_current->ai_next;
    }
    
    if (addr_current)
    {
      ret = CAPE_ALLOC (65);
      
      cape_net__ntop (addr_current->ai_addr, ret, 64);
    }
  }
  
exit_and_cleanup:
  
  freeaddrinfo (addr_result);
  return ret;
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

void cape_net__ntop (LPSOCKADDR sa, DWORD length, char* bufdat, number_t buflen)
{
	DWORD buflen_local = (DWORD)buflen;

	switch (sa->sa_family)
	{
		case AF_INET:
		{
			WSAAddressToStringA (sa, length, NULL, (LPSTR)bufdat, &buflen_local);
			break;
		}
		case AF_INET6:
		{
			WSAAddressToString (sa, length, NULL, (LPSTR)bufdat, &buflen_local);
			break;
		}
	}
}

//-----------------------------------------------------------------------------

CapeString cape_net__resolve (const CapeString host, int ipv6, CapeErr err)
{
	CapeString ret = NULL;

	int res;

	// local objects
	ADDRINFOA* addr_result = NULL;

	// in windows the WSA system must be initialized first
	if (!init_wsa())
	{
		goto exit_and_cleanup;
	}

	res = GetAddrInfoA (host, 0, 0, &addr_result);

	if (res != 0)
	{
		cape_err_lastOSError(err);
		goto exit_and_cleanup;
	}
	else
	{
		/*
		ADDRINFOA* a;
		char address[64];

		for (a = addr_result; a; a = a->ai_next)
		{
			cape_net__ntop ((LPSOCKADDR)a->ai_addr, (DWORD)a->ai_addrlen, address, 64);

			cape_log_fmt(CAPE_LL_TRACE, "CAPE", "resolve", "%s", address);
		}
		*/
	}

	{
		ADDRINFOA* addr_current = addr_result;

		while (addr_current && addr_current->ai_family != AF_INET)
		{
			addr_current = addr_current->ai_next;
		}

		if (addr_current)
		{
			ret = CAPE_ALLOC(65);

			cape_net__ntop ((LPSOCKADDR)addr_current->ai_addr, (DWORD)addr_current->ai_addrlen, ret, 64);
		}
	}

exit_and_cleanup:

	FreeAddrInfoA (addr_result);
	return ret;
}

//-----------------------------------------------------------------------------

#endif
