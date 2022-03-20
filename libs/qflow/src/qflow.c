#include "qflow.h"

#if defined __WINDOWS_OS

#include <windows.h>

#endif

// cape includes
#include <stc/cape_map.h>
#include <sys/cape_types.h>

// adbl includes
#include <adbl.h>

//-----------------------------------------------------------------------------

struct QFlowCtx_s
{
  AdblCtx ctx;
  AdblSession session;
  
}; typedef struct QFlowCtx_s* QFlowCtx;

//-----------------------------------------------------------------------------

QFlowCtx qflow_ctx_new (void)
{
  CapeErr err = cape_err_new ();
  
  AdblCtx ctx = NULL;
  AdblSession session = NULL;
  
  ctx = adbl_ctx_new ("adbl", "adbl2_mysql", err);
  if (ctx == NULL)
  {
    goto exit_and_cleanup;
  }

  session = adbl_session_open_file (ctx, "adbl_default.json", err);
  if (session == NULL)
  {
    goto exit_and_cleanup;
  }

  cape_err_del (&err);

  {
    QFlowCtx self = CAPE_NEW (struct QFlowCtx_s);
    
    self->ctx = ctx;
    ctx = NULL;
    
    self->session = session;
    session = NULL;
    
    return self;
  }
  
exit_and_cleanup:

  cape_err_del (&err);
  
  adbl_session_close (&session);
  adbl_ctx_del (&ctx);
  
  return NULL;
}

//-----------------------------------------------------------------------------

void qflow_ctx_del (QFlowCtx* p_self)
{
  if (*p_self)
  {
    QFlowCtx self = *p_self;
    
    adbl_session_close (&(self->session));
    adbl_ctx_del (&(self->ctx));
    
    CAPE_DEL (p_self, struct QFlowCtx_s);
  }
}

//-----------------------------------------------------------------------------

// singleton
QFlowCtx g_ctx = NULL;

//-----------------------------------------------------------------------------

QFlowCtx qflow_get_ctx ()
{
  QFlowCtx ctx;
  if (g_ctx == NULL)
  {
    ctx = qflow_ctx_new ();

#if defined __WINDOWS_OS

    // Only swap if the existing value is null.  If not on Windows,
    // use whatever compare and swap your platform provides.
    if (InterlockedCompareExchange (&g_ctx, ctx, NULL) != NULL)
    {
      qflow_ctx_del (&ctx);
    }

#else

    if (__sync_val_compare_and_swap (&g_ctx, NULL, ctx))
    {
      qflow_ctx_del (&ctx);
    }

#endif
    
  }

  return g_ctx;
}

//-----------------------------------------------------------------------------

struct QFlow_s
{
  AdblCtx ctx;
  
  AdblSession session;
  
  number_t raid;
  number_t rsid;
};

//-----------------------------------------------------------------------------

QFlow qflow_new (number_t raid)
{
  QFlow self = CAPE_NEW (struct QFlow_s);

  self->raid = raid;
  self->rsid = 0;
  
  return self;
}

//-----------------------------------------------------------------------------

void qflow_del (QFlow* p_self)
{
  if (*p_self)
  {
    //QFlow self = *p_self;
    
    CAPE_DEL (p_self, struct QFlow_s);
  }
}

//-----------------------------------------------------------------------------

void qflow_add (QFlow self, const CapeString description, number_t max)
{
  // local objects
  CapeErr err = cape_err_new ();
  AdblTrx trx = NULL;
  
  QFlowCtx ctx = qflow_get_ctx ();
  if (ctx == NULL)
  {
    goto exit_and_cleanup;
  }
  
  trx = adbl_trx_new (ctx->session, err);
  if (NULL == trx)
  {
    goto exit_and_cleanup;
  }
  
  {
    CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);
    
    cape_udc_add_n     (values, "raid"         , self->raid);
    cape_udc_add_s_cp  (values, "name"         , description);
    cape_udc_add_n     (values, "max"          , max);
    cape_udc_add_n     (values, "cur"          , 0);

    // execute the query
    self->rsid = adbl_trx_insert (trx, "flow_run_section", &values, err);
    if (0 == self->rsid)
    {
      goto exit_and_cleanup;
    }
  }

  adbl_trx_commit (&trx, err);

exit_and_cleanup:
    
  adbl_trx_rollback (&trx, err);
  
  if (cape_err_code (err))
  {
    
  }
  
  cape_err_del (&err);
}

//-----------------------------------------------------------------------------

void qflow_set (QFlow self, number_t current)
{
  if (self)
  {
    // local objects
    CapeErr err = cape_err_new ();
    
    QFlowCtx ctx = qflow_get_ctx ();
    if (ctx == NULL)
    {
      goto exit_and_cleanup;
    }
    
    {
      CapeUdc params = cape_udc_new (CAPE_UDC_NODE, NULL);
      
      cape_udc_add_n     (params, "id"         , self->rsid);

      adbl_session_atomic_inc (ctx->session, "flow_run_section", &params, "cur", err);
      if (cape_err_code (err))
      {
        goto exit_and_cleanup;
      }
    }
    
exit_and_cleanup:
      
    if (cape_err_code (err))
    {
      
    }
    
    cape_err_del (&err);
  }
}

//-----------------------------------------------------------------------------
