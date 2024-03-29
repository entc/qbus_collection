#include "qbus_logger.h"
#include "qbus_logger_file.h"
#include "qbus_logger_udp.h"

// cape includes
#include "fmt/cape_json.h"
#include "sys/cape_log.h"
#include "sys/cape_dl.h"
#include "sys/cape_file.h"
#include <stc/cape_map.h>
#include <sys/cape_socket.h>
#include <sys/cape_queue.h>

//-----------------------------------------------------------------------------

struct QBusLoggerEntity_s
{
  QbusLogDst dst;
  QbusLogCtx ctx;
  
}; typedef struct QBusLoggerEntity_s* QBusLoggerEntity;

//-----------------------------------------------------------------------------

void qbus_logger_entity_del (QBusLoggerEntity* p_self);

//-----------------------------------------------------------------------------

QBusLoggerEntity qbus_logger_entity__factory__file (CapeAioContext aio, CapeUdc config, CapeErr err)
{
  QBusLoggerEntity self = CAPE_NEW (struct QBusLoggerEntity_s);
  
  self->dst.log_dst_new = qbus_logger_file__dst_new;
  self->dst.log_dst_del = qbus_logger_file__dst_del;
  self->dst.log_dst_msg = qbus_logger_file__dst_msg;
  
  self->ctx = self->dst.log_dst_new (aio, config, err);
  
  // if there was an issue the ctx is NULL
  if (self->ctx == NULL)
  {
    // cleanup
    qbus_logger_entity_del (&self);
  }
  
  return self;
}

//-----------------------------------------------------------------------------

QBusLoggerEntity qbus_logger_entity__factory__udp (CapeAioContext aio, CapeUdc config, CapeErr err)
{
  QBusLoggerEntity self = CAPE_NEW (struct QBusLoggerEntity_s);
  
  self->dst.log_dst_new = qbus_logger_udp__dst_new;
  self->dst.log_dst_del = qbus_logger_udp__dst_del;
  self->dst.log_dst_msg = qbus_logger_udp__dst_msg;

  self->ctx = self->dst.log_dst_new (aio, config, err);

  // if there was an issue the ctx is NULL
  if (self->ctx == NULL)
  {
    // cleanup
    qbus_logger_entity_del (&self);
  }
  
  return self;
}

//-----------------------------------------------------------------------------

void qbus_logger_entity_del (QBusLoggerEntity* p_self)
{
  if (*p_self)
  {
    QBusLoggerEntity self = *p_self;
    
    if (self->dst.log_dst_del)
    {
      self->dst.log_dst_del (&(self->ctx));
    }
    
    CAPE_DEL (p_self, struct QBusLoggerEntity_s);
  }
}

//-----------------------------------------------------------------------------

void qbus_logger_entity_msg (QBusLoggerEntity self, const CapeString remote, const CapeString message)
{
  if (self->dst.log_dst_msg)
  {
    self->dst.log_dst_msg (self->ctx, remote, message);
  }
}

//-----------------------------------------------------------------------------

void __STDCALL qbus_logger__entity__on_del (void* key, void* val)
{
  {
    CapeString h = key; cape_str_del (&h);
  }
  {
    QBusLoggerEntity h = val; qbus_logger_entity_del (&h);
  }
}

//-----------------------------------------------------------------------------

struct QBusLogger_s
{
  CapeMap dest;
  CapeQueue queue;
};

//-----------------------------------------------------------------------------

QBusLogger qbus_logger_new (void)
{
  QBusLogger self = CAPE_NEW(struct QBusLogger_s);
  
  self->dest = cape_map_new (NULL, qbus_logger__entity__on_del, NULL);
  self->queue = cape_queue_new (2000);
  
  return self;
}

//-----------------------------------------------------------------------------

void qbus_logger_del (QBusLogger* p_self)
{
  if (*p_self)
  {
    QBusLogger self = *p_self;
    
    cape_map_del (&(self->dest));
    cape_queue_del (&(self->queue));
    
    CAPE_DEL (p_self, struct QBusLogger_s);
  }
}

//-----------------------------------------------------------------------------

int qbus_logger_init__dest (QBusLogger self, CapeAioContext aio, CapeUdc dest, CapeErr err)
{
  int res;
  CapeUdcCursor* cursor = cape_udc_cursor_new (dest, CAPE_DIRECTION_FORW);
  
  while (cape_udc_cursor_next (cursor))
  {
    const CapeString type = cape_udc_get_s (cursor->item, "type", NULL);
    if (type)
    {
      if (cape_str_equal (type, "udp"))
      {
        QBusLoggerEntity le = qbus_logger_entity__factory__udp (aio, cursor->item, err);
        if (le)
        {
          cape_map_insert (self->dest, (void*)cape_str_cp (cape_udc_name (cursor->item)), (void*)le);
        }
        else
        {
          res = cape_err_code (err);
          goto exit_and_cleanup;
        }
      }
      else if (cape_str_equal (type, "file"))
      {
        QBusLoggerEntity le = qbus_logger_entity__factory__file (aio, cursor->item, err);
        if (le)
        {
          cape_map_insert (self->dest, (void*)cape_str_cp (cape_udc_name (cursor->item)), (void*)le);
        }
        else
        {
          res = cape_err_code (err);
          goto exit_and_cleanup;
        }
      }
    }
  }
  
  res = CAPE_ERR_NONE;
  
exit_and_cleanup:
  
  cape_udc_cursor_del (&cursor);
  return res;
}

//-----------------------------------------------------------------------------

int qbus_logger_init (QBusLogger self, CapeAioContext aio, CapeUdc config, CapeErr err)
{
  int res;
  
  if (config == NULL)
  {
    res = CAPE_ERR_NONE;
    goto exit_and_cleanup;
  }

  // check for destination
  {
    CapeUdc dest = cape_udc_get (config, "dest");
    if (dest)
    {
      res = qbus_logger_init__dest (self, aio, dest, err);
      if (res)
      {
        goto exit_and_cleanup;
      }
    }
  }
  
  // init the queue
  res = cape_queue_start (self->queue, 1, err);

exit_and_cleanup:
  
  return res;
}

//-----------------------------------------------------------------------------

struct QBusLoggerWorkerItem_s
{
  QBusLoggerEntity le;
  
  CapeString remote;
  CapeString message;
  
}; typedef struct QBusLoggerWorkerItem_s* QBusLoggerWorkerItem;

//-----------------------------------------------------------------------------

void __STDCALL qbus_logger_msg__worker (void* ptr, number_t pos, number_t queue_size)
{
  QBusLoggerWorkerItem self = ptr;
  
  qbus_logger_entity_msg (self->le, self->remote, self->message);

  cape_str_del (&(self->remote));
  cape_str_del (&(self->message));

  CAPE_DEL (&self, struct QBusLoggerWorkerItem_s);
}

//-----------------------------------------------------------------------------

void qbus_logger_msg (QBusLogger self, const CapeString remote, const CapeString message)
{
  CapeMapCursor* cursor = cape_map_cursor_create (self->dest, CAPE_DIRECTION_FORW);
  
  while (cape_map_cursor_next (cursor))
  {
    QBusLoggerWorkerItem item = CAPE_NEW (struct QBusLoggerWorkerItem_s);
    
    item->le = cape_map_node_value (cursor->node);
    
    item->remote = cape_str_cp (remote);
    item->message = cape_str_cp (message);
    
    cape_queue_add (self->queue, NULL, qbus_logger_msg__worker, NULL, NULL, item, 0);
  }
  
  cape_map_cursor_destroy (&cursor);
}

//-----------------------------------------------------------------------------
