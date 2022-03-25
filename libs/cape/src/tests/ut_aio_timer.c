#include <aio/cape_aio_timer.h>
#include <sys/cape_log.h>
#include <sys/cape_socket.h>
#include <sys/cape_time.h>

//-----------------------------------------------------------------------------

static int __STDCALL cape_aio_timer__on_event (void* ptr)
{
  cape_log_msg (CAPE_LL_DEBUG, "TEST", "aio timer", "event!");

  return TRUE;
}

//-----------------------------------------------------------------------------

int main (int argc, char *argv[])
{
  int res;
  CapeErr err = cape_err_new ();
  
  // create a new aio context for all events
  CapeAioContext aio = cape_aio_context_new (); 

  // local objects
  CapeAioTimer timer = NULL;
  
  printf("start test\n");

  // start the AIO event handling
  res = cape_aio_context_open (aio, err);
  if (res)
  {
    goto exit_and_cleanup;
  }

  // add standard interupt methods
  res = cape_aio_context_set_interupts (aio, TRUE, TRUE, err);
  if (res)
  {
    goto exit_and_cleanup;
  }
  
  // timer
  timer = cape_aio_timer_new ();

  printf("set timer\n");

  res = cape_aio_timer_set (timer, 5000, NULL, cape_aio_timer__on_event, err);
  if (res)
  {
    goto exit_and_cleanup;
  }

  printf("add timer\n");

  res = cape_aio_timer_add (&timer, aio);
  if (res)
  {
    goto exit_and_cleanup;
  }
  
  printf("wait for events\n");

  res = cape_aio_context_wait (aio, err);

  printf("wait interupted\n");

exit_and_cleanup:
  
  if (cape_err_code (err))
  {
    cape_log_msg (CAPE_LL_ERROR, "TEST", "aio timer", cape_err_text (err));
  }
  
  cape_aio_context_del (&aio);
  cape_err_del (&err);
  
  return res;
}

