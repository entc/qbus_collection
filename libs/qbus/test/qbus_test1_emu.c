#include "qbus.h"

// cape includes
#include <sys/cape_thread.h>
#include <sys/cape_log.h>
#include <fmt/cape_json.h>

//-----------------------------------------------------------------------------

static int __STDCALL test3_method (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  
  cape_log_msg (CAPE_LL_DEBUG, "TEST3", "on method", "function is called");
  
  if (qin->cdata)
  {
    CapeString h = cape_json_to_s (qin->cdata);
    
    printf ("qin:cdata = %s\n", h);
    
    cape_str_del (&h);
  }
  else
  {
    printf ("NO CDATA\n");
  }
  
  qout->cdata = cape_udc_new (CAPE_UDC_NODE, NULL);
  cape_udc_add_n (qout->cdata, "good bye", 42);
  
  res = CAPE_ERR_NONE;
  
  return res;
}

//-----------------------------------------------------------------------------

static int __STDCALL test2_method_response (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  cape_log_msg (CAPE_LL_DEBUG, "TEST2", "on method", "response");

  cape_udc_replace_mv (&(qout->cdata), &(qin->cdata));
  
  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

static int __STDCALL test2_method (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  int res;

  cape_log_msg (CAPE_LL_DEBUG, "TEST2", "on method", "function is called");

  // clean up
  qbus_message_clr (qin, CAPE_UDC_NODE);
  
  cape_udc_add_s_cp (qin->cdata, "test", "hello");
  
  res = qbus_continue (qbus, "TEST3", "test_method", qin, (void**)NULL, test2_method_response, err);
  
  return res;
}

//-----------------------------------------------------------------------------

int __STDCALL th1_worker__on_method (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  cape_log_msg (CAPE_LL_DEBUG, "TEST1", "on method", "on method");

  if (qin->cdata)
  {
    CapeString h = cape_json_to_s (qin->cdata);
    
    printf ("qin:cdata = %s\n", h);
    
    cape_str_del (&h);
  }
  else
  {
    printf ("NO CDATA\n");
  }
  
  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

int __STDCALL th1_worker (void* ptr)
{
  int res;
  QBus qbus = ptr;
  
  number_t runs = 0;

  // local objects
  CapeErr err = cape_err_new ();

  // wait for all modules to be initialized
  cape_thread_sleep (100);
  
  for (runs = 0; runs < 1; runs++)
  {
    printf ("RUNS: %lu\n", runs);
    {
      QBusM msg = qbus_message_new ();
      
      msg->cdata = cape_udc_new (CAPE_UDC_NODE, NULL);
      
      cape_udc_add_s_cp (msg->cdata, "welcome", "hello world");
      
      res = qbus_send (qbus, "TEST2", "test_method", msg, NULL, th1_worker__on_method, err);
      
      qbus_message_del (&msg);
    }
  }

  // wait
  cape_thread_sleep (100);

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
  int res;
  
  CapeErr err = cape_err_new ();
  QBusManifold manifold = qbus_manifold_new ();
  
  QBus qbus01 = qbus_new ("TEST1", manifold);
  QBus qbus02 = qbus_new ("TEST2", manifold);
  QBus qbus03 = qbus_new ("TEST3", manifold);
  
  CapeThread th1 = cape_thread_new ();
  CapeThread th2 = cape_thread_new ();
  
  res = qbus_init (qbus01, 2, err);
  if (res)
  {
    return res;
  }

  res = qbus_init (qbus02, 2, err);
  if (res)
  {
    return res;
  }

  res = qbus_init (qbus03, 2, err);
  if (res)
  {
    return res;
  }

  // register methods
  qbus_register (qbus02, "test_method", NULL, test2_method, NULL, err);
  qbus_register (qbus03, "test_method", NULL, test3_method, NULL, err);
  
  cape_thread_nosignals   ();
  
  {
    CapeUdc h = cape_udc_new (CAPE_UDC_NUMBER, "_load");
    cape_udc_set_n (h, 1);
        
    qbus_emit (qbus01, h);

    cape_udc_del (&h);
  }

  {
    CapeUdc h = cape_udc_new (CAPE_UDC_NUMBER, "_load");
    cape_udc_set_n (h, 2);
        
    qbus_emit (qbus02, h);

    cape_udc_del (&h);
  }

  {
    CapeUdc h = cape_udc_new (CAPE_UDC_NUMBER, "_load");
    cape_udc_set_n (h, 3);
        
    qbus_emit (qbus03, h);

    cape_udc_del (&h);
  }


  cape_thread_start (th1, th1_worker, qbus01);
//  cape_thread_start (th2, th1_worker, qbus01);
  
  cape_thread_join (th1);
//  cape_thread_join (th2);
  
exit_and_cleanup:
  
  cape_thread_del (&th1);
  cape_thread_del (&th2);

  qbus_del (&qbus01);
  qbus_del (&qbus02);
  qbus_del (&qbus03);

  qbus_manifold_del (&manifold);
  
  return res;
}

//-----------------------------------------------------------------------------
