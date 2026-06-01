#ifndef __QTEE__TEMPLATE_COMPILER__H
#define __QTEE__TEMPLATE_COMPILER__H 1

// cape includes
#include "sys/cape_export.h"
#include "sys/cape_types.h"
#include "sys/cape_err.h"
#include "stc/cape_udc.h"

// local includes
#include "qtee_part.h"

//-----------------------------------------------------------------------------

struct QTeeCompiler_s; typedef struct QTeeCompiler_s* QTeeCompiler;

//-----------------------------------------------------------------------------

__CAPE_LIBEX   QTeeCompiler           qtee_compiler_new            (QTeePart part);

__CAPE_LIBEX   void                   qtee_compiler_del            (QTeeCompiler*);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   void                   qtee_compiler_parse          (QTeeCompiler, const char* buffer, number_t size);

//-----------------------------------------------------------------------------

#endif
