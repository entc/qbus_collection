#include <qbus.h>
#include <qbus_pvd.h>

// cape includes
#include <sys/cape_log.h>
#include <sys/cape_thread.h>
#include <fmt/cape_json.h>
#include <sys/cape_mutex.h>

#define REQUESTS 100000

static number_t total_runs = REQUESTS;
CapeMutex mutex = NULL;

//-----------------------------------------------------------------------------

int __STDCALL client02_create_thread (void* ptr)
{
  int res;
  
  CapeErr err = cape_err_new ();
  QBus qbus = qbus_new ("NODE2");
  
  CapeUdc pvds = cape_udc_new (CAPE_UDC_NODE, NULL);
  
  // construct bind parameters
  {
    CapeUdc pvd_entity = cape_udc_new (CAPE_UDC_NODE, NULL);

    cape_udc_add_s_cp (pvd_entity, "type", "tcp");
    cape_udc_add_s_cp (pvd_entity, "host", "127.0.0.1");
    cape_udc_add_n (pvd_entity, "port", 33380);
    cape_udc_add_n (pvd_entity, "mode", QBUS_PVD_MODE_CLIENT);
    
    cape_udc_add (pvds, &pvd_entity);
  }
  
  res = qbus_init (qbus, pvds, 5, err);
  if (res)
  {
   
    
  }
  
  while (cape_aio_context_next (qbus_aio (qbus), 200, err) == CAPE_ERR_NONE && total_runs > 0);

  qbus_del (&qbus);
  cape_udc_del (&pvds);
  cape_err_del (&err);
  
  return FALSE;
}

//-----------------------------------------------------------------------------

static int __STDCALL client01_test01__on01 (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  number_t i;
  number_t loop_cnt = (number_t)(rand() % 100000);
  
  if (qin->err)
  {
    cape_log_msg (CAPE_LL_ERROR, "TEST", "test01 on01", cape_err_text (qin->err));
    abort();
  }
  
  cape_log_fmt (CAPE_LL_TRACE, "TEST", "client01 test01", "on 01 -> %liloops", loop_cnt);

  /*
  for (i = 0; i < loop_cnt; i++)
  {
    
  }
  */
  
  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

static int __STDCALL client01_test01 (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  if (qin->rinfo == NULL)
  {
    return cape_err_set (err, CAPE_ERR_RUNTIME, "rinfo is NULL");
  }

  number_t splitter = (number_t)(rand() % 5);
  
  switch (splitter)
  {
    case 1:
    {
      // clean up
      qbus_message_clr (qin, CAPE_UDC_UNDEFINED);
      
      return qbus_continue (qbus, "NODE1", "test02", qin, (void**)NULL, client01_test01__on01, err);
    }
    case 2:
    {
      return cape_err_set (err, CAPE_ERR_RUNTIME, "some error");
    }
    default:
    {
      // clean up
      qbus_message_clr (qin, CAPE_UDC_UNDEFINED);
      
      return qbus_continue (qbus, "NODE0", "test01", qin, (void**)NULL, client01_test01__on01, err);
    }
  }
}

//-----------------------------------------------------------------------------

static int __STDCALL client01_test02 (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
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
  QBus qbus = qbus_new ("NODE1");

  CapeUdc pvds = cape_udc_new (CAPE_UDC_NODE, NULL);
  
  // construct bind parameters
  {
    CapeUdc pvd_entity = cape_udc_new (CAPE_UDC_NODE, NULL);
    
    cape_udc_add_s_cp (pvd_entity, "type", "tcp");
    cape_udc_add_s_cp (pvd_entity, "host", "127.0.0.1");
    cape_udc_add_n (pvd_entity, "port", 33380);
    cape_udc_add_n (pvd_entity, "mode", QBUS_PVD_MODE_CLIENT);
    
    cape_udc_add (pvds, &pvd_entity);
  }

  qbus_register (qbus, "test01", NULL, client01_test01, NULL, err);
  qbus_register (qbus, "test02", NULL, client01_test02, NULL, err);
  
  res = qbus_init (qbus, pvds, 5, err);
  if (res)
  {
    
  }
  
  while (cape_aio_context_next (qbus_aio (qbus), 200, err) == CAPE_ERR_NONE && total_runs > 0);

  qbus_del (&qbus);
  cape_udc_del (&pvds);
  cape_err_del (&err);

  return FALSE;
}

//-----------------------------------------------------------------------------

static int __STDCALL server_test01 (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  number_t wait_in_ms = (number_t)(rand() % 100);

  cape_log_fmt (CAPE_LL_TRACE, "TEST", "server test01", "got call, wait -> %lims", wait_in_ms);

  //cape_thread_sleep (wait_in_ms);

  cape_log_fmt (CAPE_LL_TRACE, "TEST", "server test01", "continue-> %lims", wait_in_ms);

  return CAPE_ERR_NONE;
}

//-------------------------------------------------------------------------------------

static int __STDCALL qbus_trigger_thread__on (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  
  if (qin->err)
  {
    cape_log_msg (CAPE_LL_DEBUG, "TEST", "test01 on01", cape_err_text (qin->err));

    res = cape_err_code (qin->err);
  }
  else
  {
    res = CAPE_ERR_NONE;
  }
  
  cape_mutex_lock (mutex);
  
  total_runs--;
  
  printf ("current run: %li\n", total_runs);
  
  cape_mutex_unlock (mutex);
  
  return res;
}

//-----------------------------------------------------------------------------

int __STDCALL qbus_trigger_thread (void* ptr)
{
  QBus qbus = ptr;
  static number_t diff = 0;
  
  // local objects
  QBusM qin = qbus_message_new (NULL, NULL);
  CapeErr err = cape_err_new ();

  cape_thread_nosignals ();
  
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
  
  printf ("send request: %li\n", diff);
  
  return diff < REQUESTS;
}

//-----------------------------------------------------------------------------

int __STDCALL server_create_thread (void* ptr)
{
  int res;
  
  CapeErr err = cape_err_new ();
  QBus qbus = qbus_new ("NODE0");
  CapeThread trigger_thread = cape_thread_new ();

  CapeUdc pvds = cape_udc_new (CAPE_UDC_LIST, NULL);
  
  // construct bind parameters
  {
    CapeUdc pvd_entity = cape_udc_new (CAPE_UDC_NODE, NULL);
    
    cape_udc_add_s_cp (pvd_entity, "type", "tcp");
    cape_udc_add_s_cp (pvd_entity, "host", "127.0.0.1");
    cape_udc_add_n (pvd_entity, "port", 33380);
    cape_udc_add_n (pvd_entity, "mode", QBUS_PVD_MODE_LISTEN);
    
    cape_udc_add (pvds, &pvd_entity);
  }
  
  qbus_register (qbus, "test01", NULL, server_test01, NULL, err);
  
  res = qbus_init (qbus, pvds, 5, err);
  if (res)
  {
    
  }

  cape_thread_sleep (500);
  
  cape_thread_start (trigger_thread, qbus_trigger_thread, qbus);
  
  while (cape_aio_context_next (qbus_aio (qbus), 200, err) == CAPE_ERR_NONE && total_runs > 0);

  cape_thread_join (trigger_thread);

  cape_thread_del (&trigger_thread);
  qbus_del (&qbus);
  cape_udc_del (&pvds);
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

  mutex = cape_mutex_new();
  
  cape_thread_start (server_thread, server_create_thread, NULL);
  
  cape_thread_sleep (100);
  
  cape_thread_start (client01_thread, client01_create_thread, NULL);
  cape_thread_start (client02_thread, client02_create_thread, NULL);

  cape_thread_join (server_thread);
  cape_thread_join (client01_thread);
  cape_thread_join (client02_thread);

  cape_mutex_del (&mutex);
  
  cape_thread_del (&server_thread);
  cape_thread_del (&client01_thread);
  cape_thread_del (&client02_thread);

  cape_err_del (&err);
}

//=====================================================================================

