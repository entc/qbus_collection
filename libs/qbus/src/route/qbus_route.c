#include "qbus_route.h"

// cape includes
#include <sys/cape_file.h>
#include <sys/cape_log.h>
#include <sys/cape_thread.h>
#include <fmt/cape_args.h>
#include <fmt/cape_json.h>
#include <fmt/cape_tokenizer.h>
#include <stc/cape_map.h>

//-----------------------------------------------------------------------------

struct QBusRoute_s
{
  CapeString uuid;            // the unique id of this module
  CapeString name;            // the name of the module (multiple modules might have the same name)

  CapeMap nodes;
};

//-----------------------------------------------------------------------------

void __STDCALL qbus_route__nodes__on_del (void* key, void* val)
{
  {
    CapeString h = key; cape_str_del (&h);
  }
  {
    
  }
}

//-----------------------------------------------------------------------------

QBusRoute qbus_route_new (const CapeString name)
{
  QBusRoute self = CAPE_NEW (struct QBusRoute_s);

  self->uuid = cape_str_uuid ();
  self->name = cape_str_cp (name);
  
  self->nodes = cape_map_new (NULL, qbus_route__nodes__on_del, NULL);
  
  return self;
}

//-----------------------------------------------------------------------------

void qbus_route_del (QBusRoute* p_self)
{
  if (*p_self)
  {
    QBusRoute self = *p_self;
    
    cape_map_del (&(self->nodes));
    
    cape_str_del (&(self->name));
    cape_str_del (&(self->uuid));
    
    CAPE_DEL (p_self, struct QBusRoute_s);
  }
}

//-----------------------------------------------------------------------------

const CapeString qbus_route_uuid_get (QBusRoute self)
{
  return self->uuid;
}

//-----------------------------------------------------------------------------

const CapeString qbus_route_name_get (QBusRoute self)
{
  return self->name;
}

//-----------------------------------------------------------------------------

CapeUdc qbus_route_node_get (QBusRoute self)
{
  CapeMapCursor* cursor = cape_map_cursor_create (self->nodes, CAPE_DIRECTION_FORW);
  
  while (cape_map_cursor_next (cursor))
  {
    
  }
  
  cape_map_cursor_destroy (&cursor);

  return NULL;
}

//-----------------------------------------------------------------------------

void qbus_route_add (QBusRoute self, const CapeString module_name, const CapeString module_uuid, void* user_ptr, CapeUdc* p_nodes)
{
  
  
  
  // tell the others the new nodes
  //  qbus_route_send_updates (self, conn);
}

//-----------------------------------------------------------------------------
