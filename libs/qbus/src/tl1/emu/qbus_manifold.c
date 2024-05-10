#include "qbus_tl1.h" 

// cape includes
#include <stc/cape_map.h>
#include <sys/cape_mutex.h>

//-----------------------------------------------------------------------------

struct QBusManifoldSubscriber_s
{
  CapeString uuid;
  CapeString name;
    
}; typedef struct QBusManifoldSubscriber_s* QBusManifoldSubscriber;

//-----------------------------------------------------------------------------

QBusManifoldSubscriber qbus_manifold_subscriber_new (const CapeString uuid, const CapeString name)
{
  QBusManifoldSubscriber self = CAPE_NEW (struct QBusManifoldSubscriber_s);
  
  self->uuid = cape_str_cp (uuid);
  self->name = cape_str_cp (name);
    
  return self;
}

//-----------------------------------------------------------------------------

void qbus_manifold_subscriber_del (QBusManifoldSubscriber* p_self)
{
  if (*p_self)
  {
    QBusManifoldSubscriber self = *p_self;
    
    cape_str_del (&(self->uuid));
    cape_str_del (&(self->name));
    
    CAPE_DEL (p_self, struct QBusManifoldSubscriber_s);
  }
}

//-----------------------------------------------------------------------------

struct QBusManifoldMember_s
{
  CapeString name;
  
  void* user_ptr;
  
  fct_qbus_manifold__on_add on_add;
  fct_qbus_manifold__on_rm on_rm;
  fct_qbus_manifold__on_call on_call;
  fct_qbus_manifold__on_recv on_recv;
  fct_qbus_manifold__on_emit on_emit;
  
  QBusManifoldSubscriber sub;
  
}; typedef struct QBusManifoldMember_s* QBusManifoldMember;

//-----------------------------------------------------------------------------

QBusManifoldMember qbus_manifold_member_new (const CapeString name, void* user_ptr, fct_qbus_manifold__on_add on_add, fct_qbus_manifold__on_rm on_rm, fct_qbus_manifold__on_call on_call,fct_qbus_manifold__on_recv on_recv, fct_qbus_manifold__on_emit on_emit)
{
  QBusManifoldMember self = CAPE_NEW (struct QBusManifoldMember_s);
  
  self->name = cape_str_cp (name);
  
  self->user_ptr = user_ptr;
  
  self->on_call = on_call;
  self->on_emit = on_emit;
  
  self->on_add = on_add;
  self->on_rm = on_rm;
  
  return self;
}

//-----------------------------------------------------------------------------

void qbus_manifold_member_del (QBusManifoldMember* p_self)
{
  if (*p_self)
  {
    QBusManifoldMember self = *p_self;
    
    cape_str_del (&(self->name));
    
    CAPE_DEL (p_self, struct QBusManifoldMember_s);
  }
}

//-----------------------------------------------------------------------------

void qbus_manifold_member_add (QBusManifoldMember self, const char* uuid, const char* module, void* node)
{
  if (self->on_add)
  {
    self->on_add (self->user_ptr, uuid, module, node);
  }
}

//-----------------------------------------------------------------------------

int qbus_manifold_member_call (QBusManifoldMember self, const CapeString method_name, QBusMethod* p_qbus_method, QBusM msg, CapeErr err)
{
  if (self->on_call)
  {
    self->on_call (self->user_ptr, method_name, p_qbus_method, msg);
  }
  
  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

struct QBusManifold_s
{
  CapeMap members;
  CapeMap chains;
  CapeMutex chains_mutex;
  CapeMap subs;
  CapeMutex subs_mutex;
};

//-----------------------------------------------------------------------------

void __STDCALL qbus_manifold__members__on_del (void* key, void* val)
{
  {
    CapeString h = key; cape_str_del (&h);
  }
  {
    QBusManifoldMember h = val; qbus_manifold_member_del (&h);
  }
}

//-----------------------------------------------------------------------------

void __STDCALL qbus_manifold__chains__on_del (void* key, void* val)
{
  {
    CapeString h = key; cape_str_del (&h);
  }
  {
    QBusMethod method = val; qbus_method_del (&method);
  }
}

//-----------------------------------------------------------------------------

void __STDCALL qbus_manifold__subs__on_del (void* key, void* val)
{
  {
    CapeString h = key; cape_str_del (&h);
  }
  {
    QBusManifoldSubscriber h = val; qbus_manifold_subscriber_del (&h);
  }
}

//-----------------------------------------------------------------------------

QBusManifold qbus_manifold_new ()
{
  QBusManifold self = CAPE_NEW (struct QBusManifold_s);

  self->members = cape_map_new (cape_map__compare__s, qbus_manifold__members__on_del, NULL);
  self->chains = cape_map_new (cape_map__compare__s, qbus_manifold__chains__on_del, NULL);
  self->subs = cape_map_new (cape_map__compare__s, qbus_manifold__subs__on_del, NULL);

  self->chains_mutex = cape_mutex_new ();
  self->subs_mutex = cape_mutex_new ();
  
  return self;
}

//-----------------------------------------------------------------------------

void qbus_manifold_del (QBusManifold* p_self)
{
  if (*p_self)
  {
    QBusManifold self = *p_self;
    
    cape_mutex_del (&(self->subs_mutex));
    cape_mutex_del (&(self->chains_mutex));
    
    cape_map_del (&(self->chains));
    cape_map_del (&(self->members));
    cape_map_del (&(self->subs));

    CAPE_DEL (p_self, struct QBusManifold_s);
  }
}

//-----------------------------------------------------------------------------

int qbus_manifold_init (QBusManifold self, const CapeString uuid, const CapeString name, void* user_ptr, fct_qbus_manifold__on_add on_add, fct_qbus_manifold__on_rm on_rm, fct_qbus_manifold__on_call on_call, fct_qbus_manifold__on_recv on_recv, fct_qbus_manifold__on_emit on_emit, CapeErr err)
{
  printf ("manifold init uuid = %s\n", uuid);
  
  QBusManifoldMember member;
  
  CapeMapNode n = cape_map_find (self->members, (void*)uuid);
  
  if (n)
  {
    return cape_err_set (err, CAPE_ERR_EOF, "already set");
  }

  member = qbus_manifold_member_new (name, user_ptr, on_add, on_rm, on_call, on_recv, on_emit);
  
  n = cape_map_insert (self->members, (void*)cape_str_cp (uuid), member);

  // tell all others the new 'connection'
  {
    CapeMapCursor* cursor = cape_map_cursor_new (self->members, CAPE_DIRECTION_FORW);
    
    while (cape_map_cursor_next (cursor))
    {
      if (cursor->node != n)
      {
        QBusManifoldMember m = cape_map_node_value (cursor->node);

        // tell the other 'module' the new 'module'
        qbus_manifold_member_add (m, uuid, name, n);
        
        // tell the new 'module' having this 'module'
        qbus_manifold_member_add (member, cape_map_node_key (cursor->node), m->name, cursor->node);
      }
    }
    
    cape_map_cursor_del (&cursor);
  }
  
  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

QBusManifoldMember qbus_manifold_member (QBusManifold self, const CapeString module_ident)
{
  QBusManifoldMember ret = NULL;

  // try to find the member
  CapeMapNode n = cape_map_find (self->members, (void*)module_ident);
  if (n)
  {
    ret = cape_map_node_value (n);
  }
  
  return ret;
}

//-----------------------------------------------------------------------------

void qbus_manifold_emit (QBusManifold self, const CapeString val_name, const CapeString val_data)
{
  // check if we find subscriptors
  cape_mutex_lock (self->subs_mutex);

  {
    CapeMapNode n = cape_map_find (self->subs, (void*)val_name);
    if (n)
    {
      
    }
  }
  
  cape_mutex_unlock (self->subs_mutex);
}

//-----------------------------------------------------------------------------

void qbus_manifold_subscribe (QBusManifold self, const CapeString uuid, const CapeString module_uuid, const CapeString module_name, const CapeString value_name)
{
  QBusManifoldMember member = qbus_manifold_member (self, uuid);
  
  if (member)
  {
    cape_mutex_lock (self->subs_mutex);

    {
      CapeMapNode n = cape_map_find (self->subs, (void*)value_name);
      if (NULL == n)
      {
        printf ("[subscriber] -> add, ident = %s, name = %s, value = %s\n", module_uuid, module_name, value_name);

        cape_map_insert (self->subs, (void*)cape_str_cp (value_name), (void*)qbus_manifold_subscriber_new (member, module_uuid, module_name));
      }
    }
    
    cape_mutex_unlock (self->subs_mutex);
  }
}

//-----------------------------------------------------------------------------

QBusMethod qbus_manifold_response__pop (QBusManifold self, const CapeString method_ident)
{
  QBusMethod ret = NULL;
  
  cape_mutex_lock (self->chains_mutex);
  
  CapeMapNode n2 = cape_map_find (self->chains, (void*)method_ident);
  if (n2)
  {
    ret = cape_map_node_value (n2);
    
    cape_map_node_set (n2, (void*)NULL);
    
    cape_map_erase (self->chains, n2);
  }

  cape_mutex_unlock (self->chains_mutex);
  
  return ret;
}

//-----------------------------------------------------------------------------

void qbus_manifold_response (QBusManifold self, QBusM msg)
{
  QBusManifoldMember member = qbus_manifold_member (self, msg->module_ident);

  // TODO check for p_node
  printf ("response -> {%s} CK[%s]\n", msg->module_ident, msg->method_ident);

  if (member)
  {
    printf ("found node\n");

    QBusMethod method = qbus_manifold_response__pop (self, msg->method_ident);

    if (method)
    {
      CapeErr err = cape_err_new ();
      
      cape_str_del (&(msg->module_ident));
      cape_str_del (&(msg->method_ident));
      
      int res = qbus_manifold_member_call (member, NULL, &method, msg, err);
      
      cape_err_del (&err);
      qbus_method_del (&method);
    }
  }
}

//-----------------------------------------------------------------------------

int qbus_manifold_send (QBusManifold self, const CapeString module_ident, void** p_node, const CapeString method_name, QBusM msg, QBusMethod* p_qbus_method, CapeErr err)
{
  CapeMapNode n = *p_node;
  
  cape_str_del (&(msg->method_ident));
  msg->method_ident = cape_str_uuid ();

  cape_str_del (&(msg->module_ident));
  msg->module_ident = cape_str_cp (module_ident);
  
  cape_mutex_lock (self->chains_mutex);

  cape_map_insert (self->chains, (void*)cape_str_cp (msg->method_ident), (void*)*p_qbus_method);
  *p_qbus_method = NULL;

  cape_mutex_unlock (self->chains_mutex);

  // shortcut of sending it to the peer
  {
    QBusManifoldMember m = cape_map_node_value (n);
    
    printf ("send {%s} -> %s {%s} -> %s CK[%s]\n", module_ident, m->name, cape_map_node_key (n), method_name, msg->method_ident);
    
    return qbus_manifold_member_call (m, method_name, NULL, msg, err);
  }
}

//-----------------------------------------------------------------------------
