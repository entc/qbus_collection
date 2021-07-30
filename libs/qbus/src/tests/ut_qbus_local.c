#include <aio/cape_aio_sock.h>
#include <sys/cape_log.h>
#include <sys/cape_socket.h>
#include <sys/cape_time.h>

//-----------------------------------------------------------------------------

void __STDCALL cape_aio_socket__on_pong (void* ptr, CapeAioSocketIcmp self, number_t ms_second, int timeout)
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
    CapeAioSocketIcmp aio_handle;

    void* icmp_handle = cape_sock__icmp__new (err);    
    if (icmp_handle == NULL)
    {
      cape_log_msg (CAPE_LL_ERROR, "TEST", "aio sock", cape_err_text (err));
      return 1;      
    }
    
    aio_handle = cape_aio_socket__icmp__new (icmp_handle); 
    
    cape_aio_socket__icmp__cb (aio_handle, aio, cape_aio_socket__on_pong, NULL);   
    
    cape_aio_socket__icmp__ping (aio_handle, aio, "127.0.0.1", 1000);
    
    cape_aio_socket__icmp__add (&aio_handle, aio);
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

