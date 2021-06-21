#ifndef __CAPE_FMT__ARGS__H
#define __CAPE_FMT__ARGS__H 1

#include "sys/cape_export.h"
#include "sys/cape_types.h"
#include "stc/cape_udc.h"

//-----------------------------------------------------------------------------

__CAPE_LIBEX   CapeUdc           cape_args_from_args           (int argc, char *argv[], const CapeString name);

__CAPE_LIBEX   CapeString        cape_args_config_file         (const CapeString primary_folder, const CapeString name);

//-----------------------------------------------------------------------------

#endif
