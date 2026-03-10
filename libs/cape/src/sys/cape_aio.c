#include "cape_aio.h"

//-----------------------------------------------------------------------------

#if defined __LINUX_OS


#elif defined __BSD_OS

#include <sys/event.h>

#elif defined _WIN64 || defined _WIN32

#include <ws2tcpip.h>
#include <winsock2.h>

#include <windows.h>
#include <stdio.h>

#endif

#define MAX_EVENTS 64

//-----------------------------------------------------------------------------

struct CapeAio_s
{
#if defined __LINUX_OS

  int epoll_fd;

#elif defined __BSD_OS

  int kq;

#elif defined _WIN64 || defined _WIN32


#endif
};

//-----------------------------------------------------------------------------

CapeAio cape_aio_new (void)
{
  
  
}

//-----------------------------------------------------------------------------

void cape_aio_del (CapeAio* p_self)
{
  
  
}

//-----------------------------------------------------------------------------

int cape_aio_init (CapeAio self, CapeErr err)
{
#if defined __LINUX_OS

  self->epoll_fd = epoll_create1 (0);

#elif defined __BSD_OS

  self->kq = kqueue();

#elif defined _WIN64 || defined _WIN32


#endif
}

//-----------------------------------------------------------------------------

int cape_aio_next (CapeAio self, CapeErr err)
{
#if defined __LINUX_OS


#elif defined __BSD_OS

  struct kevent events[MAX_EVENTS];
  
  int nevents = kevent (self->kq, NULL, 0, events, MAX_EVENTS, NULL);

  for (int i = 0; i < nevents; i++)
  {
    
    
        int fd = (int)events[i].ident;

        if (fd == server_fd) {
            // Neue Verbindungen
            while (1) {
                int client = accept(server_fd, NULL, NULL);
                if (client < 0) {
                    if (errno == EAGAIN || errno == EWOULDBLOCK)
                        break;
                    perror("accept");
                    break;
                }

                set_nonblocking(client);
                EV_SET(&change, client, EVFILT_READ, EV_ADD, 0, 0, NULL);
                kevent(kq, &change, 1, NULL, 0, NULL);
            }
        } else {
            // Client-Daten lesen
            char buffer[BUFFER_SIZE];
            ssize_t n = read(fd, buffer, sizeof(buffer));
            if (n <= 0) {
                close(fd);
                continue;
            }

            char response[] =
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/plain\r\n"
                "Content-Length: 12\r\n"
                "\r\n"
                "Hello World";

            write(fd, response, sizeof(response)-1);
            close(fd);
        }
    }
  
#elif defined _WIN64 || defined _WIN32


#endif
}

//-----------------------------------------------------------------------------

CapeAioItem cape_aio_add (CapeAio self, void* handle, CapeErr err)
{
#if defined __LINUX_OS

  struct epoll_event ev, events[MAX_EVENTS];
  ev.events = EPOLLIN;
  ev.data.fd = server_fd;

  epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &ev);

#elif defined __BSD_OS

  struct kevent change;
  
  EV_SET (&change, (int)(number_t)handle, EVFILT_READ, EV_ADD, 0, 0, NULL);
  kevent (self->kq, &change, 1, NULL, 0, NULL);

#elif defined _WIN64 || defined _WIN32


#endif
}

//-----------------------------------------------------------------------------

void cape_aio_rm (CapeAio self, CapeAioItem* p_hitem)
{
  
}

//-----------------------------------------------------------------------------
