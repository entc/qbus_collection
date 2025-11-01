#ifndef __QTEE__EVAL__H
#define __QTEE__EVAL__H 1

#include <sys/cape_types.h>
#include <sys/cape_err.h>
#include <stc/cape_udc.h>

#include "qtee_template.h"

//-----------------------------------------------------------------------------

__CAPE_LIBEX   int            qtee_eval_b                  (const CapeString s, CapeUdc node, int*, fct_cape_template__on_pipe, CapeErr);

__CAPE_LIBEX   int            qtee_compare                 (const CapeString left, const CapeString right);

//-----------------------------------------------------------------------------

#endif
