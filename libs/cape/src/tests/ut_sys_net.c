#include "sys/cape_net.h"
#include "stc/cape_str.h"

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

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <ws2tcpip.h>
#include <winsock2.h>

#include <windows.h>
#include <stdio.h>

#endif

//-----------------------------------------------------------------------------

int main (int argc, char *argv[])
{
	int res;
	CapeErr err = cape_err_new();

	// local objects
	struct addrinfo* addr1 = NULL;
	struct addrinfo* addr2 = NULL;
	CapeString h1 = NULL;
	
	h1 = cape_net__resolve("google.com", FALSE, err);
	if (NULL == h1)
	{
		res = cape_err_code(err);
		goto exit_and_cleanup;
	}

	printf ("HOST1: %s\n", h1);
	cape_str_del(&h1);

	addr1 = cape_net__new_simple (0, AF_INET, SOCK_DGRAM, IPPROTO_UDP, "127.0.0.1", 8080, NULL);
	if (NULL == addr1)
	{
		res = cape_err_code(err);
		goto exit_and_cleanup;
	}

	cape_net__print (addr1);
	cape_net__resolve_del (&addr1);

	addr1 = cape_net__resolve_os ("google.com", 8080, FALSE, err);
	if (NULL == addr1)
	{
		res = cape_err_code(err);
		goto exit_and_cleanup;
	}

	cape_net__print(addr1);

	addr2 = cape_net__new (0, AF_INET, SOCK_DGRAM, IPPROTO_UDP, addr1->ai_addr, addr1->ai_addrlen, NULL);
	if (NULL == addr2)
	{
		res = cape_err_code(err);
		goto exit_and_cleanup;
	}

	cape_net__print(addr2);

	cape_net__resolve_del(&addr1);
	cape_net__resolve_del(&addr2);

	addr1 = cape_net__resolve_os("google.com", 8080, TRUE, err);
	if (NULL == addr1)
	{
		res = cape_err_code(err);
		goto exit_and_cleanup;
	}

	cape_net__print(addr1);
	cape_net__resolve_del(&addr1);

	res = CAPE_ERR_NONE;

exit_and_cleanup:

	cape_str_del (&h1);

	cape_err_del (&err);
	return res;
}

//-----------------------------------------------------------------------------
