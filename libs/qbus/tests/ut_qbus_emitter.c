#include <qbus.h>

// cape includes
#include <sys/cape_log.h>
#include <sys/cape_thread.h>
#include <fmt/cape_json.h>
#include <sys/cape_mutex.h>

#define REQUESTS 0

//-----------------------------------------------------------------------------

static number_t total_runs = REQUESTS;
CapeMutex mutex = NULL;

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
  
  while (cape_aio_context_next (qbus_aio (qbus), 200, err) == CAPE_ERR_NONE && total_runs > 0);

  qbus_del (&qbus);
  cape_err_del (&err);
  
  return FALSE;
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
  
  while (cape_aio_context_next (qbus_aio (qbus), 200, err) == CAPE_ERR_NONE && total_runs > 0);

  qbus_del (&qbus);
  cape_err_del (&err);

  return FALSE;
}

//-----------------------------------------------------------------------------

int __STDCALL server_create_thread (void* ptr)
{
  int res;
  
  CapeErr err = cape_err_new ();
  CapeUdc args = cape_udc_new (CAPE_UDC_NODE, NULL);
  QBus qbus = qbus_new ("NODE0");

  res = qbus_init (qbus, &args, err);
  if (res)
  {
    
  }
  
  while (cape_aio_context_next (qbus_aio (qbus), 200, err) == CAPE_ERR_NONE && total_runs > 0);

  qbus_del (&qbus);
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

