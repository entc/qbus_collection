#include "qbus_engines.h"
#include "qbus_engines_pvd.h"
#include "qbus_pvd.h"

// cape includes
#include <sys/cape_log.h>
#include <sys/cape_file.h>
#include <fmt/cape_json.h>
#include <stc/cape_map.h>

//-----------------------------------------------------------------------------

struct QBusEngines_s
{
  CapeMap engines;
  CapeString path;
};

//-----------------------------------------------------------------------------

void __STDCALL qbus_engines__on_del (void* key, void* val)
{
  {
    CapeString h = key; cape_str_del (&h);
  }
  {
    QBusEnginesPvd h = val; qbus_engines_pvd_del (&h);
  }
}

//-----------------------------------------------------------------------------

QBusEngines qbus_engines_new ()
{
  QBusEngines self = CAPE_NEW (struct QBusEngines_s);

  self->engines = cape_map_new (NULL, qbus_engines__on_del, NULL);
  self->path = cape_str_cp ("qbus");
  
  return self;
}

//-----------------------------------------------------------------------------

void qbus_engines_del (QBusEngines* p_self)
{
  if (*p_self)
  {
    QBusEngines self = *p_self;
    
    cape_map_del (&(self->engines));
    cape_str_del (&(self->path));
    
    CAPE_DEL (p_self, struct QBusEngines_s);
  }
}

//-----------------------------------------------------------------------------

QBusEnginesPvd qbus_engines__init_ctx (QBusEngines self, const CapeString name, CapeAioContext aio_context, QBusRoute route, QBusObsvbl obsvbl, QBusMethods methods, CapeErr err)
{
  QBusEnginesPvd ret = NULL;
  
  CapeMapNode n = cape_map_find (self->engines, (void*)name);
  if (n)
  {
    // context was already initialized
    ret = cape_map_node_value (n);
  }
  else
  {
    ret = qbus_engines_pvd_new (route, obsvbl, methods);
    
    // try to load the engine
    // -> returns a cape error code
    if (qbus_engines_pvd_load (ret, self->path, name, aio_context, err))
    {
      // cleanup
      qbus_engines_pvd_del (&ret);
    }
    else
    {
      // append the engine to the engines map
      cape_map_insert (self->engines, (void*)cape_str_cp (name), ret);
    }
  }
  
  return ret;
}

//-----------------------------------------------------------------------------

int qbus_engines__init_pvds__entity (QBusEngines self, const CapeUdc context, CapeAioContext aio_context, QBusRoute route, QBusObsvbl obsvbl, QBusMethods methods, CapeErr err)
{
  int res;
  
  const CapeString type_name;
  QBusEnginesPvd engine;
  
  // get type of the engine
  type_name = cape_udc_get_s (context, "type", NULL);
  if (type_name == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_RUNTIME, "no pvd engine was specified");
    goto exit_and_cleanup;
  }
  
  engine = qbus_engines__init_ctx (self, type_name, aio_context, route, obsvbl, methods, err);
  if (engine == NULL)
  {
    res = cape_err_code (err);
    goto exit_and_cleanup;
  }
  
  // already create a new connection entity
  res = qbus_engines_pvd__entity_new (engine, context, err);

exit_and_cleanup:
  
  return res;
}

//-----------------------------------------------------------------------------

int qbus_engines__init_pvds (QBusEngines self, const CapeUdc pvds, CapeAioContext aio_context, QBusRoute route, QBusObsvbl obsvbl, QBusMethods methods, CapeErr err)
{
  int res;
  
  // local objects
  CapeUdcCursor* cursor = cape_udc_cursor_new (pvds, CAPE_DIRECTION_FORW);
  
  while (cape_udc_cursor_next (cursor))
  {
    res = qbus_engines__init_pvds__entity (self, cursor->item, aio_context, route, obsvbl, methods, err);
    if (res)
    {
      cape_log_msg (CAPE_LL_ERROR, "QBUS", "init pvds", cape_err_text (err));
      goto exit_and_cleanup;
    }
  }

  res = CAPE_ERR_NONE;
  
exit_and_cleanup:
  
  cape_udc_cursor_del (&cursor);
  return res;
}

//-----------------------------------------------------------------------------

void qbus_engines__send (QBusEngines self, QBusFrame frame, QBusPvdConnection conn)
{
  qbus_engines_pvd_send (conn->engine, frame, conn->connection_ptr);    
}

//-----------------------------------------------------------------------------

void qbus_engines__broadcast (QBusEngines self, QBusFrame frame, CapeList user_ptrs)
{
  CapeListCursor* cursor = cape_list_cursor_create (user_ptrs, CAPE_DIRECTION_FORW);
  
  while (cape_list_cursor_next (cursor))
  {
    QBusPvdConnection conn = cape_list_node_data (cursor->node);
    
    qbus_engines__send (self, frame, conn);    
  }
  
  cape_list_cursor_destroy (&cursor);
}

//-----------------------------------------------------------------------------
