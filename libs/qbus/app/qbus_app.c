#include "qbus.h"

#include <stdlib.h>

// cape includes
#include <aio/cape_aio_timer.h>
#include <sys/cape_log.h>
#include <fmt/cape_json.h>

//-----------------------------------------------------------------------------

static int __STDCALL test_method (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  int res;

  res = CAPE_ERR_NONE;

  return res;
}

//-----------------------------------------------------------------------------

static int __STDCALL test1__on_fct1 (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
    int res;
    
    if (qin->err)
    {
        res = cape_err_set (err, CAPE_ERR_RUNTIME, cape_err_text (qin->err));
        goto exit_and_cleanup;
    }
    
    {
        CapeString ch = cape_json_to_s (qin->cdata);
        
        cape_log_msg (CAPE_LL_TRACE, "TEST1", "on fct1", ch);

        cape_str_del (&ch);
    }
    
    res = CAPE_ERR_NONE;
    
exit_and_cleanup:
  
    return res;
}

//-----------------------------------------------------------------------------

int __STDCALL app__on_reg_fct1 (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
    CapeString ch = cape_json_to_s (qin->cdata);
  
  
    cape_str_del (&ch);
    
    return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

int __STDCALL test1__on_timer (void* user_ptr)
{
    int res;
    QBus qbus = user_ptr;
    
    // local objects
    CapeErr err = cape_err_new ();
    QBusM msg = qbus_message_new (NULL, NULL);

    msg->cdata = cape_udc_new (CAPE_UDC_NODE, NULL);
    
    cape_udc_add_s_cp (msg->cdata, "hello", "world");
    
    res = qbus_send (qbus, "test2", "function1", msg, qbus, test1__on_fct1, err);

    qbus_message_del (&msg);
    cape_err_del (&err);
    return TRUE;
}

//-----------------------------------------------------------------------------

static int __STDCALL test1_on_init (QBus qbus, void* ptr, void** p_ptr, CapeErr err)
{
    int res;
    
    // local objects
    CapeAioTimer timer = cape_aio_timer_new ();

    res = cape_aio_timer_set (timer, 1000, qbus, test1__on_timer, err);
    if (res)
    {
        goto exit_and_cleanup;
    }
    
    res = cape_aio_timer_add (&timer, qbus_aio (qbus));
    if (res)
    {
        cape_err_set (err, CAPE_ERR_RUNTIME, "ERR.TIMER_ADD");
        goto exit_and_cleanup;
    }
    
    qbus_register (qbus, "function1", qbus, app__on_reg_fct1, NULL, err);

    res = CAPE_ERR_NONE;
    
exit_and_cleanup:
    
    if (res)
    {
        cape_log_msg (CAPE_LL_ERROR, "TEST1", "on init", cape_err_text (err));
    }
    
    return res;
}

//-----------------------------------------------------------------------------

static int __STDCALL test1_on_done (QBus qbus, void* ptr, CapeErr err)
{
    cape_log_msg (CAPE_LL_DEBUG, "TEST1", "on done", "on done function called");

    return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

int __STDCALL test2__on_reg_fct1 (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
    CapeString ch = cape_json_to_s (qin->cdata);

    cape_log_msg (CAPE_LL_DEBUG, "TEST2", "on fct1", ch);

    cape_str_del (&ch);
  
    return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

static int __STDCALL test2_on_init (QBus qbus, void* ptr, void** p_ptr, CapeErr err)
{
    qbus_register (qbus, "function1", qbus, test2__on_reg_fct1, NULL, err);

    return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

int main (int argc, char *argv[])
{
    // local objects
    CapeErr err = cape_err_new ();
    
    // create the daemonized version
    QBus qbus_test2 = qbus_new ("test2", NULL);
    
    qbus_set_cb (qbus_test2, NULL, test2_on_init, NULL);

    // run in background
    qbus_run__d (qbus_test2, err);
    
    qbus_instance ("TEST1", NULL, test1_on_init, test1_on_done, argc, argv);

    qbus_del (&qbus_test2);
    
    cape_err_del (&err);
    
    return 0;
}

//-----------------------------------------------------------------------------
