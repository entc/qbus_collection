#ifndef __QTEE__FORMAT__H
#define __QTEE__FORMAT__H 1

#include <sys/cape_types.h>
#include <sys/cape_err.h>
#include <stc/cape_udc.h>

#include "qtee_template.h"

struct QTeeFormat_s; typedef struct QTeeFormat_s* QTeeFormat;

//-----------------------------------------------------------------------------
 
                              /* default constructor */
__CAPE_LIBEX   QTeeFormat     qtee_format_new               (void);

                              /* default destructor */
__CAPE_LIBEX   void           qtee_format_del               (QTeeFormat*);

                              /* factory to create a new object depending on the parsing process 

                              -> returns NULL if no format was found
                               */
__CAPE_LIBEX   QTeeFormat     qtee_format_gen               (const CapeString possible_format);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   void           qtee_format_parse             (QTeeFormat, const CapeString possible_format);

__CAPE_LIBEX   int            qtee_format_has_encrypted     (QTeeFormat);

__CAPE_LIBEX   CapeUdc        qtee_format_item              (QTeeFormat, CapeList node_stack);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   CapeString     qtee_format_apply_node        (QTeeFormat, CapeUdc item, fct_cape_template__on_pipe on_pipe);

__CAPE_LIBEX   CapeString     qtee_format_apply_s           (QTeeFormat, const CapeString value, fct_cape_template__on_pipe on_pipe);

__CAPE_LIBEX   CapeString     qtee_format_apply_n           (QTeeFormat, number_t value, fct_cape_template__on_pipe on_pipe);

__CAPE_LIBEX   CapeString     qtee_format_apply_f           (QTeeFormat, double value, fct_cape_template__on_pipe on_pipe);

__CAPE_LIBEX   CapeString     qtee_format_apply_b           (QTeeFormat, int value, fct_cape_template__on_pipe on_pipe);

__CAPE_LIBEX   CapeString     qtee_format_apply_d           (QTeeFormat, const CapeDatetime*, fct_cape_template__on_pipe on_pipe);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   CapeUdc        qtee_format_seek_item         (CapeList node_stack, const CapeString name);

//-----------------------------------------------------------------------------

#endif
