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
#include <stdio.h>

#elif defined _WIN64 || defined _WIN32

#include <ws2tcpip.h>
#include <winsock2.h>

#include <windows.h>
#include <stdio.h>
#include <stdint.h>

#endif

//-----------------------------------------------------------------------------

struct addrinfo* cape_net__new (int flags, int family, int socktype, int protocol, void* addr_buf, number_t addr_len, const CapeString canonname)
{
    struct addrinfo* self = CAPE_CALLOC (1, sizeof(struct addrinfo));

    /* Copy all scalar fields */
    self->ai_flags = flags;
    self->ai_family = family;
    self->ai_socktype = socktype;
    self->ai_protocol = protocol;

    /* Deep copy the socket address */
    self->ai_addrlen = (socklen_t)addr_len;
    self->ai_addr = CAPE_ALLOC (addr_len);

    memcpy (self->ai_addr, addr_buf, addr_len);

    /* copy canonical name */
    self->ai_canonname = cape_str_cp (canonname);
    self->ai_next = NULL;
    
    return self;
}

//-----------------------------------------------------------------------------

struct addrinfo* cape_net__new_simple (int flags, int family, int socktype, int protocol, const CapeString host, int port, const CapeString canonname)
{
    struct addrinfo* self;

    if (!host)
    {
        cape_log_msg (CAPE_LL_ERROR, "CAPE", "NET", "can't create addrinfo: host is NULL");
        return NULL;
    }

    self = CAPE_CALLOC (1, sizeof(struct addrinfo));
    
    /* Copy all scalar fields */
    self->ai_flags = flags;
    self->ai_family = family;
    self->ai_socktype = socktype;
    self->ai_protocol = protocol;

    self->ai_addrlen = sizeof(struct sockaddr_storage);
    self->ai_addr = CAPE_CALLOC (1, self->ai_addrlen);
    
    if (family == AF_INET)
    {
        struct sockaddr_in* sa4 = (struct sockaddr_in*)self->ai_addr;
        
        sa4->sin_family = AF_INET;
        sa4->sin_port = htons((uint16_t)port);

        if (inet_pton(AF_INET, host, &sa4->sin_addr) != 1)
        {
            cape_log_msg (CAPE_LL_ERROR, "CAPE", "NET", "can't create addrinfo: host [IPV4] is invalid");

            cape_net__resolve_del (&self);
            return NULL;
        }
    }
    else if (family == AF_INET6)
    {
        struct sockaddr_in6* sa6 = (struct sockaddr_in6*)self->ai_addr;
        
        sa6->sin6_family = AF_INET6;
        sa6->sin6_port = htons((uint16_t)port);

        if (inet_pton (AF_INET6, host, &sa6->sin6_addr) != 1)
        {
            cape_log_msg (CAPE_LL_ERROR, "CAPE", "NET", "can't create addrinfo: host [IPV6] is invalid");

            cape_net__resolve_del (&self);
            return NULL;
        }
    }
    else
    {
        cape_log_msg (CAPE_LL_ERROR, "CAPE", "NET", "can't create addrinfo: family is not supported");

        cape_net__resolve_del(&self);
        return NULL;
    }
    
    /* copy canonical name */
    self->ai_canonname = cape_str_cp (canonname);
    self->ai_next = NULL;
    
    return self;
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

int cape_net__ntop (struct addrinfo* ai, char* bufdat, number_t buflen, CapeErr err)
{
    if (!ai || !ai->ai_addr || !bufdat || buflen <= 0)
    {
        return cape_err_set (err, CAPE_ERR_MISSING_PARAM, "invalid parameters");
    }

    bufdat[0] = '\0';

    {
        const void* addr = NULL;
        int family = ai->ai_addr->sa_family;

        switch (family)
        {
            case AF_INET:
            {
                addr = &((struct sockaddr_in*) ai->ai_addr)->sin_addr;
                break;
            }

            case AF_INET6:
            {
                addr = &((struct sockaddr_in6*) ai->ai_addr)->sin6_addr;
                break;
            }

            default:
            {
                return cape_err_set (err, CAPE_ERR_NOT_SUPPORTED, "unsupported address family");
            }
        }
#if defined(CAPE_USE_FREERTOS) || defined(__LINUX_OS) || defined(__BSD_OS)

        if (inet_ntop (family, addr, bufdat, (socklen_t) buflen) != NULL)
        {
            return CAPE_ERR_NONE;
        }

#elif defined(__WINDOWS_OS)

        if (InetNtopA (family, (PVOID) addr, bufdat, (size_t) buflen) != NULL)
        {
            return CAPE_ERR_NONE;
        }

#else

        return cape_err_set (err, CAPE_ERR_NOT_SUPPORTED, "inet_ntop is not supported on this platform");

#endif

    }
    
    bufdat[0] = '\0';
    return cape_err_set (err, CAPE_ERR_OS, "can't convert address");
}

//-----------------------------------------------------------------------------

CapeString cape_net__address (struct addrinfo* ai, CapeErr err)
{
    CapeString ret = cape_alloc (INET6_ADDRSTRLEN);
    
    if (cape_net__ntop (ai, ret, INET6_ADDRSTRLEN, err))
    {
        cape_str_del (&ret);
        return NULL;
    }
    else
    {
        return ret;
    }
}

//-----------------------------------------------------------------------------

void cape_net__print (struct addrinfo* ai)
{
    CapeErr err = cape_err_new ();
    
    CapeString h = cape_net__address (ai, err);
    if (h)
    {
        cape_log_fmt(CAPE_LL_TRACE, "CAPE", "NET", "addrinfo: %s", h);
    }
    
    cape_str_del (&h);
    cape_err_del (&err);
}

//-----------------------------------------------------------------------------

#if defined __LINUX_OS || defined __BSD_OS

//-----------------------------------------------------------------------------

int cape_net__init (void)
{
    return TRUE;   // return always true
}

//-----------------------------------------------------------------------------

struct addrinfo* cape_net__resolve_os (const CapeString host, int port, int ipv6, CapeErr err)
{
    struct addrinfo* ret = NULL;
    struct addrinfo* addr_result = NULL;

    // resolve the address
    {
        struct addrinfo hints = {0};
        int errcode;

        hints.ai_family = ipv6 ? AF_INET6 : AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;

        if (host == NULL)
        {
            hints.ai_flags = AI_PASSIVE;
        }

        char portstr[16];
        snprintf(portstr, sizeof(portstr), "%d", port);
        
        errcode = getaddrinfo (host, portstr, &hints, &addr_result);
        if (errcode)
        {
            cape_err_set (err, CAPE_ERR_OS, gai_strerror (errcode));

            cape_log_fmt (CAPE_LL_ERROR, "CAPE", "socket", "can't resolve hostname [%s]: %s", host, cape_err_text(err));

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
                
                if (cape_net__ntop (addr_current, address, 64, err))
                {
                    // some error
                }
                else
                {
                    cape_log_fmt(CAPE_LL_TRACE, "CAPE", "resolve", "use address [%s:%i]", address, port);
                }
            }

            ret = cape_net__new (addr_current->ai_flags, addr_current->ai_family, addr_current->ai_socktype, addr_current->ai_protocol, addr_current->ai_addr, addr_current->ai_addrlen, addr_current->ai_canonname);
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

        if (cape_net__ntop (addr_current, ret, 64, err))
        {
            CAPE_FREE (ret);
            ret = NULL;
        }
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

void cape_net__print (struct addrinfo* self)
{
    char address[64];

    cape_net__ntop((LPSOCKADDR)self->ai_addr, (DWORD)self->ai_addrlen, address, 64);

    cape_log_fmt(CAPE_LL_TRACE, "CAPE", "NET", "addrinfo: %s", address);
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
        char service[16];

        ZeroMemory (&hints, sizeof(hints));
        
        hints.ai_family = ipv6 ? AF_INET6 : AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;

        if (host == NULL)
        {
            hints.ai_flags = AI_PASSIVE;
        }

        _snprintf(service, sizeof(service), "%d", port);

        errcode = GetAddrInfoA (host, service, &hints, &addr_result);
        if (errcode)
        {
            cape_err_set (err, CAPE_ERR_OS, gai_strerror (errcode));

            cape_log_fmt (CAPE_LL_ERROR, "CAPE", "NET", "can't resolve hostname [%s]: %s", host, cape_err_text(err));

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

                cape_log_fmt(CAPE_LL_TRACE, "CAPE", "NET", "use address %s", address);
            }

            ret = cape_net__new (addr_current->ai_flags, addr_current->ai_family, addr_current->ai_socktype, addr_current->ai_protocol, addr_current->ai_addr, addr_current->ai_addrlen, addr_current->ai_canonname);
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
