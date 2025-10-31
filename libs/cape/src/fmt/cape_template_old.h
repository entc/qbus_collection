#ifndef __CAPE_FMT__TEMPLATE__H
#define __CAPE_FMT__TEMPLATE__H 1

#include "sys/cape_export.h"
#include "sys/cape_types.h"
#include "sys/cape_err.h"
#include "stc/cape_udc.h"

#define CAPE_TEMPLATE_FLAG__NONE         0x0000
#define CAPE_TEMPLATE_FLAG__ENCRYPTED    0x0001

//-----------------------------------------------------------------------------

struct CapeTemplate_s; typedef struct CapeTemplate_s* CapeTemplate;

//-----------------------------------------------------------------------------

typedef int    (__STDCALL *fct_cape_template__on_text) (void* ptr, const char* text);
typedef int    (__STDCALL *fct_cape_template__on_file) (void* ptr, const char* file, number_t flags, CapeErr err);
typedef char*  (__STDCALL *fct_cape_template__on_pipe) (const char* name, const char* pipe, const char* value);
typedef void   (__STDCALL *fct_cape_template__on_tag) (void* ptr, const char* tag);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   CapeTemplate   cape_template_new            (void);

__CAPE_LIBEX   void           cape_template_del            (CapeTemplate*);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   int            cape_template_compile_file   (CapeTemplate, const char* path, const char* name, const char* lang, CapeErr);

                              /*
                               compiles the template
                               */
__CAPE_LIBEX   int            cape_template_compile_str    (CapeTemplate, const char* content, CapeErr);

__CAPE_LIBEX   int            cape_template_apply          (CapeTemplate, CapeUdc node, void* ptr, fct_cape_template__on_text, fct_cape_template__on_file, fct_cape_template__on_pipe, fct_cape_template__on_tag, CapeErr);

__CAPE_LIBEX   CapeString     cape_template_run            (const CapeString s, CapeUdc node, fct_cape_template__on_pipe, fct_cape_template__on_tag, CapeErr);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   int            cape_eval_f                  (const CapeString s, CapeUdc node, double*, fct_cape_template__on_pipe, CapeErr);

__CAPE_LIBEX   int            cape_eval_b                  (const CapeString s, CapeUdc node, int*, fct_cape_template__on_pipe, CapeErr);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   double         cape_eval_to_f               (const CapeString s);

//-----------------------------------------------------------------------------

#endif
