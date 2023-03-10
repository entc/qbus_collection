#include "qbus.h"

#include <stdlib.h>

// cape includes
#include <aio/cape_aio_timer.h>
#include <fmt/cape_json.h>

//-----------------------------------------------------------------------------

static int __STDCALL test_method (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  
  res = CAPE_ERR_NONE;
  
  return res;
}

//-----------------------------------------------------------------------------

int __STDCALL on_timer (void* ptr)
{
  QBus qbus = ptr;
  
  CapeUdc value = cape_udc_new (CAPE_UDC_STRING, NULL);

  cape_udc_set_s_cp (value, "hello world");
  
  qbus_emit (ptr, "val01", &value);
  
  return TRUE;
}

//-----------------------------------------------------------------------------

void __STDCALL on_value (void* user_ptr, const CapeString subscriber_name, CapeUdc* p_data)
{
  if (*p_data)
  {
    CapeString h = cape_json_to_s (*p_data);
    
    printf ("ON VAL: %s <- %s\n", subscriber_name, h);
    
    cape_str_del (&h);
  }
}

//-----------------------------------------------------------------------------

static int __STDCALL app_on_init (QBus qbus, void* ptr, void** p_ptr, CapeErr err)
{
  int res;
  
  // register methods
  qbus_register (qbus, "test_method", NULL, test_method, NULL, err);

  qbus_subscribe (qbus, 0, "test1", "val01", NULL, on_value);
  qbus_subscribe (qbus, 0, "test1", "val02", NULL, on_value);

  {
    CapeAioTimer timer = cape_aio_timer_new ();
    
    res = cape_aio_timer_set (timer, 2000, qbus, on_timer, err);
    if (res)
    {
      goto cleanup_and_exit;
    }
    
    res = cape_aio_timer_add (&timer, qbus_aio (qbus));
    if (res)
    {
      goto cleanup_and_exit;
    }
  }
  
  res = CAPE_ERR_NONE;
  
cleanup_and_exit:
  
  return res;
}

//-----------------------------------------------------------------------------

static int __STDCALL app_on_done (QBus qbus, void* ptr, CapeErr err)
{
  
  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

int main (int argc, char *argv[])
{
  qbus_instance ("TEST", NULL, app_on_init, app_on_done, argc, argv);
  
  return 0;
}

//-----------------------------------------------------------------------------
