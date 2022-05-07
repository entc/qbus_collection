#include <qbus.h>

// cape includes
#include <sys/cape_log.h>
#include <sys/cape_thread.h>
#include <fmt/cape_json.h>

//-----------------------------------------------------------------------------

int __STDCALL client02_create_thread (void* ptr)
{
  int res;
  
  CapeErr err = cape_err_new ();
  CapeUdc remote = cape_udc_new (CAPE_UDC_NODE, NULL);
  QBus qbus = qbus_new ("NODE2");
  
  // construct bind parameters
  {
    cape_udc_add_s_cp (remote, "type", "tcp");
    cape_udc_add_s_cp (remote, "host", "127.0.0.1");
    cape_udc_add_n (remote, "port", 33380);
  }
  
  res = qbus_wait (qbus, NULL, remote, 5, err);
  
  qbus_del (&qbus);
  cape_udc_del (&remote);
  cape_err_del (&err);
  
  return FALSE;
}

//-----------------------------------------------------------------------------

static int __STDCALL client01_test01__on01 (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  number_t i;
  number_t loop_cnt = (number_t)(rand() % 100000);
  
  cape_log_fmt (CAPE_LL_TRACE, "TEST", "client01 test01", "on 01 -> %liloops", loop_cnt);

  for (i = 0; i < loop_cnt; i++)
  {
    
  }
  
  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

static int __STDCALL client01_test01 (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  
  if (qin->rinfo == NULL)
  {
    return cape_err_set (err, CAPE_ERR_RUNTIME, "rinfo is NULL");
  }

  // clean up
  qbus_message_clr (qin, CAPE_UDC_UNDEFINED);
  
  return qbus_continue (qbus, "NODE0", "test01", qin, (void**)NULL, client01_test01__on01, err);
}

//-----------------------------------------------------------------------------

int __STDCALL client01_create_thread (void* ptr)
{
  int res;
  
  CapeErr err = cape_err_new ();
  CapeUdc remote = cape_udc_new (CAPE_UDC_NODE, NULL);
  QBus qbus = qbus_new ("NODE1");
  
  // construct bind parameters
  {
    cape_udc_add_s_cp (remote, "type", "tcp");
    cape_udc_add_s_cp (remote, "host", "127.0.0.1");
    cape_udc_add_n (remote, "port", 33380);
  }

  qbus_register (qbus, "test01", NULL, client01_test01, NULL, err);

  res = qbus_wait (qbus, NULL, remote, 5, err);
  
  qbus_del (&qbus);
  cape_udc_del (&remote);
  cape_err_del (&err);

  return FALSE;
}

//-----------------------------------------------------------------------------

static int __STDCALL server_test01 (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  number_t wait_in_ms = (number_t)(rand() % 10);

  cape_log_fmt (CAPE_LL_TRACE, "TEST", "server test01", "got call, wait -> %lims", wait_in_ms);

  cape_thread_sleep (wait_in_ms);
  
  return CAPE_ERR_NONE;
}

//-------------------------------------------------------------------------------------

static int __STDCALL qbus_trigger_thread__on (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  
  if (qin->rinfo == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_RUNTIME, "rinfo is NULL");
    goto exit_and_cleanup;
  }

exit_and_cleanup:
  
  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

int __STDCALL qbus_trigger_thread (void* ptr)
{
  QBus qbus = ptr;
  static number_t diff = 0;
  
  // local objects
  QBusM qin = qbus_message_new (NULL, NULL);
  CapeErr err = cape_err_new ();

  if (diff == 0)
  {
    cape_thread_sleep (400);
    cape_log_msg (CAPE_LL_TRACE, "TEST", "trigger", "start with requests");
  }

  // add rinfo for checking
  qin->rinfo = cape_udc_new (CAPE_UDC_NODE, NULL);
  cape_udc_add_n (qin->rinfo, "userid", 99);
  
  qbus_send (qbus, "NODE1", "test01", qin, ptr, qbus_trigger_thread__on, err);
  
  diff++;
  
  cape_err_del (&err);
  qbus_message_del (&qin);
  
  //if (diff > 100)
  {
    // this allow a context switch
    cape_thread_sleep (0);
  }
  
  
  return diff < 1000;
}

//-----------------------------------------------------------------------------

int __STDCALL server_create_thread (void* ptr)
{
  int res;
  
  CapeErr err = cape_err_new ();
  CapeUdc bind = cape_udc_new (CAPE_UDC_NODE, NULL);
  QBus qbus = qbus_new ("NODE0");
  CapeThread trigger_thread = cape_thread_new ();

  // construct bind parameters
  {
    cape_udc_add_s_cp (bind, "type", "tcp");
    cape_udc_add_s_cp (bind, "host", "127.0.0.1");
    cape_udc_add_n (bind, "port", 33380);
  }
  
  qbus_register (qbus, "test01", NULL, server_test01, NULL, err);

  cape_thread_start (trigger_thread, qbus_trigger_thread, qbus);

  res = qbus_wait (qbus, bind, NULL, 5, err);
  
  cape_thread_join (trigger_thread);

  cape_thread_del (&trigger_thread);
  qbus_del (&qbus);
  cape_udc_del (&bind);
  cape_err_del (&err);

  return FALSE;
}

//-----------------------------------------------------------------------------

int main (int argc, char *argv[])
{
  int res;
  CapeErr err = cape_err_new ();
  
  CapeThread server_thread = cape_thread_new ();
  CapeThread client01_thread = cape_thread_new ();
  CapeThread client02_thread = cape_thread_new ();

  cape_thread_start (server_thread, server_create_thread, NULL);
  
  cape_thread_sleep (100);
  
  cape_thread_start (client01_thread, client01_create_thread, NULL);
  cape_thread_start (client02_thread, client02_create_thread, NULL);

  cape_thread_join (server_thread);
  cape_thread_join (client01_thread);
  cape_thread_join (client02_thread);

  cape_thread_del (&server_thread);
  cape_thread_del (&client01_thread);
  cape_thread_del (&client02_thread);

  cape_err_del (&err);
}

//=====================================================================================

