#include "qbus_tcp.h"

// cape includes
#include <sys/cape_log.h>
#include <sys/cape_socket.h>
#include <sys/cape_thread.h>
#include <sys/cape_mutex.h>
#include <fmt/cape_json.h>

#include <signal.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>

//-----------------------------------------------------------------------------

#define THREAD_SIGNAL_MSG__TERMINATE    'T'
#define THREAD_SIGNAL_MSG__UPDATE       'U'

//-----------------------------------------------------------------------------

struct QBusPvdCtx_s
{
  int pipe_fd[2];
  
  CapeThread worker;

  CapeMutex mutex;
  
  CapeList fds;
};

//-----------------------------------------------------------------------------

struct QBusPvdFD_s
{
  QBusPvdEntity entity;
  void* handle;
  
}; typedef struct QBusPvdFD_s* QBusPvdFD;

//-----------------------------------------------------------------------------

// forwarded functions
int pvd2_entity__event (QBusPvdEntity, QBusPvdFD* p_new_pfd);
QBusPvdEntity pvd2_entity__on_connect (QBusPvdCtx, void* handle, const CapeString host);
void pvd2_entity__on_done (QBusPvdEntity*);
void pvd2_entity__connect (QBusPvdEntity);

//-----------------------------------------------------------------------------

char pvd2_ctx__retrieve_last_ctrl (QBusPvdCtx self)
{
  char ret = THREAD_SIGNAL_MSG__UPDATE;
  char buffer[100];
  
  // reads a maximum of 100 control bytes at once
  int bytes_read = read (self->pipe_fd[0], &buffer, 100);
    
  printf ("ctrl bytes read: %i\n", bytes_read);
    
  if (bytes_read > 0)
  {
    ret = buffer[bytes_read - 1];
  }
  
  return ret;
}

//-----------------------------------------------------------------------------

void pvd2_ctx__set_fds (QBusPvdCtx self, fd_set* pset)
{
  number_t cnt = 0;
  
  cape_mutex_lock (self->mutex);
  
  {
    CapeListCursor* cursor = cape_list_cursor_create (self->fds, CAPE_DIRECTION_FORW);
    
    while (cape_list_cursor_next (cursor))
    {
      QBusPvdFD pfd = cape_list_node_data (cursor->node);

      FD_SET((long)pfd->handle, pset);
      
      cnt++;
    }
    
    cape_list_cursor_destroy (&cursor);
  }
  
  cape_mutex_unlock (self->mutex);
  
  printf ("added fds = %lu\n", cnt);
}

//-----------------------------------------------------------------------------

void pvd2_ctx__get_fds (QBusPvdCtx self, fd_set* pset, int changed_fds)
{
  int selected_fds_left = changed_fds;
  cape_mutex_lock (self->mutex);
  
  {
    CapeListCursor* cursor = cape_list_cursor_create (self->fds, CAPE_DIRECTION_FORW);
    
    while (cape_list_cursor_next (cursor) && (selected_fds_left > 0))
    {
      QBusPvdFD pfd = cape_list_node_data (cursor->node);
      
      if (FD_ISSET ((long)pfd->handle, pset))
      {        
        QBusPvdFD new_pfd = NULL;
        
        selected_fds_left--;

        if (pvd2_entity__event (pfd->entity, &new_pfd))
        {
          if (new_pfd)
          {
            cape_list_push_front (self->fds, new_pfd);
          }
        }
        else
        {
          printf ("connection was dropped\n");
          
          cape_list_cursor_erase (self->fds, cursor);
        }
      }
    }
    
    cape_list_cursor_destroy (&cursor);
  }
  
  cape_mutex_unlock (self->mutex);
}

//-----------------------------------------------------------------------------

int __STDCALL pvd2_ctx__main_worker (void* ptr)
{
  QBusPvdCtx self = ptr;
  
  cape_thread_nosignals ();
  
  fd_set rfds;

  FD_ZERO(&rfds);
  FD_SET(self->pipe_fd[0], &rfds);
  
  pvd2_ctx__set_fds (self, &rfds);
    
  int ret = TRUE;
  int changed_fds = select (FD_SETSIZE, &rfds, NULL, NULL, NULL);
  
  switch (changed_fds)
  {
    case -1:  // error
    {
      
      
      break;
    }
    case 0:   // timeout (no fd was selected)
    {
      
      break;
    }
    default:
    {
      // check the control 'channel'
      if (FD_ISSET (self->pipe_fd[0], &rfds))
      {
        ret = (pvd2_ctx__retrieve_last_ctrl (self) == THREAD_SIGNAL_MSG__UPDATE);
        changed_fds--;
      }

      if (changed_fds > 0)
      {
        pvd2_ctx__get_fds (self, &rfds, changed_fds);
      }
      
      break;
    }
  }
  
  return ret;
}

//-----------------------------------------------------------------------------

void __STDCALL pvd2_ctx__fds__on_del (void* ptr)
{
  QBusPvdFD pfd = ptr;
  
  pvd2_entity__on_done (&(pfd->entity));
  
  CAPE_DEL(&pfd, struct QBusPvdFD_s);
}

//-----------------------------------------------------------------------------

QBusPvdCtx __STDCALL pvd2_ctx_new (CapeAioContext aio_context, CapeErr err)
{
  QBusPvdCtx self = CAPE_NEW (struct QBusPvdCtx_s);
  
  if (pipe (self->pipe_fd) == -1)
  {
    // something went wrong
    // -> cleanup and return error
    
    pvd2_ctx_del (&self);    
    cape_err_lastOSError(err);

    goto exit_and_cleanup;
  }
  
  self->mutex = cape_mutex_new ();
  
  self->fds = cape_list_new (pvd2_ctx__fds__on_del);

  self->worker = cape_thread_new ();
  
  // start the main background worker thread
  cape_thread_start (self->worker, pvd2_ctx__main_worker, self);
  
exit_and_cleanup:
  
  return self;
}

//-----------------------------------------------------------------------------

void pvd2_ctx__signal_thread (QBusPvdCtx self, char signal_msg)
{
  if (write (self->pipe_fd[1], &signal_msg, 1) < 0)
  {
    CapeErr err = cape_err_new ();
    
    cape_err_lastOSError (err);
    cape_log_msg (CAPE_LL_ERROR, "QBUS TCP", "signal thread", cape_err_text(err));
    
    cape_err_del (&err);
  }
}

//-----------------------------------------------------------------------------

void __STDCALL pvd2_ctx_del (QBusPvdCtx* p_self)
{
  if (*p_self)
  {
    QBusPvdCtx self = *p_self;

    // tell the thread to terminate
    pvd2_ctx__signal_thread (self, THREAD_SIGNAL_MSG__TERMINATE);
    
    // wait until the thread terminates
    cape_thread_join (self->worker);
    
    cape_thread_del (&(self->worker));
    
    // close all entities
    cape_list_del (&(self->fds));
    
    // cleanup mutex
    cape_mutex_del (&(self->mutex));
    
    // close the pipe
    close (self->pipe_fd[0]);
    close (self->pipe_fd[1]);
    
    CAPE_DEL(p_self, struct QBusPvdCtx_s);
  }
}

//-----------------------------------------------------------------------------

void pvd2_ctx__register_entity (QBusPvdCtx self, QBusPvdEntity entity, void* handle)
{
  QBusPvdFD pfd = CAPE_NEW (struct QBusPvdFD_s);
  
  pfd->entity = entity;
  pfd->handle = handle;

  cape_mutex_lock (self->mutex);
  
  cape_list_push_back (self->fds, (void*)pfd);
  
  cape_mutex_unlock (self->mutex);
  
  pvd2_ctx__signal_thread (self, THREAD_SIGNAL_MSG__UPDATE);
}

//-----------------------------------------------------------------------------

void pvd2_ctx__unregister_entity (QBusPvdCtx self, QBusPvdEntity entity)
{
  
}

//-----------------------------------------------------------------------------

struct QBusPvdEntity_s
{
  QBusPvdCtx ctx;
  
  void* socket_fd;     // accept socket
  
  CapeString host;
  number_t port;
  
  number_t mode;
};

//-----------------------------------------------------------------------------

QBusPvdEntity __STDCALL pvd2_entity_new (QBusPvdCtx ctx, CapeUdc config, CapeErr err)
{
  QBusPvdEntity self = NULL;
  
  {
    CapeString h = cape_json_to_s (config);
    
    cape_log_fmt (CAPE_LL_TRACE, "QBUS", "new TCP context", "params: %s", h);
    
    cape_str_del (&h);
  }

  switch (cape_udc_get_n (config, "mode", 0))
  {
    case QBUS_PVD_MODE_CONNECT:
    {
      self = CAPE_NEW (struct QBusPvdEntity_s);
      
      self->ctx = ctx;

      self->host = cape_str_cp (cape_udc_get_s (config, "host", ""));
      self->port = cape_udc_get_n (config, "port", 33350);
      
      self->socket_fd = NULL;
      
      self->mode = QBUS_PVD_MODE_CONNECT;
      
      pvd2_entity__connect (self);
      
      break;
    }
    case QBUS_PVD_MODE_LISTEN:
    {
      self = CAPE_NEW (struct QBusPvdEntity_s);

      self->ctx = ctx;
      
      self->socket_fd = cape_sock__tcp__srv_new (cape_udc_get_s (config, "host", ""), cape_udc_get_n (config, "port", 33350), err);
      if (self->socket_fd == 0)
      {
        pvd2_entity_del (&self);
        break;
      }
      
      self->mode = QBUS_PVD_MODE_LISTEN;
      
      pvd2_ctx__register_entity (self->ctx, self, self->socket_fd);
      
      break;
    }
    default:
    {
      cape_err_set (err, CAPE_ERR_NOT_SUPPORTED, "mode not supported");
      break;
    }
  }
  
  return self;
}

//-----------------------------------------------------------------------------

void __STDCALL pvd2_entity_del (QBusPvdEntity* p_self)
{
  if (*p_self)
  {
    QBusPvdEntity self = *p_self;
    
    pvd2_ctx__unregister_entity (self->ctx, self);
    
    cape_sock__close ((void*)self->socket_fd);

    cape_str_del (&(self->host));
    
    CAPE_DEL (p_self, struct QBusPvdEntity_s);
  }
}

//-----------------------------------------------------------------------------

void __STDCALL pvd2_entity_cb (QBusPvdEntity self)
{
  
}

//-----------------------------------------------------------------------------

int pvd2_entity__event (QBusPvdEntity self, QBusPvdFD* p_new_pfd)
{
  switch (self->mode)
  {
    case QBUS_PVD_MODE_CONNECT: // read
    {
      int so_err; socklen_t size = sizeof(int);
      
      // check for errors on the socket, eg connection was refused
      getsockopt((long)self->socket_fd, SOL_SOCKET, SO_ERROR, &so_err, &size);
      
      if (so_err)
      {
        CapeErr err = cape_err_new ();
        
        cape_err_formatErrorOS (err, so_err);
        
        cape_log_fmt (CAPE_LL_ERROR, "CAPE", "socket on_event", "error on socket: %s", cape_err_text(err));
        
        cape_err_del (&err);
      }
      else
      {
        
        
        
      }
      
      char buffer[1024];
      
      ssize_t readBytes = recv ((long)self->socket_fd, buffer, 1024, 0);
      
      printf ("read bytes: %lu\n", readBytes);
      
      if (readBytes < 0)
      {
        if( (errno != EWOULDBLOCK) && (errno != EINPROGRESS) && (errno != EAGAIN))
        {
          cape_log_fmt (CAPE_LL_ERROR, "CAPE", "socket read", "error while writing data to the socket [%i]", errno);
          
        }
        
        return TRUE;
      }
      else if (readBytes == 0)
      {
        // disconnect
        return FALSE;
      }
      else
      {
        

        
      }
      
      break; 
    }
    case QBUS_PVD_MODE_LISTEN: // accept
    {
      struct sockaddr addr;
      socklen_t addrlen = 0;
      
      memset (&addr, 0x00, sizeof(addr));
      
      number_t sock = accept ((long)(self->socket_fd), &addr, &addrlen);
      if (sock < 0)
      {
        if( (errno != EWOULDBLOCK) && (errno != EINPROGRESS) && (errno != EAGAIN))
        {
          return TRUE;
        }
        else
        {
          return FALSE;
        }
      }
      
      {
        QBusPvdFD pfd = CAPE_NEW (struct QBusPvdFD_s);
        
        pfd->entity = pvd2_entity__on_connect (self->ctx, (void*)sock, inet_ntoa(((struct sockaddr_in*)&addr)->sin_addr));
        pfd->handle = (void*)sock;
        
        *p_new_pfd = pfd;
      }
      
      // set the socket to none blocking
      /*
      {
        int flags = fcntl(sock, F_GETFL, 0);
        if (flags == -1)
        {
          
        }
        
        flags |= O_NONBLOCK;
        
        if (fcntl(sock, F_SETFL, flags) != 0)
        {
          
        }
      }
      */
      
      break;
    }
  }  
  
  return TRUE;
}

//-----------------------------------------------------------------------------

QBusPvdEntity pvd2_entity__on_connect (QBusPvdCtx ctx, void* handle, const CapeString host)
{
  QBusPvdEntity entity = CAPE_NEW (struct QBusPvdEntity_s);
  
  entity->ctx = ctx;
  
  entity->host = cape_str_cp (host);
  entity->port = 0;
  
  entity->socket_fd = handle;
  
  entity->mode = QBUS_PVD_MODE_CONNECT;
  
  return entity;
}

//-----------------------------------------------------------------------------

void pvd2_entity__connect (QBusPvdEntity self)
{
  CapeErr err = cape_err_new ();
  
  self->socket_fd = cape_sock__tcp__clt_new (self->host, self->port, err);
  if (self->socket_fd == NULL)
  {
    
  }
  else
  {
    pvd2_ctx__register_entity (self->ctx, self, self->socket_fd);
  }
  
  cape_err_del (&err);
}

//-----------------------------------------------------------------------------

void pvd2_entity__on_done (QBusPvdEntity* p_self)
{
  cape_log_msg (CAPE_LL_DEBUG, "QBUS", "disconnect", "connection lost");
  
  pvd2_entity_del (p_self);
}

//-----------------------------------------------------------------------------
