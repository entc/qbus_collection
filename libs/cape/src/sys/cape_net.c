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

int cape_net__init (void)
{
    return TRUE;   // return always true
}

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

CapeSockaddr cape_net__resolve_os (const CapeString host, int port, int ipv6, CapeErr err)
{
  CapeSockaddr ret = NULL;
  struct addrinfo* addr_result = NULL;

  if (NULL == host)
  {
    ret = CAPE_NEW(struct sockaddr_in);

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
      /*
      struct addrinfo *a;
      char address[64];

      for (a = addr_result; a; a = a->ai_next)
      {
        cape_net__ntop (a->ai_addr, address, 64);

        cape_log_fmt (CAPE_LL_TRACE, "CAPE", "resolve", "%s", address);
      }
       */
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
      ret = CAPE_NEW (struct sockaddr_in);

      {
        char address[64];
        cape_net__ntop (addr_current->ai_addr, address, 64);

        cape_log_fmt (CAPE_LL_TRACE, "CAPE", "resolve", "use address [%s:%i]", address, port);
      }
      
      memcpy (ret, addr_current->ai_addr, sizeof(struct sockaddr_in));  // set the address
      ret->sin_port = htons(port);    // set the port
    }
  }

exit_and_cleanup:

  if (addr_result)
  {
      freeaddrinfo (addr_result);
  }

  return ret;
}

//-----------------------------------------------------------------------------

void cape_net__resolve_del (struct addrinfo** p_self)
{
  if (*p_self)
  {
      freeaddrinfo (*p_self);
      *p_self = NULL;
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

#elif defined _WIN64 || defined _WIN32

//-----------------------------------------------------------------------------

static INIT_ONCE g_wsa_init_once = INIT_ONCE_STATIC_INIT;
static int g_wsa_initialized = 0;

static BOOL CALLBACK wsa_init_callback (PINIT_ONCE InitOnce, PVOID Parameter, PVOID* Context)
{
    WSADATA wsa;

    if (WSAStartup (MAKEWORD (2, 2), &wsa) != 0)
    {
        return FALSE;
    }

    g_wsa_initialized = 1;
    return TRUE;
}

//-----------------------------------------------------------------------------

int cape_net__init (void)
{
    BOOL ok = InitOnceExecuteOnce (&g_wsa_init_once, wsa_init_callback, NULL, NULL);

    return ok && g_wsa_initialized;
}

//-----------------------------------------------------------------------------

void cape_net__ntop (LPSOCKADDR sa, DWORD length, char* bufdat, number_t buflen)
{
    DWORD buflen_local = (DWORD)buflen;

    switch (sa->sa_family)
    {
        case AF_INET:
        {
            WSAAddressToString (sa, length, NULL, (LPSTR)bufdat, &buflen_local);
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

struct addrinfo* cape_net__resolve_os (const CapeString host, int port, int ipv6, CapeErr err)
{
    struct addrinfo* ret = NULL;
    struct addrinfo* addr_result = NULL;

    // in windows the WSA system must be initialized first
    if (!cape_net__init())
    {
        return ret;
    }

    // resolve the address
    {
        struct addrinfo hints;
        int errcode;

        ZeroMemory (&hints, sizeof(hints));
        
        hints.ai_family = ipv6 ? AF_INET6 : AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;

        if (host == NULL)
        {
            hints.ai_flags = AI_PASSIVE;
        }

        errcode = GetAddrInfoA (host, NULL, &hints, &addr_result);
        if (errcode)
        {
            cape_err_set (err, CAPE_ERR_OS, gai_strerror (errcode));

            cape_log_fmt (CAPE_LL_ERROR, "CAPE", "socket", "can't resolve hostname [%s]: %s", host, cape_err_text(err));

            goto exit_and_cleanup;
        }
    }

    // find the first ip address
    {
        struct addrinfo* addr_current = addr_result;
        int family = ipv6 ? AF_INET6 : AF_INET;

        // find the first address
        while (addr_current && (addr_current->ai_family != family))
        {
            addr_current = addr_current->ai_next;
        }

        if (addr_current)
        {
            // for output purposes
            {
                char address[64];
                cape_net__ntop ((LPSOCKADDR)addr_current->ai_addr, (DWORD)addr_current->ai_addrlen, address, 64);

                cape_log_fmt(CAPE_LL_TRACE, "CAPE", "resolve", "use address [%s:%i]", address, port);
            }

            ret = CAPE_CALLOC (1, sizeof(struct addrinfo));

            /* Copy all scalar fields */
            ret->ai_flags = addr_current->ai_flags;
            ret->ai_family = addr_current->ai_family;
            ret->ai_socktype = addr_current->ai_socktype;
            ret->ai_protocol = addr_current->ai_protocol;

            /* Deep copy the socket address */
            ret->ai_addrlen = addr_current->ai_addrlen;
            ret->ai_addr = CAPE_ALLOC (addr_current->ai_addrlen);

            memcpy (ret->ai_addr, addr_current->ai_addr, addr_current->ai_addrlen);

            /* copy canonical name */
            ret->ai_canonname = cape_str_cp (addr_current->ai_canonname);
            ret->ai_next = NULL;
        }
    }

exit_and_cleanup:

    if (addr_result)
    {
        FreeAddrInfoA (addr_result);
    }

    return ret;
}

//-----------------------------------------------------------------------------

void cape_net__resolve_del (struct addrinfo** p_self)
{
    if (*p_self)
    {
        struct addrinfo* self = *p_self;

        if (self->ai_addr)
        {
            CAPE_FREE (self->ai_addr);
        }

        cape_str_del (&(self->ai_canonname));

        CAPE_FREE (self);
        *p_self = NULL;
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
    if (!cape_net__init())
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

            cape_net__ntop((LPSOCKADDR)addr_current->ai_addr, (DWORD)addr_current->ai_addrlen, ret, 64);
        }
    }

exit_and_cleanup:

    if (addr_result)
    {
        FreeAddrInfoA (addr_result);
    }

    return ret;
}

//-----------------------------------------------------------------------------

#endif

//-----------------------------------------------------------------------------
