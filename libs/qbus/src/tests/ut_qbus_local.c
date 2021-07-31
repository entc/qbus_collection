#include <qbus.h>

// cape includes
#include <sys/cape_log.h>
#include <sys/cape_thread.h>

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

static int __STDCALL qbus_test_request__test1__on (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  QbusTestRequest self = ptr;
  
  if (qin->err)
  {
    res = cape_err_set (err, CAPE_ERR_RUNTIME, cape_err_text (qin->err));
    goto exit_and_cleanup;
  }
  
  res = CAPE_ERR_NONE;
  
exit_and_cleanup:
  
  qbus_test_request_del (&self);
  return res;
}

//-------------------------------------------------------------------------------------

int qbus_test_request__test1 (QbusTestRequest* p_self, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  QbusTestRequest self = *p_self;
  
  // clean up
  qbus_message_clr (qin, CAPE_UDC_UNDEFINED);
  res = qbus_continue (self->qbus, "TEST", "method2", qin, (void**)p_self, qbus_test_request__test1__on, err);

exit_and_cleanup:
  
  qbus_test_request_del (p_self);
  return res;
}

//-------------------------------------------------------------------------------------

int qbus_test_request__test2 (QbusTestRequest* p_self, QBusM qin, QBusM qout, CapeErr err)
{
  int res;
  
  res = CAPE_ERR_NONE;
  
exit_and_cleanup:
  
  qbus_test_request_del (p_self);
  return res;
}

//-------------------------------------------------------------------------------------

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

//-----------------------------------------------------------------------------

struct TriggerContext
{
  QBus qbus;
  number_t cnt;
};

//-------------------------------------------------------------------------------------

static int __STDCALL qbus_trigger_thread__on (QBus qbus, void* ptr, QBusM qin, QBusM qout, CapeErr err)
{

  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

int __STDCALL qbus_trigger_thread (void* ptr)
{
  struct TriggerContext* ctx = ptr;
  
  // local objects
  QBusM qin = qbus_message_new (NULL, NULL);
  CapeErr err = cape_err_new ();
  
  qbus_send (ctx->qbus, "TEST", "method1", qin, ptr, qbus_trigger_thread__on, err);
  
  cape_err_del (&err);
  qbus_message_del (&qin);
  
  ctx->cnt++;
  
  if (ctx->cnt > 10000)
  {
    kill (getpid(), SIGINT);
  }
  
  printf ("CNT: %lu\n", ctx->cnt);
  
  return g_running;
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
  
  qbus_register (qbus, "method1"      , NULL, qbus_test1, NULL, err);
  qbus_register (qbus, "method2"      , NULL, qbus_test2, NULL, err);
  
  // initialize
  ctx.qbus = qbus;
  ctx.cnt = 0;
  
  cape_thread_start (trigger_thread, qbus_trigger_thread, &ctx);
  
  qbus_wait (qbus, NULL, NULL, 2, err);
  
  g_running = FALSE;
  
  cape_thread_join (trigger_thread);

  qbus_del (&qbus);
  cape_thread_del (&trigger_thread);

  cape_err_del (&err);
  
  return 0;
}

