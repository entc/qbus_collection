#ifndef __QBUS_CONFIG__H
#define __QBUS_CONFIG__H 1

#include <sys/cape_export.h>
#include <sys/cape_err.h>
#include <stc/cape_udc.h>

//-----------------------------------------------------------------------------

struct QBusConfig_s; typedef struct QBusConfig_s* QBusConfig;

//-----------------------------------------------------------------------------

__CAPE_LIBEX   QBusConfig         qbus_config_new        (const CapeString name);

__CAPE_LIBEX   void               qbus_config_del        (QBusConfig*);

__CAPE_LIBEX   void               qbus_config_init       (QBusConfig, int argc, char *argv[]);

__CAPE_LIBEX   const CapeString   qbus_config_name       (QBusConfig);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   const CapeString   qbus_config_s          (QBusConfig, const char* name, const CapeString default_val);

__CAPE_LIBEX   number_t           qbus_config_n          (QBusConfig, const char* name, number_t default_val);

__CAPE_LIBEX   double             qbus_config_f          (QBusConfig, const char* name, double default_val);

__CAPE_LIBEX   int                qbus_config_b          (QBusConfig, const char* name, int default_val);

__CAPE_LIBEX   CapeUdc            qbus_config_node       (QBusConfig, const char* name);

//-----------------------------------------------------------------------------

#endif
