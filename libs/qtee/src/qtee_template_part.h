#ifndef __QTEE__TEMPLATE_PART__H
#define __QTEE__TEMPLATE_PART__H 1

// cape includes
#include "sys/cape_export.h"
#include "sys/cape_types.h"
#include "sys/cape_err.h"
#include "stc/cape_udc.h"

// include all callbacks
#include "qtee_template_cb.h"

//-----------------------------------------------------------------------------

#define PART_TYPE_NONE   0
#define PART_TYPE_TEXT   1
#define PART_TYPE_FILE   2
#define PART_TYPE_TAG    3
#define PART_TYPE_NODE   4
#define PART_TYPE_MOD    6

//-----------------------------------------------------------------------------

struct QTeeTemplatePart_s; typedef struct QTeeTemplatePart_s* QTeeTemplatePart;

//-----------------------------------------------------------------------------

__CAPE_LIBEX   QTeeTemplatePart   qtee_template_part_new      (int type, const CapeString raw_text, QTeeTemplatePart parent);

__CAPE_LIBEX   void               qtee_template_part_del      (QTeeTemplatePart*);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   void               qtee_template_part_add      (QTeeTemplatePart, QTeeTemplatePart part);

__CAPE_LIBEX   QTeeTemplatePart   qtee_template_part_parent   (QTeeTemplatePart);

__CAPE_LIBEX   int                qtee_template_part_equal    (QTeeTemplatePart, const CapeString text);

__CAPE_LIBEX   int                qtee_template_part_apply    (QTeeTemplatePart, CapeList node_stack, QTeeTemplateCB cb, number_t pos, CapeErr err);

__CAPE_LIBEX   void               qtee_template_part_set      (QTeeTemplatePart, CapeString* p_text, CapeString* p_modn);

//-----------------------------------------------------------------------------

#endif
