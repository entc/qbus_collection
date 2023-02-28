#include "qbus_tcp.h"

// cape includes
#include <sys/cape_log.h>
#include <sys/cape_socket.h>
#include <sys/cape_thread.h>
#include <fmt/cape_json.h>

#include <signal.h>
#include <unistd.h>

//-----------------------------------------------------------------------------

struct QBusPvdCtx_s
{
  void* socket_fd;     // accept socket
  
  CapeThread worker;
};

//-----------------------------------------------------------------------------

int __STDCALL pvd2_ctx__accept_worker (void* ptr)
{
  QBusPvdCtx self = ptr;
  
  sigset_t sigset;
  sigset_t orig_mask;
  int sig;
  
  fd_set rfds;
  
  FD_ZERO(&rfds);
  FD_SET((long)self->socket_fd, &rfds);
  
  //sigfillset (&sigset);
  //sigdelset (&sigset, SIGUSR1);
  
  cape_thread_nosignals ();
  
  sigemptyset(&sigset);
  sigaddset(&sigset, SIGUSR2);
  
  
  sigprocmask(SIG_BLOCK, &sigset, &orig_mask);
  
  int res = pselect (1, &rfds, NULL, NULL, NULL, &orig_mask);
  
  //sigwait (&sigset, &sig);
  
  printf ("SIGNAL !!\n");
  
  return FALSE;
}

//-----------------------------------------------------------------------------

QBusPvdCtx __STDCALL pvd2_ctx_new (CapeUdc config, CapeAioContext aio_context, CapeErr err)
{
  QBusPvdCtx ret = NULL;
  
  {
    CapeString h = cape_json_to_s (config);
    
    cape_log_fmt (CAPE_LL_TRACE, "QBUS", "new TCP context", "params: %s", h);
    
    cape_str_del (&h);
  }

  switch (cape_udc_get_n (config, "mode", 0))
  {
    case QBUS_PVD_MODE_CONNECT:
    {
      ret = CAPE_NEW (struct QBusPvdCtx_s);
      
      
      break;
    }
    case QBUS_PVD_MODE_LISTEN:
    {
      ret = CAPE_NEW (struct QBusPvdCtx_s);

      ret->socket_fd = cape_sock__tcp__srv_new (cape_udc_get_s (config, "host", ""), cape_udc_get_n (config, "port", 33350), err);
      if (ret->socket_fd == 0)
      {
        pvd2_ctx_del (&ret);
        break;
      }
      
      ret->worker = cape_thread_new ();

      cape_thread_start (ret->worker, pvd2_ctx__accept_worker, ret);
      
      break;
    }
    default:
    {
      cape_err_set (err, CAPE_ERR_NOT_SUPPORTED, "mode not supported");
      break;
    }
  }
  
  return ret;
}

//-----------------------------------------------------------------------------

void __STDCALL pvd2_ctx_del (QBusPvdCtx* p_self)
{
  if (*p_self)
  {
    QBusPvdCtx self = *p_self;
    
    printf ("send signal\n");
    
    // send a signal to the thread to break out of select
    cape_thread_signal (self->worker);
    
    // wait until the thread terminates
    cape_thread_join (self->worker);
    
    cape_thread_del (&(self->worker));
    cape_sock__close ((void*)self->socket_fd);
    
    CAPE_DEL (p_self, struct QBusPvdCtx_s);
  }
}

//-----------------------------------------------------------------------------

void __STDCALL pvd2_ctx_cb (QBusPvdCtx self)
{
  
}

//-----------------------------------------------------------------------------
