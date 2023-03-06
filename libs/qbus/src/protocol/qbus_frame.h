#ifndef __QBUS__PROTOCOL__H
#define __QBUS__PROTOCOL__H 1

// cape includes
#include <sys/cape_export.h>
#include <sys/cape_err.h>
#include <stc/cape_stream.h>
#include <stc/cape_udc.h>

//-----------------------------------------------------------------------------

#define QBUS_FRAME_TYPE_NONE         0
#define QBUS_FRAME_TYPE_ROUTE_REQ    1
#define QBUS_FRAME_TYPE_ROUTE_RES    2
#define QBUS_FRAME_TYPE_MSG_REQ      3
#define QBUS_FRAME_TYPE_MSG_RES      4
#define QBUS_FRAME_TYPE_ROUTE_UPD    5
#define QBUS_FRAME_TYPE_METHODS      6
#define QBUS_FRAME_TYPE_OBSVBL_REQ   7
#define QBUS_FRAME_TYPE_OBSVBL_RES   8

#define QBUS_MTYPE_NONE         0
#define QBUS_MTYPE_JSON         1
#define QBUS_MTYPE_FILE         2

//-----------------------------------------------------------------------------

#pragma pack(push, 16)
struct QBusFrame_s
{
  // basic values
  
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

};
#pragma pack(pop)

typedef struct QBusFrame_s* QBusFrame;

//-----------------------------------------------------------------------------

__CAPE_LIBEX   QBusFrame         qbus_frame_new           ();

__CAPE_LIBEX   void              qbus_frame_del           (QBusFrame*);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   void              qbus_frame_set           (QBusFrame, number_t ftype, const CapeString chain_key, const CapeString module, const CapeString method, const CapeString sender);

__CAPE_LIBEX   void              qbus_frame_set_type      (QBusFrame, number_t ftype, const char* sender);

__CAPE_LIBEX   void              qbus_frame_set_chainkey  (QBusFrame, CapeString* p_chain_key);

__CAPE_LIBEX   void              qbus_frame_set_sender    (QBusFrame, CapeString* p_sender);

__CAPE_LIBEX   void              qbus_frame_set_module__cp  (QBusFrame, const CapeString module);

__CAPE_LIBEX   void              qbus_frame_set_err       (QBusFrame, CapeErr);

// returns the rinfo if available
__CAPE_LIBEX   CapeUdc           qbus_frame_set_udc       (QBusFrame, number_t msgType, CapeUdc* p_payload);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   const CapeString  qbus_frame_get_chainkey  (QBusFrame);

__CAPE_LIBEX   CapeUdc           qbus_frame_get_udc       (QBusFrame);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   int               qbus_frame_decode        (QBusFrame, const char* bufdat, number_t buflen, number_t* written);

__CAPE_LIBEX   void              qbus_frame_encode        (QBusFrame, CapeStream cs);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   number_t            __STDCALL pvd2_frame_get_type         (QBusFrame);

__CAPE_LIBEX   const CapeString    __STDCALL pvd2_frame_get_module       (QBusFrame);

__CAPE_LIBEX   const CapeString    __STDCALL pvd2_frame_get_method       (QBusFrame);

__CAPE_LIBEX   const CapeString    __STDCALL pvd2_frame_get_sender       (QBusFrame);

//-----------------------------------------------------------------------------

#endif
