#include "cape_net.h"
#include "cape_log.h"

//-----------------------------------------------------------------------------

#if defined __LINUX_OS || defined __BSD_OS

// c includes
#include <memory.h>
#include <sys/types.h>
#include <arpa/inet.h>  // inet(3) functions
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

//-----------------------------------------------------------------------------

void cape_net__ntop (struct sockaddr* sa, char* bufdat, number_t buflen)
{
  switch (sa->sa_family)
  {
    case AF_INET:
    {
      inet_ntop (AF_INET, &(((struct sockaddr_in *)sa)->sin_addr), bufdat, (socklen_t)buflen);
      break;
    }
    case AF_INET6:
    {
      inet_ntop (AF_INET6, &(((struct sockaddr_in6 *)sa)->sin6_addr), bufdat, (socklen_t)buflen);
      break;
    }
  }
}

//-----------------------------------------------------------------------------

struct sockaddr_in* cape_net__resolve_os (const CapeString host, u_short port, int ipv6, CapeErr err)
{
  struct sockaddr_in* ret = NULL;
  struct addrinfo* addr_result;

  if (NULL == host)
  {
    ret = CAPE_ALLOC(sizeof(struct sockaddr_in));

    ret->sin_family = AF_INET;      // set the network type
    ret->sin_port = htons(port);    // set the port

    ret->sin_addr.s_addr = INADDR_ANY;

    return ret;
  }

  {
    int errcode = getaddrinfo (host, 0, 0, &addr_result);
    if (errcode)
    {
      cape_err_set (err, CAPE_ERR_OS, gai_strerror (errcode));

      cape_log_fmt (CAPE_LL_ERROR, "CAPE", "socket", "can't resolve hostname [%s]: %s", host, cape_err_text (err));

      goto exit_and_cleanup;
    }
    else
    {
      struct addrinfo *a;
      char address[64];

      for (a = addr_result; a; a = a->ai_next)
      {
        cape_net__ntop (a->ai_addr, address, 64);

        cape_log_fmt (CAPE_LL_TRACE, "CAPE", "resolve", "%s", address);
      }
    }
  }

  // find the first ip address
  {
    struct addrinfo* addr_current = addr_result;

    while (addr_current && addr_current->ai_family != AF_INET)
    {
      addr_current = addr_current->ai_next;
    }

    if (addr_current)
    {
      ret = CAPE_ALLOC(sizeof(struct sockaddr_in));

      memcpy (ret, addr_current->ai_addr, sizeof(struct sockaddr_in));
    }
  }

exit_and_cleanup:

  freeaddrinfo (addr_result);
  return ret;
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

#elif defined _WIN64 || defined _WIN32

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

//-----------------------------------------------------------------------------
