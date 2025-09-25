#include "qbus_con.h"

// cape includes
#include <stc/cape_str.h>
#include <stc/cape_map.h>
#include <sys/cape_log.h>
#include <fmt/cape_json.h>

// qcrypt includes
#include <qcrypt.h>

//-----------------------------------------------------------------------------

struct QBusCon_s
{
  QbusPvdCtx engine_context;
  QbusPvdConnection con;

  QBusEngine engine;           // reference
  QBusRouter router;           // reference
  QBusMethods methods;         // reference
  
  CapeString module;
};

//-----------------------------------------------------------------------------

QBusCon qbus_con_new (QBusRouter router, QBusMethods methods, const CapeString module_name)
{
  QBusCon self = CAPE_NEW (struct QBusCon_s);
  
  self->engine_context = NULL;
  self->con = NULL;

  self->engine = NULL;
  self->router = router;
  self->methods = methods;
  
  self->module = cape_str_cp (module_name);

  cape_log_fmt (CAPE_LL_TRACE, "QBUS", "con new", "create new connection hub as name = '%s'", self->module);
  
  return self;
}

//-----------------------------------------------------------------------------

void qbus_con_del (QBusCon* p_self)
{
  if (*p_self)
  {
    QBusCon self = *p_self;
    
    cape_str_del (&(self->module));
    
    CAPE_DEL (p_self, struct QBusCon_s);
  }
}

//-----------------------------------------------------------------------------

QBusFrame qbus_con__frame_from_qin (QBusM msg)
{
  QBusFrame frame = qbus_frame_new ();
  
  // local objects
  CapeUdc payload = cape_udc_new (CAPE_UDC_NODE, NULL);
  
  if (msg->clist)
  {
    cape_udc_add_name (payload, &(msg->clist), "L");
  }
  
  if (msg->cdata)
  {
    cape_udc_add_name (payload, &(msg->cdata), "D");
  }
  
  if (msg->pdata)
  {
    cape_udc_add_name (payload, &(msg->pdata), "P");
  }
  
  if (msg->rinfo)
  {
    cape_udc_add_name (payload, &(msg->rinfo), "I");
  }
  
  if (msg->files)
  {
    cape_udc_add_name (payload, &(msg->files), "F");
  }
  
  if (msg->err)
  {
    number_t err_code = cape_err_code (msg->err);
    if (err_code)
    {
      cape_log_fmt (CAPE_LL_TRACE, "QBUS", "frame set", "{%i} -- set err -- %s", cape_err_code (msg->err), cape_err_text (msg->err));
      
      cape_udc_add_s_cp (payload, "err_text", cape_err_text (msg->err));
      cape_udc_add_n (payload, "err_code", cape_err_code (msg->err));
    }
  }
  
  {
    frame->msg_data = cape_json_to_s__ex (payload, qcrypt__stream_base64_encode);
    frame->msg_size = cape_str_size (frame->msg_data);
    frame->msg_type = QBUS_MTYPE_JSON;
  }

  cape_udc_del (&payload);

  return frame;
}

//-----------------------------------------------------------------------------

QBusM qbus_con__qin_from_frame (QBusFrame frame)
{
  QBusM qin = qbus_message_new (frame->chain_key, frame->sender);
    
  qin->mtype = frame->msg_type;
  
  switch (frame->msg_type)
  {
    case QBUS_MTYPE_JSON:
    case QBUS_MTYPE_FILE:
    {
      if (frame->msg_size)
      {
        // convert from raw data into json data structure
        CapeUdc payload = cape_json_from_buf (frame->msg_data, frame->msg_size, qcrypt__stream_base64_decode);
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
              cape_err_set (qin->err, (int)err_code, cape_udc_get_s (payload, "err_text", "no error text"));
            }
          }
        }
        else
        {
          cape_log_fmt (CAPE_LL_ERROR, "QBUS", "frame qin", "can't parse JSON [%lu]", frame->msg_size);

          printf ("CAN'T PARSE JSON '%s'\n", frame->msg_data);
        }
        
        cape_udc_del (&payload);
      }
      
      break;
    }
  }
  
  return qin;
}

//-----------------------------------------------------------------------------

void __STDCALL qbus_con__on_snd (void* user_ptr, QBusFrame frame)
{
  QBusCon self = user_ptr;

  //cape_log_fmt (CAPE_LL_TRACE, "QBUS", "routing", "request info: module = %s, sender = %s, method = '%s'", frame->module, frame->sender, frame->method);

  if (cape_str_equal (qbus_engine_con_cid (self->engine, self->con), frame->module))
  {
    switch (frame->ftype)
    {
      case QBUS_FRAME_TYPE_MSG_REQ:
      {
        CapeErr err = cape_err_new ();
        
        QBusM qin = qbus_con__qin_from_frame (frame);
        
        const CapeString saves_key = qbus_methods_save (self->methods, NULL, NULL, frame->chain_key, frame->sender);

        int res = qbus_methods_run (self->methods, frame->method, saves_key, &qin, err);
        if (res)
        {
          cape_log_fmt (CAPE_LL_ERROR, "QBUS", "routing", "%s", cape_err_text (err));
        }
        
        cape_err_del (&err);

        break;
      }
      case QBUS_FRAME_TYPE_MSG_RES:
      {
        CapeErr err = cape_err_new ();
        QBusM qin = qbus_con__qin_from_frame (frame);

        QBusMethodItem mitem = qbus_methods_load (self->methods, frame->chain_key);

        if (NULL == mitem)
        {
          // this can't happen, but better to check this
          
        }
        else
        {
          const CapeString saves_key = qbus_methods_save (self->methods, NULL, NULL, qbus_method_item_skey (mitem), qbus_method_item_sender (mitem));

          qbus_methods_queue (self->methods, mitem, &qin, saves_key);
        }
        
        qbus_method_item_del (&mitem);

        qbus_message_del (&qin);
        cape_err_del (&err);

        break;
      }
    }
  }
  else
  {
    
    
    
  }
}

//-----------------------------------------------------------------------------

void __STDCALL qbus_con__on_con (void* user_ptr, const CapeString cid, const CapeString name, number_t type)
{
  QBusCon self = user_ptr;

  switch (type)
  {
    case 1:
    {
      qbus_router_add (self->router, cid, name);
      break;
    }
    case 2:
    {
      qbus_router_rm (self->router, cid, name);
      break;
    }
  }
}

//-----------------------------------------------------------------------------

int qbus_con_init (QBusCon self, QBusEngines engines, CapeAioContext aio, const CapeString host, CapeErr err)
{
  int res;
  
  // local objects
  CapeUdc options = cape_udc_new (CAPE_UDC_NODE, NULL);
  
  self->engine = qbus_engines_add (engines, "mqtt", err);
  if (self->engine == NULL)
  {
    res = cape_err_code (err);
    goto exit_and_cleanup;
  }
  
  self->engine_context = qbus_engine_ctx_new (self->engine, aio, self->module, err);
  if (self->engine_context == NULL)
  {
    res = cape_err_code (err);
    goto exit_and_cleanup;
  }

  if (host)
  {
    cape_udc_add_s_cp (options, "host", host);
  }
  
  qbus_engine_ctx_add (self->engine, self->engine_context, &(self->con), options, self, qbus_con__on_con, qbus_con__on_snd);
  res = CAPE_ERR_NONE;
  
exit_and_cleanup:
    
  cape_udc_del (&options);
  return res;
}

//-----------------------------------------------------------------------------

void qbus_con_snd (QBusCon self, const CapeString cid, const CapeString method, const CapeString save_key, int ftype, QBusM msg)
{
  const CapeString own_cid = qbus_engine_con_cid (self->engine, self->con);
  
  //cape_log_fmt (CAPE_LL_TRACE, "QBUS", "con send", "send frame to another module, skey = '%s'", save_key);
  
  if (own_cid)
  {
    QBusFrame frame = qbus_con__frame_from_qin (msg);
    
    // set extra infos
    qbus_frame_set (frame, ftype, save_key, cid, method, own_cid);

    qbus_engine_con_snd (self->engine, self->con, cid, frame);

    qbus_frame_del (&frame);
  }

}

//-----------------------------------------------------------------------------
