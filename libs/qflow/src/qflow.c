#include "qflow.h"

// cape includes
#include <stc/cape_map.h>
#include <sys/cape_types.h>

//-----------------------------------------------------------------------------

struct QFlow_s
{
  AdblSession session;    // reference
  
  number_t raid;
  number_t rsid;
};

//-----------------------------------------------------------------------------

QFlow qflow_new (number_t raid, AdblSession session)
{
  QFlow self = CAPE_NEW (struct QFlow_s);

  self->session = session;
  
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
  
  trx = adbl_trx_new (self->session, err);
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

void qflow_set (QFlow self)
{
  if (self)
  {
    // local objects
    CapeErr err = cape_err_new ();
    
    {
      CapeUdc params = cape_udc_new (CAPE_UDC_NODE, NULL);
      
      cape_udc_add_n     (params, "id"         , self->rsid);

      adbl_session_atomic_inc (self->session, "flow_run_section", &params, "cur", err);
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
