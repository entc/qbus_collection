#ifndef __QTEE__TEMPLATE__H
#define __QTEE__TEMPLATE__H 1

// cape includes
#include "sys/cape_export.h"
#include "sys/cape_types.h"
#include "sys/cape_err.h"
#include "stc/cape_udc.h"

// include all callbacks
#include "qtee_template_cb.h"

#define CAPE_TEMPLATE_FLAG__NONE         0x0000
#define CAPE_TEMPLATE_FLAG__ENCRYPTED    0x0001

//-----------------------------------------------------------------------------

struct CapeTemplate_s; typedef struct CapeTemplate_s* CapeTemplate;

//-----------------------------------------------------------------------------

__CAPE_LIBEX   CapeTemplate   cape_template_new            (void);

__CAPE_LIBEX   void           cape_template_del            (CapeTemplate*);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   int            cape_template_compile_file   (CapeTemplate, const char* path, const char* name, const char* lang, CapeErr);

                              /* compiles the template into parts */
__CAPE_LIBEX   void           cape_template_compile_str    (CapeTemplate, const char* content);

__CAPE_LIBEX   int            cape_template_apply          (CapeTemplate, CapeUdc node, void* ptr, fct_cape_template__on_text, fct_cape_template__on_file, fct_cape_template__on_pipe, fct_cape_template__on_tag, CapeErr);

__CAPE_LIBEX   CapeString     cape_template_run            (const CapeString s, CapeUdc node, fct_cape_template__on_pipe, fct_cape_template__on_tag, CapeErr);

//-----------------------------------------------------------------------------

#endif
