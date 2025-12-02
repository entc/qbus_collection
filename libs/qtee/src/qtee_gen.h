#ifndef __QTEE__GENERATOR__H
#define __QTEE__GENERATOR__H 1

#include <sys/cape_types.h>
#include <sys/cape_err.h>
#include <stc/cape_udc.h>

//-----------------------------------------------------------------------------

__CAPE_LIBEX   CapeString     qtee_gen_decimal             (double value, const CapeString format_text);

__CAPE_LIBEX   CapeString     qtee_gen_lpadding            (const CapeString value, const CapeString format_text);

__CAPE_LIBEX   CapeString     qtee_gen_substr              (const CapeString value, const CapeString format_text);

//-----------------------------------------------------------------------------

#endif
