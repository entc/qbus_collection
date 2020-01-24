#ifndef __CAPE_FMT__TEMPLATE__H
#define __CAPE_FMT__TEMPLATE__H 1

#include "sys/cape_export.h"
#include "sys/cape_types.h"
#include "sys/cape_err.h"
#include "stc/cape_udc.h"

//-----------------------------------------------------------------------------

struct CapeTemplate_s; typedef struct CapeTemplate_s* CapeTemplate;

//-----------------------------------------------------------------------------

typedef int (__STDCALL *fct_cape_template__on_text) (void* ptr, const char* text);
typedef int (__STDCALL *fct_cape_template__on_file) (void* ptr, const char* file);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   CapeTemplate   cape_template_new            (void);

__CAPE_LIBEX   void           cape_template_del            (CapeTemplate*);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   int            cape_template_compile_file   (CapeTemplate, const char* path, const char* name, const char* lang, CapeErr);

__CAPE_LIBEX   int            cape_template_compile_str    (CapeTemplate, const char* content, CapeErr);

__CAPE_LIBEX   int            cape_template_apply          (CapeTemplate, CapeUdc node, void* ptr, fct_cape_template__on_text, fct_cape_template__on_file, CapeErr);

//-----------------------------------------------------------------------------

#endif
