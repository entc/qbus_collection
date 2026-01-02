#include <qbus.h>

// cape includes
#include <sys/cape_log.h>
#include <sys/cape_thread.h>
#include <fmt/cape_json.h>
#include <sys/cape_mutex.h>

#define WAIT_AFTER_INIT 1000

CapeMutex mutex = NULL;

//-----------------------------------------------------------------------------

void __STDCALL client02_on_val (void* ptr, CapeUdc val)
{
  if (val)
  {
    const CapeString s = cape_udc_s (val, "[none]");
    
    printf ("[client02] received value: %s\n", s);
  }
}

//-----------------------------------------------------------------------------

int __STDCALL client02_create_thread (void* ptr)
{
  int res;

  CapeErr err = cape_err_new ();
  CapeUdc args = cape_udc_new (CAPE_UDC_NODE, NULL);
  QBus qbus = qbus_new ("NODE2");

  res = qbus_init (qbus, &args, err);
  if (res)
  {

  }

  cape_thread_sleep (WAIT_AFTER_INIT);

  qbus_subscribe (qbus, "node1", "param1", NULL, client02_on_val, err);
  qbus_subscribe (qbus, "node1", "param2", NULL, client02_on_val, err);

  cape_thread_sleep (WAIT_AFTER_INIT);

  while (cape_aio_context_next (qbus_aio (qbus), 1000, err) == CAPE_ERR_NONE)
  {
    CapeUdc val = cape_udc_new (CAPE_UDC_STRING, NULL);
    cape_udc_set_s_cp (val, "hello world 02");

    qbus_next (qbus, "param1", &val);
  }

  qbus_del (&qbus);
  cape_err_del (&err);

  return FALSE;
}

//-----------------------------------------------------------------------------

void __STDCALL client01_on_val (void* ptr, CapeUdc val)
{
  if (val)
  {
    const CapeString s = cape_udc_s (val, "[none]");

    printf ("[client01] received value: %s\n", s);
  }

}

//-----------------------------------------------------------------------------

int __STDCALL client01_create_thread (void* ptr)
{
  int res;

  CapeErr err = cape_err_new ();
  CapeUdc args = cape_udc_new (CAPE_UDC_NODE, NULL);
  QBus qbus = qbus_new ("NODE1");

  res = qbus_init (qbus, &args, err);
  if (res)
  {

  }
  
  qbus_subscribe (qbus, "node2", "param1", NULL, client01_on_val, err);
  qbus_subscribe (qbus, "node2", "param2", NULL, client01_on_val, err);

  cape_thread_sleep (WAIT_AFTER_INIT);

  while (cape_aio_context_next (qbus_aio (qbus), 1000, err) == CAPE_ERR_NONE)
  {
    CapeUdc val = cape_udc_new (CAPE_UDC_STRING, NULL);
    cape_udc_set_s_cp (val, "hello world 01");
    
    qbus_next (qbus, "param1", &val);
  }

  qbus_del (&qbus);
  cape_err_del (&err);

  return FALSE;
}

//-----------------------------------------------------------------------------

int main (int argc, char *argv[])
{
  int res;
  CapeErr err = cape_err_new ();

  CapeThread client01_thread = cape_thread_new ();
  CapeThread client02_thread = cape_thread_new ();

  mutex = cape_mutex_new();

  cape_thread_sleep (100);

  cape_thread_start (client01_thread, client01_create_thread, NULL);
  cape_thread_start (client02_thread, client02_create_thread, NULL);

  cape_thread_join (client01_thread);
  cape_thread_join (client02_thread);

  cape_mutex_del (&mutex);

  cape_thread_del (&client01_thread);
  cape_thread_del (&client02_thread);

  cape_err_del (&err);
}

//=====================================================================================

