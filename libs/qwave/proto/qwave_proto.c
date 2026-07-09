#include <fmt/cape_args.h>

// cape includes
#include <sys/cape_aio.h>
#include <sys/cape_socket.h>

#define PORT 8000
#define MAX_EVENTS 64
#define BUFFER_SIZE 4096

//-----------------------------------------------------------------------------

/*
int set_nonblocking (int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}
*/
//-----------------------------------------------------------------------------

/*
void proto_accept (int server_fd)
{
  while (1)
  {
    int client = accept(server_fd, NULL, NULL);
    
    if (client == -1)
    {
      if (errno == EAGAIN || errno == EWOULDBLOCK)
      {
        break;
      }
      else
      {
        perror("accept");
      }
    }
    
    set_nonblocking(client);
    
    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = client;
    
    epoll_ctl (epoll_fd, EPOLL_CTL_ADD, client, &ev);
  }
}
*/
//-----------------------------------------------------------------------------

/*
void proto_read (int fd)
{
    char buffer[BUFFER_SIZE];

    while (1)
    {
        int n = read(fd, buffer, sizeof(buffer));

        if (n == -1)
        {
            if (errno == EAGAIN)
            {
                break;
            }
            else
            {
                perror("read");
                close(fd);
                break;
            }
        }

        if (n == 0)
        {
            close(fd);
            break;
        }

        {
            char response[] =
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/plain\r\n"
                "Content-Length: 12\r\n"
                "\r\n"
                "Hello World";

            write(client, response, strlen(response));
            close(client);
        }
    }
}
 
void write ()
 {
 
 while (bytes_left > 0) {

     int n = write(fd, buffer + sent, bytes_left);

     if (n == -1) {

         if (errno == EAGAIN) {
             // später wieder EPOLLOUT warten
             break;
         }

         close(fd);
         return;
     }

     sent += n;
     bytes_left -= n;
 }
 
 Wenn Buffer voll ist → EPOLLOUT registrieren.
 }
 
 loop ()
 {
 
 for (int i = 0; i < n; i++) {

     int fd = events[i].data.fd;

     if (fd == server_fd) {

         while (1) {

             int client = accept(server_fd, NULL, NULL);

             if (client == -1) {
                 if (errno == EAGAIN)
                     break;
                 else
                     perror("accept");
             }

             set_nonblocking(client);

             struct epoll_event ev;
             ev.events = EPOLLIN | EPOLLET;
             ev.data.fd = client;

             epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client, &ev);
         }

     } else {

         while (1) {

             char buf[4096];
             int count = read(fd, buf, sizeof(buf));

             if (count == -1) {

                 if (errno == EAGAIN)
                     break;

                 close(fd);
                 break;
             }

             if (count == 0) {
                 close(fd);
                 break;
             }

             handle_request(fd, buf, count);
         }
     }
 }
 }
*/
//-----------------------------------------------------------------------------

int main (int argc, char *argv[])
{
  int res;

  // local objects
  CapeErr err = cape_err_new ();
  CapeAio aio = cape_aio_new ();
  CapeUdc parameters = cape_args_from_args (argc, argv, NULL);
  CapeAioItem aio_server_item = NULL;
    
  // create a server tcp socket
  void* server_handle = cape_sock__tcp__srv_new ("127.0.0.1", 8000, err);
  if (NULL == server_handle)
  {
    res = cape_err_code (err);
    goto cleanup_and_exit;
  }

  // set the server handle to none-blocking
  res = cape_sock__noneblocking (server_handle, err);
  if (res)
  {
    goto cleanup_and_exit;
  }
  
  // initialize the event handling
  res = cape_aio_init (aio, err);
  if (res)
  {
    goto cleanup_and_exit;
  }

  aio_server_item = cape_aio_add (aio, server_handle, CAPE_AIO_MODE__RECV, err);
  if (NULL == aio_server_item)
  {
    res = cape_err_code (err);
    goto cleanup_and_exit;
  }

  res = cape_aio_wait (aio, err);
  if (res)
  {
      goto cleanup_and_exit;
  }
  
  /*
  while (1)
  {
      int n = epoll_wait (epoll_fd, events, MAX_EVENTS, -1);

      for (int i = 0; i < n; i++)
      {
          if (events[i].data.fd == server_fd)
          {
              proto_accept ();
          }
          else
          {
              proto_read (events[i].data.fd);
            
              int client = ;

              char buffer[BUFFER_SIZE];
              int bytes = read(client, buffer, BUFFER_SIZE);

              if (bytes <= 0)
              {
                  close(client);
                  continue;
              }

          }
      }
  }
*/
cleanup_and_exit:
  
  cape_udc_del (&parameters);
  cape_err_del (&err);

  return res;
}
