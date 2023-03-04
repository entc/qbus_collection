#include "qbus_frame.h"

// cape includes
#include <sys/cape_log.h>
#include <fmt/cape_json.h>

#include <stdio.h>
#include <qcrypt.h>

//-----------------------------------------------------------------------------

#define QBUS_PP_STATE__START     0
#define QBUS_PP_STATE__P1        1
#define QBUS_PP_STATE__P2        2
#define QBUS_PP_STATE__P3        3
#define QBUS_PP_STATE__P4        4
#define QBUS_PP_STATE__PS        5
#define QBUS_PP_STATE__P5        6
#define QBUS_PP_STATE__P6        7
#define QBUS_PP_STATE__CO        8

#define QBUS_SE_STATE__P1       '#'
#define QBUS_SE_STATE__P2       '!'
#define QBUS_SE_STATE__P3       '#'
#define QBUS_SE_STATE__P4       '|'
#define QBUS_SE_STATE__PS       '|'
#define QBUS_SE_STATE__P5       '#'
#define QBUS_SE_STATE__P6       '|'
#define QBUS_SE_STATE__CO       '|'

//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------

QBusFrame qbus_frame_new ()
{
  QBusFrame self = CAPE_NEW (struct QBusFrame_s);
  
  self->ftype = 0;
  self->chain_key = NULL;
  self->module = NULL;
  self->method = NULL;
  self->sender = NULL;
  
  self->msg_type = 0;
  self->msg_size = 0;
  self->msg_data = NULL;
  
  self->state = QBUS_PP_STATE__START;
  self->stream = cape_stream_new ();
  
  return self;
}

//-----------------------------------------------------------------------------

void qbus_frame_del (QBusFrame* p_self)
{
  if (*p_self)
  {
    QBusFrame self = *p_self;
    
    cape_str_del (&(self->chain_key));
    
    cape_str_del (&(self->module));
    cape_str_del (&(self->method));
    cape_str_del (&(self->sender));
    
    cape_str_del (&(self->msg_data));
    
    cape_stream_del (&(self->stream));
    
    CAPE_DEL (p_self, struct QBusFrame_s);
  }
}

//-----------------------------------------------------------------------------

void qbus_frame_set_chainkey (QBusFrame self, CapeString* p_chain_key)
{
  cape_str_replace_mv (&(self->chain_key), p_chain_key);
}

//-----------------------------------------------------------------------------

void qbus_frame_set_sender (QBusFrame self, CapeString* p_sender)
{
  cape_str_replace_mv (&(self->sender), p_sender);
}

//-----------------------------------------------------------------------------

void qbus_frame_set_module__cp (QBusFrame self, CapeString module)
{
  cape_str_replace_cp (&(self->module), module);
}

//-----------------------------------------------------------------------------

void qbus_frame_set (QBusFrame self, number_t ftype, const char* chain_key, const char* module, const char* method, const char* sender)
{
  self->ftype = ftype;
  
  cape_str_replace_cp (&(self->chain_key), chain_key);

  cape_str_replace_cp (&(self->module), module);
  cape_str_replace_cp (&(self->method), method);
  cape_str_replace_cp (&(self->sender), sender);
}

//-----------------------------------------------------------------------------

void qbus_frame_set_type (QBusFrame self, number_t ftype, const char* sender)
{
  self->ftype = ftype;

  cape_str_replace_cp (&(self->sender), sender);
}

//-----------------------------------------------------------------------------

CapeUdc qbus_frame_set_udc (QBusFrame self, number_t msgType, CapeUdc* p_payload)
{
  CapeUdc payload = *p_payload;
  
  CapeString h = cape_json_to_s__ex (payload, qcrypt__stream_base64_encode);
  
  CapeUdc rinfo = cape_udc_ext (payload, "I");
  
  // stringify
  cape_str_replace_mv (&(self->msg_data), &h);
 
  self->msg_size = cape_str_size (self->msg_data);
  self->msg_type = msgType;

  cape_udc_del (p_payload);
  
  return rinfo;
}

//-----------------------------------------------------------------------------

void qbus_frame_set_err (QBusFrame self, CapeErr err)
{
  CapeUdc payload = cape_udc_new (CAPE_UDC_NODE, NULL);
  
  cape_udc_add_s_cp (payload, "err_text", cape_err_text (err));
  cape_udc_add_n (payload, "err_code", cape_err_code (err));
  
  qbus_frame_set_udc (self, QBUS_MTYPE_JSON, &payload);
}

//-----------------------------------------------------------------------------

number_t pvd2_frame_get_type (QBusFrame self)
{
  return self->ftype;
}

//-----------------------------------------------------------------------------

const CapeString pvd2_frame_get_module (QBusFrame self)
{
  return self->module;
}

//-----------------------------------------------------------------------------

const CapeString pvd2_frame_get_method (QBusFrame self)
{
  return self->method;
}

//-----------------------------------------------------------------------------

const CapeString pvd2_frame_get_sender (QBusFrame self)
{
  return self->sender;
}

//-----------------------------------------------------------------------------

const CapeString qbus_frame_get_chainkey (QBusFrame self)
{
  return self->chain_key;
}

//-----------------------------------------------------------------------------

CapeUdc qbus_frame_get_udc (QBusFrame self)
{
  if (self->msg_size)
  {
    // convert from raw data into json data structure
    CapeUdc payload = cape_json_from_buf (self->msg_data, self->msg_size, qcrypt__stream_base64_decode);
    
    if (payload)
    {
      return payload;
    }
    else
    {
      printf ("CAN'T PARSE JSON [%lu] '%s'\n", self->msg_size, self->msg_data);
    }
  }
  
  return NULL;
}

//-----------------------------------------------------------------------------

int qbus_frame_decode (QBusFrame self, const char* bufdat, number_t buflen, number_t* written)
{
  number_t posS = 0;
  const char* posB = bufdat;

  if (buflen == 0)
  {
    return 0;
  }
    
  for (; posS < buflen; posS++, posB++)
  {
    switch (self->state)
    {
      case QBUS_PP_STATE__START:
      {
        // check first character
        if (*posB != QBUS_SE_STATE__P1)
        {
          printf ("qbus protocol error at P1\n");
        }
        
        self->state = QBUS_PP_STATE__P1;
        
        break;
      }
      case QBUS_PP_STATE__P1:
      {
        if (*posB == QBUS_SE_STATE__P2)
        {
          self->state = QBUS_PP_STATE__P2;
          self->ftype = cape_stream_to_n (self->stream);
        }
        else
        {
          cape_stream_append_c (self->stream, *posB);
        }
        
        break;
      }
      case QBUS_PP_STATE__P2:
      {
        if (*posB == QBUS_SE_STATE__P3)
        {
          self->state = QBUS_PP_STATE__P3;
          self->chain_key = cape_stream_to_s (self->stream);
        }
        else
        {
          cape_stream_append_c (self->stream, *posB);
        }
        
        break;
      }
      case QBUS_PP_STATE__P3:
      {
        if (*posB == QBUS_SE_STATE__P4)
        {
          self->state = QBUS_PP_STATE__P4;
          self->module = cape_stream_to_s (self->stream);          
        }
        else
        {
          cape_stream_append_c (self->stream, *posB);
        }
        
        break;
      }
      case QBUS_PP_STATE__P4:
      {
        if (*posB == QBUS_SE_STATE__PS)
        {
          self->state = QBUS_PP_STATE__PS;          
          self->method = cape_stream_to_s (self->stream);          
        }
        else
        {
          cape_stream_append_c (self->stream, *posB);
        }
        
        break;
      }
      case QBUS_PP_STATE__PS:
      {
        if (*posB == QBUS_SE_STATE__P5)
        {
          self->state = QBUS_PP_STATE__P5;          
          self->sender = cape_stream_to_s (self->stream);          
        }
        else
        {
          cape_stream_append_c (self->stream, *posB);
        }
        
        break;
      }
      case QBUS_PP_STATE__P5:
      {
        if (*posB == QBUS_SE_STATE__P6)
        {
          self->state = QBUS_PP_STATE__P6;
          self->msg_type = cape_stream_to_n (self->stream);          
        }
        else
        {
          cape_stream_append_c (self->stream, *posB);
        }
        
        break;
      }
      case QBUS_PP_STATE__P6:
      {
        if (*posB == QBUS_SE_STATE__CO)
        {
          self->msg_size = cape_stream_to_n (self->stream);
          
          if (self->msg_size == 0)
          {
            *written += (posB - bufdat) + 1;
            
            self->state = QBUS_PP_STATE__START;
            
            return TRUE;
          }
          else
          {
            self->state = QBUS_PP_STATE__CO;
          }
        }
        else
        {
          cape_stream_append_c (self->stream, *posB);
        }
        
        break;
      }
      case QBUS_PP_STATE__CO:
      {
        cape_stream_append_c (self->stream, *posB);
        
        if (cape_stream_size (self->stream) == self->msg_size)
        {
          self->msg_data = cape_stream_to_s (self->stream);
          
          *written += (posB - bufdat) + 1;
          
          self->state = QBUS_PP_STATE__START;
          
          return TRUE;
        }
        
        break;
      }
    }
  }
  
  *written += buflen;
  
  return FALSE;
}

//-----------------------------------------------------------------------------

void qbus_frame_encode (QBusFrame self, CapeStream cs)
{
  cape_stream_clr (cs);
  
  // P1
  cape_stream_append_c (cs, QBUS_SE_STATE__P1);
  cape_stream_append_n (cs, self->ftype);
  
  // P2
  cape_stream_append_c (cs, QBUS_SE_STATE__P2);
  cape_stream_append_str (cs, self->chain_key);
  
  // P3
  cape_stream_append_c (cs, QBUS_SE_STATE__P3);
  if (self->module)
  {
    cape_stream_append_str (cs, self->module);
  }
  
  // P4
  cape_stream_append_c (cs, QBUS_SE_STATE__P4);
  if (self->method)
  {
    cape_stream_append_str (cs, self->method);
  }
  
  // PS
  cape_stream_append_c (cs, QBUS_SE_STATE__PS);
  if (self->sender)
  {
    cape_stream_append_str (cs, self->sender);
  }
  
  // P5
  cape_stream_append_c (cs, QBUS_SE_STATE__P5);
  cape_stream_append_n (cs, self->msg_type);
  
  // P6
  cape_stream_append_c (cs, QBUS_SE_STATE__P6);
  cape_stream_append_n (cs, self->msg_size);
  
  // CO
  cape_stream_append_c (cs, QBUS_SE_STATE__CO);
  if (self->msg_data)
  {
    cape_stream_append_buf (cs, self->msg_data, self->msg_size);
  }
}

//-----------------------------------------------------------------------------

