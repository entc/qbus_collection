#ifndef __CAPE_SYS__NET__H
#define __CAPE_SYS__NET__H 1

#include "sys/cape_export.h"
#include "sys/cape_err.h"
#include "stc/cape_str.h"

//-----------------------------------------------------------------------------

__CAPE_LIBEX   CapeString    cape_net__resolve            (const CapeString host, int ipv6, CapeErr err);

//-----------------------------------------------------------------------------

#endif
