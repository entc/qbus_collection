#include <qbus.h>
#include <qbus_pvd.h>

// cape includes
#include <sys/cape_log.h>
#include <sys/cape_thread.h>
#include <fmt/cape_json.h>
#include <sys/cape_mutex.h>

#define REQUESTS 100000

static number_t running = TRUE;

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
  
  while (cape_aio_context_next (qbus_aio (qbus), 200, err) == CAPE_ERR_NONE && running);

  qbus_del (&qbus);
  cape_udc_del (&pvds);
  cape_err_del (&err);
  
  return FALSE;
}

//-----------------------------------------------------------------------------

static int __STDCALL client01_method01 (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  if (qin->rinfo == NULL)
  {
    return cape_err_set (err, CAPE_ERR_RUNTIME, "rinfo is NULL");
  }

  return CAPE_ERR_NONE;
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

  qbus_register (qbus, "method01", NULL, client01_method01, NULL, err);
  
  res = qbus_init (qbus, pvds, 5, err);
  if (res)
  {
    
  }
  
  while (cape_aio_context_next (qbus_aio (qbus), 200, err) == CAPE_ERR_NONE && running);

  qbus_del (&qbus);
  cape_udc_del (&pvds);
  cape_err_del (&err);

  return FALSE;
}

//-------------------------------------------------------------------------------------

static int __STDCALL qbus_trigger_thread__on_method01 (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
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

  running = FALSE;
  
  return res;
}

//-----------------------------------------------------------------------------

int __STDCALL server_create_thread (void* ptr)
{
  int res;
  
  CapeErr err = cape_err_new ();
  QBus qbus = qbus_new ("NODE0");

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
  
  res = qbus_init (qbus, pvds, 5, err);
  if (res)
  {
    
  }

  cape_thread_sleep (500);

  {
    // local objects
    QBusM qin = qbus_message_new (NULL, NULL);

    qbus_send (qbus, "NODE1", "method01", qin, ptr, qbus_trigger_thread__on_method01, err);
  }
  
  while (cape_aio_context_next (qbus_aio (qbus), 200, err) == CAPE_ERR_NONE && running);

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

