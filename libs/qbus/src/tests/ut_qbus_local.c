#include <qbus.h>

// cape includes
#include <sys/cape_log.h>
#include <sys/cape_thread.h>
#include <fmt/cape_json.h>

// linux includes
#include <signal.h>
#include <unistd.h>

static int g_running = TRUE;

//-------------------------------------------------------------------------------------

struct QbusTestRequest_s
{
  QBus qbus;              // reference
  
  CapeString content;
  
}; typedef struct QbusTestRequest_s* QbusTestRequest;

//-------------------------------------------------------------------------------------

QbusTestRequest qbus_test_request_new (QBus qbus)
{
  QbusTestRequest self = CAPE_NEW (struct QbusTestRequest_s);
  
  self->qbus = qbus;
  self->content = NULL;
  
  return self;
}

//-------------------------------------------------------------------------------------

void qbus_test_request_del (QbusTestRequest* p_self)
{
  if (*p_self)
  {
    QbusTestRequest self = *p_self;
    
    cape_str_del (&(self->content));
    
    CAPE_DEL (p_self, struct QbusTestRequest_s);
  }
}

//-------------------------------------------------------------------------------------

static int __STDCALL qbus_test_request__test1__on_complex (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  QbusTestRequest self = ptr;
  
  if (qin->err)
  {
    res = cape_err_set (err, CAPE_ERR_RUNTIME, cape_err_text (qin->err));
    goto exit_and_cleanup;
  }
  
  if (qin->rinfo == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_RUNTIME, "rinfo is NULL");
    goto exit_and_cleanup;
  }

  if (qin->cdata == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "cdata is missing");
    goto exit_and_cleanup;
  }
  
  {
    CapeString h = cape_json_to_s (qin->cdata);
    
    //printf ("ON COMPLEX: %s\n", h);
    
    cape_str_del (&h);
  }

  res = CAPE_ERR_NONE;
  cape_udc_replace_mv (&(qout->cdata), &(qin->cdata));
  
exit_and_cleanup:
  
  if (cape_err_code (err))
  {
    cape_log_msg (CAPE_LL_ERROR, "MAIN", "on complex", cape_err_text (err));
  }

  qbus_test_request_del (&self);
  return res;
}

//-------------------------------------------------------------------------------------

static int __STDCALL qbus_test_request__test1__on_simple (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  QbusTestRequest self = ptr;
  
  if (qin->err)
  {
    res = cape_err_set (err, CAPE_ERR_RUNTIME, cape_err_text (qin->err));
    goto exit_and_cleanup;
  }
  
  if (qin->rinfo == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_RUNTIME, "rinfo is NULL");
    goto exit_and_cleanup;
  }
  
  if (qin->cdata == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "cdata is missing");
    goto exit_and_cleanup;
  }

  {
    CapeString h = cape_json_to_s (qin->cdata);
    
    //printf ("ON SIMPLE: %s\n", h);
    
    cape_str_del (&h);
  }
  
  // clean up
  qbus_message_clr (qin, CAPE_UDC_NODE);
  
  cape_udc_add_n (qin->cdata, "var1", 42);
  cape_udc_add_s_cp (qin->cdata, "var2", "hello world");
  
  res = qbus_continue (self->qbus, "TEST", "complex", qin, (void**)&self, qbus_test_request__test1__on_complex, err);

exit_and_cleanup:
  
  if (cape_err_code (err))
  {
    cape_log_msg (CAPE_LL_ERROR, "MAIN", "on simple", cape_err_text (err));
  }

  qbus_test_request_del (&self);
  return res;
}

//-------------------------------------------------------------------------------------

int qbus_test_request__test1 (QbusTestRequest* p_self, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  QbusTestRequest self = *p_self;
  
  if (qin->rinfo == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_RUNTIME, "rinfo is NULL");
    goto exit_and_cleanup;
  }

  // clean up
  qbus_message_clr (qin, CAPE_UDC_NODE);
  
  cape_udc_add_n (qin->cdata, "var1", 42);
  cape_udc_add_s_cp (qin->cdata, "var2", "hello world");
  
  res = qbus_continue (self->qbus, "TEST", "simple", qin, (void**)p_self, qbus_test_request__test1__on_simple, err);

exit_and_cleanup:
  
  qbus_test_request_del (p_self);
  return res;
}

//-------------------------------------------------------------------------------------

int qbus_test_request__test2 (QbusTestRequest* p_self, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  
  if (qin->rinfo == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_RUNTIME, "rinfo is NULL");
    goto exit_and_cleanup;
  }

  if (qin->cdata == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "cdata is missing");
    goto exit_and_cleanup;
  }
  
  res = CAPE_ERR_NONE;
  
  // add some output
  qout->cdata = cape_udc_new (CAPE_UDC_NODE, NULL);
  
  cape_udc_add_n (qout->cdata, "var3", 42);
  cape_udc_add_s_cp (qout->cdata, "var4", "hello world");

exit_and_cleanup:
  
  if (cape_err_code (err))
  {
    cape_log_msg (CAPE_LL_ERROR, "TEST2", "entry", cape_err_text (err));
  }
    
  qbus_test_request_del (p_self);
  return res;
}

//-------------------------------------------------------------------------------------

static int __STDCALL qbus_test_request__complex__onl2 (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  QbusTestRequest self = ptr;
  
  if (qin->err)
  {
    res = cape_err_set (err, CAPE_ERR_RUNTIME, cape_err_text (qin->err));
    goto exit_and_cleanup;
  }
  
  if (qin->rinfo == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_RUNTIME, "rinfo is NULL");
    goto exit_and_cleanup;
  }

  if (qin->cdata == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "cdata is missing");
    goto exit_and_cleanup;
  }
  
  res = CAPE_ERR_NONE;
  
  // forward qin
  cape_udc_replace_mv (&(qout->cdata), &(qin->cdata));
  
exit_and_cleanup:
  
  qbus_test_request_del (&self);
  return res;
}

//-------------------------------------------------------------------------------------

int qbus_test_request__complex (QbusTestRequest* p_self, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  QbusTestRequest self = *p_self;
  
  if (qin->cdata == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "cdata is missing");
    goto exit_and_cleanup;
  }
  
  // clean up
  qbus_message_clr (qin, CAPE_UDC_NODE);
  
  cape_udc_add_n (qin->cdata, "var5", 42);
  cape_udc_add_s_cp (qin->cdata, "var6", "hello world");
  
  res = qbus_continue (self->qbus, "TEST", "complex_l2", qin, (void**)p_self, qbus_test_request__complex__onl2, err);

exit_and_cleanup:
  
  if (cape_err_code (err))
  {
    cape_log_msg (CAPE_LL_ERROR, "TEST2", "entry", cape_err_text (err));
  }
  
  qbus_test_request_del (p_self);
  return res;
}

//-------------------------------------------------------------------------------------

int qbus_test_request__complex_l2 (QbusTestRequest* p_self, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  
  if (qin->cdata == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "cdata is missing");
    goto exit_and_cleanup;
  }

  res = CAPE_ERR_NONE;
  
  // add some output
  qout->cdata = cape_udc_new (CAPE_UDC_NODE, NULL);
  
  cape_udc_add_n (qout->cdata, "var7", 42);
  cape_udc_add_s_cp (qout->cdata, "var8", "hello world");
  
exit_and_cleanup:
  
  if (cape_err_code (err))
  {
    cape_log_msg (CAPE_LL_ERROR, "COMPLEX2", "entry", cape_err_text (err));
  }
  
  qbus_test_request_del (p_self);
  return res;
}

//=====================================================================================

static int __STDCALL qbus_test1 (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  // create a temporary object
  QbusTestRequest obj = qbus_test_request_new (qbus);
  
  // run the command
  return qbus_test_request__test1 (&obj, qin, qout, err);
}

//-------------------------------------------------------------------------------------

static int __STDCALL qbus_test2 (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  // create a temporary object
  QbusTestRequest obj = qbus_test_request_new (qbus);
  
  // run the command
  return qbus_test_request__test2 (&obj, qin, qout, err);
}

//-------------------------------------------------------------------------------------

static int __STDCALL qbus_method_complex (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  // create a temporary object
  QbusTestRequest obj = qbus_test_request_new (qbus);
  
  // run the command
  return qbus_test_request__complex (&obj, qin, qout, err);
}

//-------------------------------------------------------------------------------------

static int __STDCALL qbus_method_complex_l2 (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  // create a temporary object
  QbusTestRequest obj = qbus_test_request_new (qbus);
  
  // run the command
  return qbus_test_request__complex_l2 (&obj, qin, qout, err);
}

//=====================================================================================

struct TriggerContext
{
  QBus qbus;
  number_t cnt;
};

//-----------------------------------------------------------------------------

static number_t diff = 0;

//-------------------------------------------------------------------------------------

static int __STDCALL qbus_trigger_thread__on (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  
  if (qin->rinfo == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_RUNTIME, "rinfo is NULL");
    goto exit_and_cleanup;
  }

  if (qin->cdata == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "cdata is missing");
    goto exit_and_cleanup;
  }
  
  {
    CapeString h = cape_json_to_s (qin->cdata);
    
    //printf ("ON MAIN: %s\n", h);
    
    cape_str_del (&h);
  }
  
  res = CAPE_ERR_NONE;

exit_and_cleanup:
  
  if (cape_err_code (err))
  {
    cape_log_msg (CAPE_LL_ERROR, "MAIN", "on main", cape_err_text (err));
  }
  
  diff--;

  return res;
}

//-----------------------------------------------------------------------------

int __STDCALL qbus_trigger_thread (void* ptr)
{
  struct TriggerContext* ctx = ptr;
  
  // local objects
  QBusM qin = qbus_message_new (NULL, NULL);
  CapeErr err = cape_err_new ();
  
  // add rinfo for checking
  qin->rinfo = cape_udc_new (CAPE_UDC_NODE, NULL);
  cape_udc_add_n (qin->rinfo, "userid", 99);

  qbus_send (ctx->qbus, "TEST", "main", qin, ptr, qbus_trigger_thread__on, err);
  
  diff++;
  
  cape_err_del (&err);
  qbus_message_del (&qin);
  
  if (diff > 100)
  {
    // this allow a context switch
    cape_thread_sleep (0);
  }
  
  ctx->cnt++;
  
  if (ctx->cnt > 100000000)
  {
    printf ("CNT: %lu\n", ctx->cnt);
    kill (getpid(), SIGINT);    
  }
    
  return g_running;
   
/*
 *  //cape_thread_sleep (100);
 *
 *  // terminate qbus
  kill (getpid(), SIGINT);

  return FALSE;
  */
}

//-----------------------------------------------------------------------------

int main (int argc, char *argv[])
{
  int res;
  struct TriggerContext ctx;
  
  CapeErr err = cape_err_new ();
  
  // local objects
  QBus qbus = qbus_new ("test");
  CapeThread trigger_thread = cape_thread_new ();
  
  qbus_register (qbus, "main"        , NULL, qbus_test1, NULL, err);
  qbus_register (qbus, "simple"      , NULL, qbus_test2, NULL, err);
  qbus_register (qbus, "complex"     , NULL, qbus_method_complex, NULL, err);
  qbus_register (qbus, "complex_l2"  , NULL, qbus_method_complex_l2, NULL, err);

  cape_log_set_level (CAPE_LL_INFO);
  
  // initialize
  ctx.qbus = qbus;
  ctx.cnt = 0;
  
  cape_thread_start (trigger_thread, qbus_trigger_thread, &ctx);
  
  qbus_wait (qbus, NULL, NULL, 30, err);
  
  g_running = FALSE;
  
  cape_thread_join (trigger_thread);

  qbus_del (&qbus);
  cape_thread_del (&trigger_thread);

  cape_err_del (&err);
  
  printf ("done\n");
  return 0;
}

//=====================================================================================

