#include "qbus_tcp.h"
#include "qbus_frame.h"

// cape includes
#include <sys/cape_log.h>
#include <sys/cape_socket.h>
#include <sys/cape_thread.h>
#include <sys/cape_mutex.h>
#include <fmt/cape_json.h>
#include <sys/cape_queue.h>

#include <signal.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>

// includes specific event subsystem
#if defined __BSD_OS

#define CAPE_NO_SIGNALS 0

#elif defined __LINUX_OS

#define CAPE_NO_SIGNALS MSG_NOSIGNAL

#endif

//-----------------------------------------------------------------------------

#define THREAD_SIGNAL_MSG__TERMINATE    'T'
#define THREAD_SIGNAL_MSG__UPDATE       'U'

//-----------------------------------------------------------------------------

struct QBusPvdCtx_s
{
  int pipe_fd[2];
  
  CapeThread worker;

  CapeMutex mutex;
  
  CapeList readfds;
  CapeList writefds;
  
  CapeList connect_queue;
  
  fct_qbus_pvd__fcts_new fcts_new;
  fct_qbus_pvd__fcts_del fcts_del;

  void* factory_ptr;
};

//-----------------------------------------------------------------------------

struct QBusPvdFD_s
{
  QBusPvdEntity entity;
  void* handle;
  
  int handle_type;
  
  // keep the last frame for decoding
  QBusFrame last_frame;
  
}; typedef struct QBusPvdFD_s* QBusPvdFD;

//-----------------------------------------------------------------------------

// forwarded functions
void pvd2_entity__on_done (QBusPvdEntity*, QBusPvdFD, int allow_reconnect);
void* pvd2_entity__reconnect (QBusPvdEntity);

void pvd2_entity__cb_on_connect (QBusPvdEntity, QBusPvdFD);
void pvd2_entity__cb_on_disconnect (QBusPvdEntity, int allow_reconnect);

void pvd2_entity__cb_on_frame (QBusPvdEntity, QBusFrame* p_frame);

//-----------------------------------------------------------------------------

char pvd2_ctx__retrieve_last_ctrl (QBusPvdCtx self)
{
  char ret = THREAD_SIGNAL_MSG__UPDATE;
  char buffer[100];
  
  // reads a maximum of 100 control bytes at once
  int bytes_read = read (self->pipe_fd[0], &buffer, 100);
    
  if (bytes_read > 0)
  {
    ret = buffer[bytes_read - 1];
  }
  
  return ret;
}

//-----------------------------------------------------------------------------

void pvd2_ctx__set_fds (QBusPvdCtx self, fd_set* readfds, fd_set* writefds)
{
  cape_mutex_lock (self->mutex);
  
  {
    CapeListCursor* cursor = cape_list_cursor_create (self->readfds, CAPE_DIRECTION_FORW);
    
    while (cape_list_cursor_next (cursor))
    {
      QBusPvdFD pfd = cape_list_node_data (cursor->node);

      FD_SET((long)pfd->handle, readfds);
    }
    
    cape_list_cursor_destroy (&cursor);
  }

  {
    CapeListCursor* cursor = cape_list_cursor_create (self->writefds, CAPE_DIRECTION_FORW);
    
    while (cape_list_cursor_next (cursor))
    {
      QBusPvdFD pfd = cape_list_node_data (cursor->node);
      
      FD_SET((long)pfd->handle, writefds);
    }
    
    cape_list_cursor_destroy (&cursor);
  }
  
  cape_mutex_unlock (self->mutex);
}

//-----------------------------------------------------------------------------

QBusPvdFD pvd2_pfd_new (void* handle, int handle_type, QBusPvdEntity entity)
{
  QBusPvdFD self = CAPE_NEW (struct QBusPvdFD_s);
  
  self->handle = handle;
  self->handle_type = handle_type;
  self->entity = entity;
  
  self->last_frame = qbus_frame_new ();
  
  return self;
}

//-----------------------------------------------------------------------------

void pvd2_pfd_del (QBusPvdFD* p_self)
{
  if (*p_self)
  {
    QBusPvdFD self = *p_self;
    
    qbus_frame_del (&(self->last_frame));
    
    pvd2_entity__on_done (&(self->entity), self, self->handle_type > QBUS_PVD_MODE_DISABLED);
    
    if (self->handle)
    {
      cape_sock__close (self->handle);
    }
    
    CAPE_DEL(p_self, struct QBusPvdFD_s);
  }
}

//-----------------------------------------------------------------------------

void pvd2_pfd__send (QBusPvdFD self, CapeStream s)
{
  number_t bytes_sent = 0;

  //printf ("try to send = %lu\n", cape_stream_size (s));
  
  while (bytes_sent < cape_stream_size (s))
  {
    ssize_t res_sent = send ((long)self->handle, cape_stream_data (s) + bytes_sent, cape_stream_size (s) - bytes_sent, CAPE_NO_SIGNALS);
    
    if (res_sent < 0)
    {
      if (errno == EWOULDBLOCK)
      {
        // TODO: add to writefds
      }
      else if((errno != EINPROGRESS) && (errno != EAGAIN))
      {
        CapeErr err = cape_err_new ();
        
        cape_err_formatErrorOS (err, errno);
   
        cape_log_fmt (CAPE_LL_ERROR, "CAPE", "socket write", "error while writing data to the socket: %s", cape_err_text(err));
        
        cape_err_del (&err);
        
        break;
      }
      else
      {
        sleep (0);
      }
    }
    else if (res_sent == 0)
    {
      cape_log_msg (CAPE_LL_ERROR, "CAPE", "socket write", "error while writing data to the socket: lost connection");
      
      break;
    }
    else
    {
      bytes_sent += res_sent;
    }
  }
}

//-----------------------------------------------------------------------------

int pvd2_pfd__accept (QBusPvdFD self, QBusPvdCtx ctx, QBusPvdFD* p_new_pfd)
{
  int ret = TRUE;  
  long sock;
  
  struct sockaddr addr;
  socklen_t addrlen = sizeof(addr);

  // local objects
  CapeErr err = cape_err_new ();
  
  memset (&addr, 0x00, sizeof(addr));
   
  sock = accept ((long)(self->handle), &addr, &addrlen);
  if (sock < 0)
  {
    if( (errno != EWOULDBLOCK) && (errno != EINPROGRESS) && (errno != EAGAIN))
    {
      goto exit_and_cleanup;
    }
    else
    {
      cape_err_lastOSError (err);
      
      ret = FALSE;
      goto exit_and_cleanup;
    }
  }
 
  if (cape_sock__noneblocking ((void*)sock, err))
  {
    goto exit_and_cleanup;
  }
 
  if (ctx->fcts_new)
  {
    // cast to socket 
    struct sockaddr_in* socket_in = (struct sockaddr_in*)&addr;

    // allocate a new entity
    QBusPvdEntity entity = pvd2_entity_new (ctx, FALSE, inet_ntoa(socket_in->sin_addr), htons (socket_in->sin_port));

    // allocate a new connection object
    *p_new_pfd = pvd2_pfd_new ((void*)sock, QBUS_PVD_MODE_REMOTE, entity);
  }
  
exit_and_cleanup:

  if (cape_err_code (err))
  {
    cape_log_msg (CAPE_LL_ERROR, "QBUS", "accept", cape_err_text (err));    
  }

  cape_err_del (&err);
  return ret;
}

//-----------------------------------------------------------------------------

int pvd2_pfd_event (QBusPvdFD self, QBusPvdCtx ctx, QBusPvdFD* p_new_pfd)
{
  switch (self->handle_type)
  {
    case QBUS_PVD_MODE_CONNECT:
    case QBUS_PVD_MODE_REMOTE:
    case QBUS_PVD_MODE_CLIENT: // read
    {      
      char buffer[1024];
      
      ssize_t readBytes = recv ((long)self->handle, buffer, 1024, 0);
      
      //printf ("read bytes: %zd\n", readBytes);
      
      if (readBytes < 0)
      {
        if ((errno == ECONNRESET) || (errno == ECONNREFUSED))
        {
          // reset by peer
          return FALSE;
        }
        else if( (errno != EWOULDBLOCK) && (errno != EINPROGRESS) && (errno != EAGAIN))
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
        number_t total_written = 0;

        // set callbacks
        pvd2_entity__cb_on_connect (self->entity, self);
        
        while (total_written < readBytes)
        {
          number_t written = 0;
          
          if (qbus_frame_decode (self->last_frame, buffer + total_written, readBytes - total_written, &written))
          {
            pvd2_entity__cb_on_frame (self->entity, &(self->last_frame));
            
            // to be on the safe side
            qbus_frame_del (&(self->last_frame));
            
            self->last_frame = qbus_frame_new ();
          }
          
          total_written += written;
        }
        
      }
      
      break; 
    }
    case QBUS_PVD_MODE_LISTEN: // accept
    {
      return pvd2_pfd__accept (self, ctx, p_new_pfd);
    }
  }  
  
  return TRUE;
}

//-----------------------------------------------------------------------------

void pvd2_ctx__get_fds (QBusPvdCtx self, fd_set* pset, int changed_fds)
{
  int selected_fds_left = changed_fds;
  cape_mutex_lock (self->mutex);
  
  {
    CapeListCursor* cursor = cape_list_cursor_create (self->readfds, CAPE_DIRECTION_FORW);
    
    while (cape_list_cursor_next (cursor) && (selected_fds_left > 0))
    {
      QBusPvdFD pfd = cape_list_node_data (cursor->node);
      
      if (FD_ISSET ((long)pfd->handle, pset))
      {        
        QBusPvdFD new_pfd = NULL;
        
        selected_fds_left--;

        if (pvd2_pfd_event (pfd, self, &new_pfd))
        {
          if (new_pfd)
          {
            cape_list_push_front (self->readfds, new_pfd);
            
            // notify of a new connection
            pvd2_entity__cb_on_connect (new_pfd->entity, new_pfd);
          }
        }
        else
        {
          cape_list_cursor_erase (self->readfds, cursor);
        }
      }
    }
    
    cape_list_cursor_destroy (&cursor);
  }
  
  cape_mutex_unlock (self->mutex);
}

//-----------------------------------------------------------------------------

void pvd2_ctx__disable_fds (QBusPvdCtx self)
{
  cape_mutex_lock (self->mutex);
  
  {
    CapeListCursor* cursor = cape_list_cursor_create (self->readfds, CAPE_DIRECTION_FORW);
    
    while (cape_list_cursor_next (cursor))
    {
      QBusPvdFD pfd = cape_list_node_data (cursor->node);
      
      pfd->handle_type = QBUS_PVD_MODE_DISABLED;
    }

    cape_list_cursor_destroy (&cursor);
  }

  cape_mutex_unlock (self->mutex);
}

//-----------------------------------------------------------------------------

void pvd2_ctx__check_connects (QBusPvdCtx self)
{
  CapeListCursor* cursor = cape_list_cursor_create (self->connect_queue, CAPE_DIRECTION_FORW);
  
  while (cape_list_cursor_next (cursor))
  {
    QBusPvdEntity entity = cape_list_node_data (cursor->node);
    
    void* handle = pvd2_entity__reconnect (entity);

    if (handle)
    {
      QBusPvdFD pfd = pvd2_pfd_new (handle, QBUS_PVD_MODE_CONNECT, entity);
      
      cape_list_push_front (self->readfds, (void*)pfd);
      
      pvd2_ctx__signal_thread (self, THREAD_SIGNAL_MSG__UPDATE);
            
      // remove from connection queue
      cape_list_cursor_extract (self->connect_queue, cursor);
    }
  }
  
  cape_list_cursor_destroy (&cursor);
}

//-----------------------------------------------------------------------------

int __STDCALL pvd2_ctx__main_worker (void* ptr)
{
  QBusPvdCtx self = ptr;
  
  cape_thread_nosignals ();
  
  fd_set readfds;
  fd_set writefds;
  
  struct timeval timeout;

  FD_ZERO(&readfds);
  FD_ZERO(&writefds);

  FD_SET(self->pipe_fd[0], &readfds);
  
  pvd2_ctx__set_fds (self, &readfds, &writefds);
  
  timeout.tv_usec = 0;
  timeout.tv_sec = 10;
  
  int ret = TRUE;
  int changed_fds = select (FD_SETSIZE, &readfds, &writefds, NULL, &timeout);
  
  //printf ("EVENT: %i\n", changed_fds);
  
  switch (changed_fds)
  {
    case -1:  // error
    {
      
      
      break;
    }
    case 0:   // timeout (no fd was selected)
    {
      pvd2_ctx__check_connects (self);

      break;
    }
    default:
    {
      // check the control 'channel'
      if (FD_ISSET (self->pipe_fd[0], &readfds))
      {
        ret = (pvd2_ctx__retrieve_last_ctrl (self) == THREAD_SIGNAL_MSG__UPDATE);
        changed_fds--;
      }

      if (changed_fds > 0)
      {
        pvd2_ctx__get_fds (self, &readfds, changed_fds);
      }
      
      break;
    }
  }
  
  return ret;
}

//-----------------------------------------------------------------------------

void __STDCALL pvd2_ctx__fds__on_del (void* ptr)
{
  QBusPvdFD pfd = ptr; pvd2_pfd_del (&pfd);
}

//-----------------------------------------------------------------------------

void __STDCALL pvd2_ctx__entities__on_del (void* ptr)
{
  QBusPvdEntity entity = ptr; pvd2_entity_del (&entity);
}

//-----------------------------------------------------------------------------

QBusPvdCtx __STDCALL pvd2_ctx_new (CapeAioContext aio_context, fct_qbus_pvd__fcts_new fcts_new, fct_qbus_pvd__fcts_del fcts_del, void* factory_ptr, CapeErr err)
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

  self->fcts_new = fcts_new;
  self->fcts_del = fcts_del;
  self->factory_ptr = factory_ptr;
  
  self->mutex = cape_mutex_new ();
  
  self->readfds = cape_list_new (pvd2_ctx__fds__on_del);
  self->writefds = cape_list_new (pvd2_ctx__fds__on_del);
  
  self->connect_queue = cape_list_new (pvd2_ctx__entities__on_del);
  
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

    // disable the fds before deletion
    // -> avoid reconnection mechanics
    pvd2_ctx__disable_fds (self);
    
    // tell the thread to terminate
    pvd2_ctx__signal_thread (self, THREAD_SIGNAL_MSG__TERMINATE);
    
    // wait until the thread terminates
    cape_thread_join (self->worker);
    
    cape_thread_del (&(self->worker));

    // close all fds
    cape_list_del (&(self->readfds));

    // close all fds
    cape_list_del (&(self->writefds));
    
    // close all entities
    cape_list_del (&(self->connect_queue));

    // cleanup mutex
    cape_mutex_del (&(self->mutex));
    
    // close the pipe
    close (self->pipe_fd[0]);
    close (self->pipe_fd[1]);
    
    CAPE_DEL(p_self, struct QBusPvdCtx_s);
  }
}

//-----------------------------------------------------------------------------

void pvd2_ctx_send (QBusPvdCtx self, void* pfd_ptr, QBusFrame frame)
{
  CapeStream s = cape_stream_new ();
  
  qbus_frame_encode (frame, s);

  pvd2_pfd__send (pfd_ptr, s);
  
  // cleanup
  cape_stream_del (&s);
}

//-----------------------------------------------------------------------------

void pvd2_ctx__register_handle (QBusPvdCtx self, QBusPvdEntity entity, void* handle, int handle_type)
{
  QBusPvdFD pfd = pvd2_pfd_new (handle, handle_type, entity);
  
  cape_mutex_lock (self->mutex);
  
  cape_list_push_front (self->readfds, (void*)pfd);
  
  cape_mutex_unlock (self->mutex);
  
  pvd2_ctx__signal_thread (self, THREAD_SIGNAL_MSG__UPDATE);
}

//-----------------------------------------------------------------------------

void pvd2_ctx__register_connect (QBusPvdCtx self, QBusPvdEntity entity)
{
  cape_list_push_front (self->connect_queue, (void*)entity);
}

//-----------------------------------------------------------------------------

struct QBusPvdEntity_s
{
  QBusPvdCtx ctx;
  
  CapeString host;
  number_t port;
  
  int reconnect;
  
  QBusPvdFcts fcts;
};

//-----------------------------------------------------------------------------

QBusPvdEntity pvd2_entity_new (QBusPvdCtx ctx, int reconnect, const CapeString host, number_t port)
{
  QBusPvdEntity self = CAPE_NEW (struct QBusPvdEntity_s);
  
  self->ctx = ctx;
  self->reconnect = reconnect;
  
  self->host = cape_str_cp (host);
  self->port = port;
  
  self->fcts = NULL;
  
  return self;
}

//-----------------------------------------------------------------------------

void pvd2_entity_del (QBusPvdEntity* p_self)
{
  if (*p_self)
  {
    QBusPvdEntity self = *p_self;
    
    cape_str_del (&(self->host));
        
    CAPE_DEL (p_self, struct QBusPvdEntity_s);
  }
}

//-----------------------------------------------------------------------------

void pvd2_entity_fcts_set (QBusPvdEntity self, QBusPvdFcts* p_fcts)
{
  self->fcts = *p_fcts;
  *p_fcts = NULL;
}

//-----------------------------------------------------------------------------

void* pvd2_entity__bind (QBusPvdEntity self)
{
  CapeErr err = cape_err_new ();
  
  void* handle = cape_sock__tcp__srv_new (self->host, self->port, err);
  
  if (handle)
  {
    // always set noneblocking
    if (cape_sock__noneblocking (handle, err))
    {
      cape_sock__close (handle);
      handle = NULL;
    }
  }
  
  if (cape_err_code (err))
  {
    cape_log_msg (CAPE_LL_ERROR, "QBUS", "bind", cape_err_text (err));    
  }
  
  cape_err_del (&err);
  
  return handle;
}

//-----------------------------------------------------------------------------
// notify of a new connection

void pvd2_entity__listen (QBusPvdEntity* p_self)
{
  QBusPvdEntity self = *p_self;
  
  // try to bind to host
  {
    void* handle = pvd2_entity__bind (self);
    
    if (handle == NULL)
    {
      // remove this entity
      pvd2_entity_del (p_self);
    }
    else
    {
      pvd2_ctx__register_handle (self->ctx, self, handle, QBUS_PVD_MODE_LISTEN);
    }
  }
}

//-----------------------------------------------------------------------------

void* pvd2_entity__reconnect (QBusPvdEntity self)
{
  CapeErr err = cape_err_new ();
  
  void* handle = cape_sock__tcp__clt_new (self->host, self->port, err);
  
  if (handle)
  {
    // always set noneblocking
    if (cape_sock__noneblocking (handle, err))
    {
      cape_sock__close (handle);
      handle = NULL;
    }
  }
    
  if (cape_err_code (err))
  {
    cape_log_msg (CAPE_LL_ERROR, "QBUS", "connect", cape_err_text (err));    
  }
  
  cape_err_del (&err);
  
  return handle;
}

//-----------------------------------------------------------------------------

void pvd2_entity__connect (QBusPvdEntity* p_self)
{
  QBusPvdEntity self = *p_self;

  // try to connect to host
  {
    void* handle = pvd2_entity__reconnect (self);
    
    if (handle == NULL)
    {
      pvd2_ctx__register_connect (self->ctx, self);
      *p_self = NULL;
    }
    else
    {
      pvd2_ctx__register_handle (self->ctx, self, handle, QBUS_PVD_MODE_CONNECT);
    }
  }
}

//-----------------------------------------------------------------------------

void pvd2_entity__cb_on_connect (QBusPvdEntity self, QBusPvdFD pfd)
{  
  if (self->fcts == NULL)
  {
    QBusPvdFcts fcts = self->ctx->fcts_new (self->ctx->factory_ptr, pfd);
    
    pvd2_entity_fcts_set (self, &fcts);

    cape_log_fmt (CAPE_LL_TRACE, "QBUS", "entity", "new connection from %s:%lu", self->host, self->port);
  }
}

//-----------------------------------------------------------------------------

void pvd2_entity__cb_on_disconnect (QBusPvdEntity self, int allow_reconnect)
{  
  if (self->fcts)
  {
    cape_log_fmt (CAPE_LL_TRACE, "QBUS", "entity", "lost connection from %s:%lu", self->host, self->port);

    if (self->ctx->fcts_del)
    {
      self->ctx->fcts_del (self->ctx->factory_ptr, allow_reconnect, &(self->fcts));
    }
  }
  else
  {
    cape_log_fmt (CAPE_LL_TRACE, "QBUS", "entity", "can't connect to %s:%lu", self->host, self->port);
  }
}

//-----------------------------------------------------------------------------

void pvd2_entity__cb_on_frame (QBusPvdEntity self, QBusFrame* p_frame)
{
  if (self->fcts->on_frame)
  {
    self->fcts->on_frame (self->ctx->factory_ptr, self->fcts->conn, p_frame);
  }
}

//-----------------------------------------------------------------------------

void __STDCALL pvd2_ctx_reg (QBusPvdCtx self, CapeUdc config)
{
  {
    CapeString h = cape_json_to_s (config);
    
    cape_log_fmt (CAPE_LL_TRACE, "QBUS", "new TCP context", "params: %s", h);
    
    cape_str_del (&h);
  }

  switch (cape_udc_get_n (config, "mode", 0))
  {
    case QBUS_PVD_MODE_CLIENT:
    {
      if (self->fcts_new)
      {
        QBusPvdEntity entity = pvd2_entity_new (self, TRUE, cape_udc_get_s (config, "host", ""), cape_udc_get_n (config, "port", 33350));
                
        pvd2_entity__connect (&entity);
      }

      break;
    }
    case QBUS_PVD_MODE_LISTEN:
    {
      if (self->fcts_new)
      {        
        QBusPvdEntity entity = pvd2_entity_new (self, FALSE, cape_udc_get_s (config, "host", ""), cape_udc_get_n (config, "port", 33350));
        
        pvd2_entity__listen (&entity);      
      }

      break;
    }
    default:
    {
      cape_log_msg (CAPE_LL_ERROR, "QBUS", "register", "register mode invalid");
      break;
    }
  }
}

//-----------------------------------------------------------------------------

void pvd2_entity__on_done (QBusPvdEntity* p_self, QBusPvdFD pfd, int allow_reconnect)
{
  if (*p_self)
  {
    QBusPvdEntity self = *p_self;
    
    pvd2_entity__cb_on_disconnect (self, allow_reconnect);
        
    if (allow_reconnect && self->reconnect)
    {
      pvd2_ctx__register_connect (self->ctx, self);
    }
    else
    {
      cape_log_msg (CAPE_LL_TRACE, "QBUS", "disconnect", "remove entity");
      
      pvd2_entity_del (p_self);
    }  
  }
}

//-----------------------------------------------------------------------------
