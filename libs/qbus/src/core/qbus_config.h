#ifndef __QBUS_CONFIG__H
#define __QBUS_CONFIG__H 1

#include "sys/cape_export.h"
#include "sys/cape_err.h"
#include "stc/cape_udc.h"

//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------

struct QBusConfig_s; typedef struct QBusConfig_s* QBusConfig;

//-----------------------------------------------------------------------------

__CAPE_LOCAL   QBusConfig         qbus_config_new               (const char* module_origin);

__CAPE_LOCAL   void               qbus_config_del               (QBusConfig*);

__CAPE_LOCAL   void               qbus_config_set_args          (QBusConfig, CapeUdc* p_args);

__CAPE_LOCAL   int                qbus_config_init              (QBusConfig, CapeErr);

__CAPE_LOCAL   CapeUdc            qbus_config_get_pvds          (QBusConfig);

__CAPE_LOCAL   CapeUdc            qbus_config_get_logger        (QBusConfig);

__CAPE_LOCAL   number_t           qbus_config_get_threads       (QBusConfig);

__CAPE_LOCAL   const CapeString   qbus_config_get_name          (QBusConfig);

//-----------------------------------------------------------------------------

#endif