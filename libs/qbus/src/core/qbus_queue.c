#include "qbus_queue.h"

// cape includes
#include <sys/cape_log.h>
#include <sys/cape_queue.h>
#include <fmt/cape_json.h>

//-----------------------------------------------------------------------------

struct QBusQueue_s
{
  CapeQueue queue;
  

};

//-----------------------------------------------------------------------------

QBusQueue qbus_queue_new ()
{
  QBusQueue self = CAPE_NEW (struct QBusQueue_s);
  
  self->queue = cape_queue_new (5000);
  
  return self;
}

//-----------------------------------------------------------------------------

void qbus_queue_del (QBusQueue* p_self)
{
  if (*p_self)
  {
    QBusQueue self = *p_self;
    
    cape_queue_del (&(self->queue));
    
    CAPE_DEL (p_self, struct QBusQueue_s);
  }
}

//-----------------------------------------------------------------------------

int qbus_queue_init (QBusQueue self, number_t threads, CapeErr err)
{
  return cape_queue_start (self->queue, threads, err);
}

//-----------------------------------------------------------------------------
