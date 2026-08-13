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

struct QTeePart_s; typedef struct QTeePart_s* QTeePart;

//-----------------------------------------------------------------------------

__CAPE_LIBEX   QTeePart   qtee_part_new      (int type, const CapeString raw_text, QTeePart parent);

__CAPE_LIBEX   void               qtee_part_del      (QTeePart*);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   void               qtee_part_add      (QTeePart, QTeePart part);

__CAPE_LIBEX   void               qtee_part_clear    (QTeePart);

__CAPE_LIBEX   QTeePart   qtee_part_parent   (QTeePart);

__CAPE_LIBEX   int                qtee_part_equal    (QTeePart, const CapeString text);

__CAPE_LIBEX   int                qtee_part_apply    (QTeePart, CapeList node_stack, QTeeTemplateCB cb, number_t pos, CapeErr err);

__CAPE_LIBEX   void               qtee_part_set      (QTeePart, CapeString* p_text, CapeString* p_modn);

//-----------------------------------------------------------------------------

#endif
