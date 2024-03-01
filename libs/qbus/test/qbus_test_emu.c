#include "qbus.h"

// cape includes
#include <sys/cape_thread.h>

//-----------------------------------------------------------------------------

static int __STDCALL test_method (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  
  res = CAPE_ERR_NONE;
  
  return res;
}

//-----------------------------------------------------------------------------

static int __STDCALL app_on_init (QBus qbus, void* ptr, void** p_ptr, CapeErr err)
{
  // register methods
  qbus_register (qbus, "test_method", NULL, test_method, NULL, err);
  
  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

static int __STDCALL app_on_done (QBus qbus, void* ptr, CapeErr err)
{
  
  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

int __STDCALL th1_worker (void* ptr)
{
  int res;
  
  CapeErr err = cape_err_new ();
  QBus qbus = qbus_new ("TEST1", ptr);

  res = qbus_wait (qbus, NULL, NULL, 2, err);
  
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
