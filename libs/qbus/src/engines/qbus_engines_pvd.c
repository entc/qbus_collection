#include "qbus_engines_pvd.h"
#include "qbus_pvd.h"

// cape includes
#include <sys/cape_log.h>
#include <sys/cape_dl.h>
#include <sys/cape_file.h>
#include <fmt/cape_json.h>
#include <stc/cape_list.h>

//-----------------------------------------------------------------------------

struct QBusEnginesPvd_s
{
  CapeDl hlib;         // handle for the shared library
  QBusPvd2 pvd2;       // function pointers
  QBusPvdCtx ctx;      // library context
};

//-----------------------------------------------------------------------------

QBusEnginesPvd qbus_engines_pvd_new ()
{
  QBusEnginesPvd self = CAPE_NEW (struct QBusEnginesPvd_s);

  self->hlib = cape_dl_new ();
  self->ctx = NULL;
  
  memset (&(self->pvd2), 0, sizeof(QBusPvd2));
  
  return self;
}

//-----------------------------------------------------------------------------

void qbus_engines_pvd_del (QBusEnginesPvd* p_self)
{
  if (*p_self)
  {
    QBusEnginesPvd self = *p_self;
    
    cape_log_msg (CAPE_LL_TRACE, "QBUS", "engines", "release engine");
        
    if (self->pvd2.ctx_del)
    {
      self->pvd2.ctx_del (&(self->ctx));
    }
    
    cape_dl_del (&(self->hlib));
    
    CAPE_DEL (p_self, struct QBusEnginesPvd_s);
  }
}

//-----------------------------------------------------------------------------

void __STDCALL qbus_engines_pvd__on_connect (void* user_ptr)
{
  
  cape_log_fmt (CAPE_LL_DEBUG, "QBUS", "entity", "new entity connection");
  
}

//-----------------------------------------------------------------------------

void __STDCALL qbus_engines_pvd__on_disconnect (void* user_ptr)
{
  
  cape_log_fmt (CAPE_LL_DEBUG, "QBUS", "entity", "lost connection");
  
}

//-----------------------------------------------------------------------------

QBusPvdFcts* __STDCALL qbus_engines_pvd__fcts_new (void* factory_ptr)
{
  QBusPvdFcts* fcts = CAPE_NEW (QBusPvdFcts);
  
  fcts->on_connect = qbus_engines_pvd__on_connect;
  fcts->on_disconnect = qbus_engines_pvd__on_disconnect;
  fcts->user_ptr = NULL;
  
  return fcts;
}

//-----------------------------------------------------------------------------

void __STDCALL qbus_engines_pvd__fcts_del (QBusPvdFcts** p_self)
{
  if (*p_self)
  {
    
   
    CAPE_DEL (p_self, QBusPvdFcts);
  }
}

//-----------------------------------------------------------------------------

int qbus_engines_pvd_load (QBusEnginesPvd self, const CapeString path, const CapeString name, CapeAioContext aio_context, CapeErr err)
{
  int res;
  
  CapeString path_current = NULL;
  CapeString path_resolved = NULL;
  CapeString pvd_name = NULL;
  
  // fetch the current path
  path_current = cape_fs_path_current (path);
  if (path_current == NULL)
  {
    res = cape_err_set_fmt (err, CAPE_ERR_NOT_FOUND, "can't find path: %s", path_current);
    goto exit_and_cleanup;
  }
  
  path_resolved = cape_fs_path_resolve (path_current, err);
  if (path_resolved == NULL)
  {
    res = cape_err_set_fmt (err, CAPE_ERR_NOT_FOUND, "can't find path: %s", path_current);
    goto exit_and_cleanup;
  }
  
  pvd_name = cape_str_catenate_2 ("qbus_pvd2_", name);
  
  cape_log_fmt (CAPE_LL_TRACE, "QBUS", "engine", "try to load engine = %s, library = %s", name, pvd_name);
  
  // try to load the module
  res = cape_dl_load (self->hlib, path_resolved, pvd_name, err);
  if (res)
  {
    goto exit_and_cleanup;
  }

  self->pvd2.ctx_new = cape_dl_funct (self->hlib, "pvd2_ctx_new", err);
  if (self->pvd2.ctx_new == NULL)
  {
    res = cape_err_code (err);
    goto exit_and_cleanup;
  }
  
  self->pvd2.ctx_del = cape_dl_funct (self->hlib, "pvd2_ctx_del", err);
  if (self->pvd2.ctx_del == NULL)
  {
    res = cape_err_code (err);
    goto exit_and_cleanup;
  }
  
  self->pvd2.ctx_reg = cape_dl_funct (self->hlib, "pvd2_ctx_reg", err);
  if (self->pvd2.ctx_reg == NULL)
  {
    res = cape_err_code (err);
    goto exit_and_cleanup;
  }
  
  self->ctx = self->pvd2.ctx_new (aio_context, qbus_engines_pvd__fcts_new, qbus_engines_pvd__fcts_del, self, err);
  
  res = CAPE_ERR_NONE;

exit_and_cleanup:
  
  cape_str_del (&path_current);
  cape_str_del (&path_resolved);
  cape_str_del (&pvd_name);

  return res;
}

//-----------------------------------------------------------------------------

int qbus_engines_pvd__entity_new (QBusEnginesPvd self, const CapeUdc config, CapeErr err)
{
  self->pvd2.ctx_reg (self->ctx, config);
  
  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------
