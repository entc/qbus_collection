#include <aio/cape_aio_sock.h>
#include <sys/cape_log.h>
#include <sys/cape_socket.h>
#include <sys/cape_time.h>

//-----------------------------------------------------------------------------

void __STDCALL cape_aio_socket__srv__on_sent_ready (void* ptr, CapeAioSocketUdp self, void* userdata)
{
  
}

//-----------------------------------------------------------------------------

void __STDCALL cape_aio_socket__srv__on_recv_from (void* ptr, CapeAioSocketUdp self, const char* bufdat, number_t buflen, const char* host)
{
  //printf ("GOT MESSAGE: %s from %s\n", bufdat, host);
}

//-----------------------------------------------------------------------------

void __STDCALL cape_aio_socket__clt__on_sent_ready (void* ptr, CapeAioSocketUdp self, void* userdata)
{
  if (userdata)
  { 
    CapeString h = userdata;
    
    cape_str_del (&h);
  }
  
  {
    CapeString h = cape_str_uuid();
    
    // send message
    cape_aio_socket__udp__send (self, ptr, h, cape_str_size (h), h, "127.0.0.1", 43340);
  }
}

//-----------------------------------------------------------------------------

void __STDCALL cape_aio_socket__clt__on_recv_from (void* ptr, CapeAioSocketUdp self, const char* bufdat, number_t buflen, const char* host)
{
  
  
}

//-----------------------------------------------------------------------------

void __STDCALL cape_aio_socket__clt__on_done (void* ptr, void* userdata)
{
  if (userdata)
  { 
    CapeString h = userdata;
    
    cape_str_del (&h);
  }
}

//-----------------------------------------------------------------------------

int main (int argc, char *argv[])
{
  int res;
  
  CapeErr err = cape_err_new ();
  
  // create a new aio context for all events
  CapeAioContext aio = cape_aio_context_new (); 

  // start the AIO event handling
  res = cape_aio_context_open (aio, err);
  if (res)
  {
    cape_log_msg (CAPE_LL_ERROR, "TEST", "aio sock", cape_err_text (err));
    return 1;
  }

  // add standard interupt methods
  res = cape_aio_context_set_interupts (aio, TRUE, TRUE, err);
  if (res)
  {
    cape_log_msg (CAPE_LL_ERROR, "TEST", "aio sock", cape_err_text (err));
    return 1;
  }
  
  // server socket
  
  {
    CapeAioSocketUdp aio_handle;

    void* srv_handle = cape_sock__udp__srv_new ("127.0.0.1", 43340, err);    
    if (srv_handle == NULL)
    {
      cape_log_msg (CAPE_LL_ERROR, "TEST", "aio sock", cape_err_text (err));
      return 1;      
    }
    
    aio_handle = cape_aio_socket__udp__new (srv_handle); 
    
    cape_aio_socket__udp__cb (aio_handle, aio, cape_aio_socket__srv__on_sent_ready, cape_aio_socket__srv__on_recv_from, NULL);   
    
    cape_aio_socket__udp__add (&aio_handle, aio, CAPE_AIO_READ);
  }
  
  // client socket
  
  {
    CapeAioSocketUdp aio_handle;

    void* clt_handle = cape_sock__udp__clt_new ("127.0.0.1", 43340, err);    
    if (clt_handle == NULL)
    {
      cape_log_msg (CAPE_LL_ERROR, "TEST", "aio sock", cape_err_text (err));
      return 1;      
    }
    
    aio_handle = cape_aio_socket__udp__new (clt_handle); 
    
    cape_aio_socket__udp__cb (aio_handle, aio, cape_aio_socket__clt__on_sent_ready, cape_aio_socket__clt__on_recv_from, cape_aio_socket__clt__on_done);   
    
    cape_aio_socket__udp__add (&aio_handle, aio, CAPE_AIO_READ | CAPE_AIO_WRITE);
  }
  
  {
    number_t m = 8000;
    int i = 0;
  
    CapeStopTimer st = cape_stoptimer_new ();
    
    cape_stoptimer_start (st);
    
    // loop processing all events
    for (i = 0; i < m && cape_aio_context_next (aio, -1, err) == CAPE_ERR_NONE; i++)
    {
    }
  
    cape_stoptimer_stop (st);
    
    cape_log_fmt (CAPE_LL_DEBUG, "TEST", "aio sock", "transfered %li messages in %5.3f milliseconds", m, cape_stoptimer_get (st));
    
    cape_stoptimer_del (&st);
  }
  
  cape_aio_context_del (&aio);
  
  cape_err_del (&err);
  
  return 0;
}

