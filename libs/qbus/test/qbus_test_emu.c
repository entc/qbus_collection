#include "qbus.h"

// cape includes
#include <sys/cape_thread.h>
#include <sys/cape_log.h>

//-----------------------------------------------------------------------------

static int __STDCALL test_method (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  
  cape_log_msg (CAPE_LL_DEBUG, "TEST", "on method", "function is called");
  
  res = CAPE_ERR_NONE;
  
  return res;
}

//-----------------------------------------------------------------------------

int __STDCALL th1_worker__on_method (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  
  
  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

int __STDCALL th1_worker (void* ptr)
{
  int res;
  number_t runs = 0;
  
  CapeErr err = cape_err_new ();
  QBus qbus = qbus_new ("TEST1", ptr);

  res = qbus_init (qbus, 2, err);
  if (res)
  {
    return res;
  }

  // activate signal handling strategy
  res = cape_aio_context_set_interupts (qbus_aio (qbus), TRUE, TRUE, err);
  if (res)
  {
    return res;
  }

  // wait for all modules to be initialized
  cape_thread_sleep (300);
  
  for (runs = 0; runs < 100; runs++)
  {
    printf ("RUNS: %lu\n", runs);
    {
      QBusM msg = qbus_message_new (NULL, NULL);
      
      res = qbus_send (qbus, "TEST2", "test_method", msg, NULL, th1_worker__on_method, err);
      
      qbus_message_del (&msg);
    }
    
    if (res)
    {
      cape_log_fmt (CAPE_LL_ERROR, "TEST", "qbus send", "returned error: %s", cape_err_text (err));
      break;
    }
    
    if (qbus_next (qbus, err))
    {
      break;
    }
  }
  
  qbus_del (&qbus);
  cape_err_del (&err);
  
  return FALSE;
}

//-----------------------------------------------------------------------------

int __STDCALL th2_worker (void* ptr)
{
  int res;
  
  CapeErr err = cape_err_new ();
  QBus qbus = qbus_new ("TEST2", ptr);
  
  // register methods
  qbus_register (qbus, "test_method", NULL, test_method, NULL, err);

  res = qbus_wait (qbus, NULL, NULL, 2, err);
  
  qbus_del (&qbus);
  cape_err_del (&err);
  
  return FALSE;
}

//-----------------------------------------------------------------------------

int main (int argc, char *argv[])
{
  QBusManifold manifold = qbus_manifold_new ();
  
  CapeThread th1 = cape_thread_new ();
  CapeThread th2 = cape_thread_new ();
  
  cape_thread_nosignals   ();
  
  cape_thread_start (th1, th1_worker, manifold);
  cape_thread_start (th2, th2_worker, manifold);
  
  cape_thread_join (th1);
  cape_thread_join (th2);
  
  cape_thread_del (&th1);
  cape_thread_del (&th2);

  qbus_manifold_del (&manifold);
  
  return 0;
}

//-----------------------------------------------------------------------------
