#include "qbus_obsvbl.h"
#include "qbus_engines.h"

//-----------------------------------------------------------------------------

struct QBusObsvbl_s
{
  QBusEngines engines;        // reference
  
  
};

//-----------------------------------------------------------------------------

QBusObsvbl qbus_obsvbl_new (QBusEngines engines)
{
  QBusObsvbl self = CAPE_NEW (struct QBusObsvbl_s);
  
  self->engines = engines;
  
  return self;
}

//-----------------------------------------------------------------------------

void qbus_obsvbl_del (QBusObsvbl* p_self)
{
  if (*p_self)
  {
    
    
    CAPE_DEL(p_self, struct QBusObsvbl_s);    
  }
}

//-----------------------------------------------------------------------------

void qbus_obsvbl_add_nodes (QBusObsvbl self, const CapeString module_name, const CapeString module_uuid, QBusPvdConnection conn, CapeUdc* p_nodes)
{
  
  
}

//-----------------------------------------------------------------------------

void qbus_obsvbl_send_update (QBusObsvbl self, QBusPvdConnection conn_ex, QBusPvdConnection conn_di)
{
  CapeList user_ptrs = cape_list_new (NULL);
  
  if (conn_di)
  {
    cape_list_push_back (user_ptrs, conn_di);
  }
  else
  {
    
  }

  if (cape_list_size (user_ptrs) > 0)
  {
    QBusFrame frame = qbus_frame_new ();
    
    // CH01: replace self->name with self->uuid and add name as module
    qbus_frame_set (frame, QBUS_FRAME_TYPE_OBSVBL_UPD, NULL, NULL, NULL, NULL);
    

    
    // send the frame
    qbus_engines__broadcast (self->engines, frame, user_ptrs);
    
    qbus_frame_del (&frame);
  }
  
  cape_list_del (&user_ptrs);
}

//-----------------------------------------------------------------------------
