#include "sys/cape_queue.h"
#include "sys/cape_log.h"
#include "sys/cape_thread.h"

//-----------------------------------------------------------------------------

static void __STDCALL cape_queue01_callback (void* ptr, number_t pos, number_t queue_size)
{
  //cape_log_fmt (CAPE_LL_DEBUG, "CAPE", "UT :: queue", "[%i] callback start", h);

  cape_thread_sleep (4000);
  
  cape_log_fmt (CAPE_LL_DEBUG, "CAPE", "UT :: queue", "[%i] callback done", pos);
}

//-----------------------------------------------------------------------------

// c includes
#include <stdio.h>

int main (int argc, char *argv[])
{
  CapeErr err = cape_err_new ();
  
  CapeQueue queue01 = cape_queue_new ();
  
  CapeSync sync01 = cape_sync_new ();
  
  // this creates 10 threads
  cape_queue_start (queue01, 3, err);
  
  // add 10 tasks
  {
    int i;
    
    for (i = 0; i < 10; i++)
    {
      cape_queue_add (queue01, sync01, cape_queue01_callback, NULL, NULL, i);
    }
  }

  // wait until all sync items were deleted
  cape_sync_del (&sync01);
  
  // this shall stop all threads
  cape_queue_del (&queue01);
  
  if (cape_err_code(err))
  {
    cape_log_fmt (CAPE_LL_ERROR, "CAPE", "UT :: queue", "error: %s", cape_err_text(err));
    
    return 1;
  }
  else
  {
    return 0;
  }
}

//-----------------------------------------------------------------------------
