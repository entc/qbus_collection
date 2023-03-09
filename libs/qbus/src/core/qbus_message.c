#include "qbus_message.h"

// cape includes
#include <sys/cape_log.h>
#include <fmt/cape_json.h>

#include <qcrypt.h>

//-----------------------------------------------------------------------------

QBusM qbus_message_new (const CapeString key, const CapeString sender)
{
  QBusM self = CAPE_NEW (struct QBusMessage_s);
  
  if (key)
  {
    // clone the key
    self->chain_key = cape_str_cp (key);
  }
  else
  {
    // create a new key
    self->chain_key = cape_str_uuid ();    
  }
  
  self->sender = cape_str_cp (sender);
  
  // init the objects
  self->cdata = NULL;
  self->pdata = NULL;
  self->clist = NULL;
  self->rinfo = NULL;
  self->files = NULL;
  self->blob = NULL;
  
  self->err = NULL;
  
  self->mtype = QBUS_MTYPE_NONE;
  
  return self;
}

//-----------------------------------------------------------------------------

void qbus_message_clr (QBusM self, u_t cdata_udc_type)
{
  cape_udc_del (&(self->cdata));
  cape_udc_del (&(self->pdata));
  cape_udc_del (&(self->clist));
  cape_stream_del (&(self->blob));
  
  cape_err_del (&(self->err));
  
  if (cdata_udc_type != CAPE_UDC_UNDEFINED)
  {
    // we don't need a name
    self->cdata = cape_udc_new (cdata_udc_type, NULL);
  }
}

//-----------------------------------------------------------------------------

void qbus_message_del (QBusM* p_self)
{
  if (*p_self)
  {
    QBusM self = *p_self;
    
    qbus_message_clr (self, CAPE_UDC_UNDEFINED);
    
    // only clear it here
    cape_udc_del (&(self->rinfo));
    cape_udc_del (&(self->files));
    
    cape_str_del (&(self->chain_key));
    cape_str_del (&(self->sender));
    
    CAPE_DEL (p_self, struct QBusMessage_s);
  }
}

//-----------------------------------------------------------------------------

QBusM qbus_message_data_mv (QBusM source)
{
  QBusM self = CAPE_NEW (struct QBusMessage_s);
  
  self->chain_key = cape_str_cp (source->chain_key);
  self->sender = cape_str_cp (source->sender);
  
  // move the objects
  self->cdata = cape_udc_mv (&(source->cdata));
  self->pdata = cape_udc_mv (&(source->pdata));
  self->clist = cape_udc_mv (&(source->clist));
  self->rinfo = cape_udc_mv (&(source->rinfo));
  self->files = cape_udc_mv (&(source->files));
  
  if (source->blob)
  {
    self->blob = source->blob;
    source->blob = NULL;
  }
  
  self->err = NULL;
  self->mtype = source->mtype;
  
  return self;
}

//-----------------------------------------------------------------------------

int qbus_message_role_has (QBusM self, const CapeString role_name)
{
  CapeUdc roles;
  CapeUdc role;
  
  if (self->rinfo == NULL)
  {
    return FALSE;
  }
  
  roles = cape_udc_get (self->rinfo, "roles");
  if (roles == NULL)
  {
    return FALSE;
  }
  
  role = cape_udc_get (roles, role_name);
  if (role == NULL)
  {
    return FALSE;
  }
  
  return TRUE;
}

//-----------------------------------------------------------------------------

int qbus_message_role_or2 (QBusM self, const CapeString role01, const CapeString role02)
{
  CapeUdc roles;
  CapeUdc role;
  
  if (self->rinfo == NULL)
  {
    cape_log_msg (CAPE_LL_WARN, "QBUS", "role or2", "rinfo is NULL");
    return FALSE;
  }
  
  roles = cape_udc_get (self->rinfo, "roles");
  if (roles == NULL)
  {
    return FALSE;
  }
  
  role = cape_udc_get (roles, role01);
  if (role == NULL)
  {
    role = cape_udc_get (roles, role02);
    if (role == NULL)
    {
      return FALSE;
    }
  }
  
  return TRUE;
}

//-----------------------------------------------------------------------------

QBusM qbus_message_frame (QBusFrame self)
{
  QBusM qin = qbus_message_new (self->chain_key, self->sender);
  
  qin->mtype = self->msg_type;
  
  switch (self->msg_type)
  {
    case QBUS_MTYPE_JSON:
    case QBUS_MTYPE_FILE:
    {
      if (self->msg_size)
      {
        // convert from raw data into json data structure
        CapeUdc payload = cape_json_from_buf (self->msg_data, self->msg_size, qcrypt__stream_base64_decode);
        if (payload)
        {
          // extract all substructures from the payload
          qin->clist = cape_udc_ext_list (payload, "L");
          qin->cdata = cape_udc_ext (payload, "D");
          qin->pdata = cape_udc_ext (payload, "P");
          qin->rinfo = cape_udc_ext (payload, "I");
          qin->files = cape_udc_ext (payload, "F");
          
          // check for errors
          {
            number_t err_code = cape_udc_get_n (payload, "err_code", 0);
            if (err_code)
            {
              // create a new error object
              qin->err = cape_err_new ();
              
              // set the error
              cape_err_set (qin->err, err_code, cape_udc_get_s (payload, "err_text", "no error text"));
            }
          }
        }
        else
        {
          cape_log_fmt (CAPE_LL_ERROR, "QBUS", "frame qin", "can't parse JSON [%lu]", self->msg_size);
          
          printf ("CAN'T PARSE JSON '%s'\n", self->msg_data);
        }
        
        cape_udc_del (&payload);
      }
      
      break;
    }
  }
  
  return qin;
}

//-----------------------------------------------------------------------------

CapeUdc qbus_frame_set_qmsg (QBusFrame self, QBusM qmsg, CapeErr err)
{
  CapeUdc payload = cape_udc_new (CAPE_UDC_NODE, NULL);
  
  if (qmsg->clist)
  {
    cape_udc_add_name (payload, &(qmsg->clist), "L");
  }
  
  if (qmsg->cdata)
  {
    cape_udc_add_name (payload, &(qmsg->cdata), "D");
  }
  
  if (qmsg->pdata)
  {
    cape_udc_add_name (payload, &(qmsg->pdata), "P");
  }
  
  if (qmsg->rinfo)
  {
    cape_udc_add_name (payload, &(qmsg->rinfo), "I");
  }
  
  if (qmsg->files)
  {
    cape_udc_add_name (payload, &(qmsg->files), "F");
  }
  
  if (err)
  {
    number_t err_code = cape_err_code (err);
    if (err_code)
    {
      cape_log_fmt (CAPE_LL_TRACE, "QBUS", "frame set", "{%i} -- set err -- %s", cape_err_code (err), cape_err_text (err));
      
      cape_udc_add_s_cp (payload, "err_text", cape_err_text (err));
      cape_udc_add_n (payload, "err_code", cape_err_code (err));
    }
  }
  
  // correct mtype
  if (qmsg->mtype == QBUS_MTYPE_NONE)
  {
    qmsg->mtype = QBUS_MTYPE_JSON;
  }
  
  return qbus_frame_set_udc (self, qmsg->mtype, &payload);
}

//-----------------------------------------------------------------------------
