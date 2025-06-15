#ifndef __QBUS_FRAME__H
#define __QBUS_FRAME__H 1

// cape includes
#include <sys/cape_err.h>
#include <stc/cape_udc.h>
#include <stc/cape_stream.h>

// qbus includes
#include "../src/qbus_message.h"

//-----------------------------------------------------------------------------
// object typedefs and definitions

struct QBusFrame_s
{
  number_t     ftype;
  
  CapeString   chain_key;
  
  CapeString   module;
  
  CapeString   method;
  
  CapeString   sender;
  
  number_t     msg_type;
  
  number_t     msg_size;
  
  CapeString   msg_data;

  // for decoding
  
  number_t     state;
  
  CapeStream   stream;

}; typedef struct QBusFrame_s* QBusFrame;

//-----------------------------------------------------------------------------

__CAPE_LIBEX   QBusFrame        qbus_frame_new                  (QBusM);

__CAPE_LIBEX   void             qbus_frame_del                  (QBusFrame*);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   void             qbus_frame_serialize            (QBusFrame, CapeStream);

__CAPE_LIBEX   int              qbus_frame_deserialize          (QBusFrame, const char* bufdat, number_t buflen, number_t* written);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   CapeUdc          qbus_frame_set_udc              (QBusFrame, number_t msgType, CapeUdc* p_payload);

//-----------------------------------------------------------------------------

#endif
