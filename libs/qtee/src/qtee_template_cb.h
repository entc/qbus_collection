#ifndef __QTEE__TEMPLATE_CB__H
#define __QTEE__TEMPLATE_CB__H 1

#include "sys/cape_export.h"
#include "sys/cape_types.h"
#include "sys/cape_err.h"
#include "stc/cape_udc.h"

#include "qtee_format.h"

//-----------------------------------------------------------------------------

typedef int    (__STDCALL *fct_cape_template__on_text) (void* ptr, const char* bufdat, number_t buflen);
typedef int    (__STDCALL *fct_cape_template__on_file) (void* ptr, const char* file, number_t flags, CapeErr err);
typedef void   (__STDCALL *fct_cape_template__on_tag) (void* ptr, const char* tag);

//-----------------------------------------------------------------------------

struct QTeeTemplateCB_s; typedef struct QTeeTemplateCB_s* QTeeTemplateCB;

//-----------------------------------------------------------------------------

__CAPE_LIBEX   QTeeTemplateCB     qtee_template_cb_new        (void* user_ptr, fct_cape_template__on_text on_text, fct_cape_template__on_file on_file, fct_cape_template__on_pipe on_pipe, fct_cape_template__on_tag on_tag);

__CAPE_LIBEX   void               qtee_template_cb_del        (QTeeTemplateCB*);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   void               qtee_template_cb__value     (QTeeTemplateCB, const CapeString text);

__CAPE_LIBEX   void               qtee_template_cb__tag       (QTeeTemplateCB, const CapeString tag);

__CAPE_LIBEX   void               qtee_template_cb__s         (QTeeTemplateCB, QTeeFormat, const CapeString value);

__CAPE_LIBEX   void               qtee_template_cb__n         (QTeeTemplateCB, QTeeFormat, number_t value);

__CAPE_LIBEX   void               qtee_template_cb__f         (QTeeTemplateCB, QTeeFormat, double value);

__CAPE_LIBEX   void               qtee_template_cb__d         (QTeeTemplateCB, QTeeFormat, const CapeDatetime* value);

__CAPE_LIBEX   void               qtee_template_cb__b         (QTeeTemplateCB, QTeeFormat, int value);

__CAPE_LIBEX   void               qtee_template_cb__m         (QTeeTemplateCB, QTeeFormat, CapeStream);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   int                qtee_template_cb__file      (QTeeTemplateCB, QTeeFormat format, CapeList node_stack, CapeErr err);

//-----------------------------------------------------------------------------

#endif
