#ifndef __ADBL_MYSQL__BINDVARS_H
#define __ADBL_MYSQL__BINDVARS_H 1

//-----------------------------------------------------------------------------

// mysql includes
#include <mysql.h>

// cape includes
#include "sys/cape_export.h"
#include "sys/cape_err.h"
#include "stc/cape_udc.h"

//-----------------------------------------------------------------------------

// FIX
// pre-8.0 defines the my_bool variable
// after 8.0.2 it was re-introduced
#ifndef my_bool
typedef char my_bool;
#endif

//=============================================================================

struct AdblBindVars_s; typedef struct AdblBindVars_s* AdblBindVars;

//-----------------------------------------------------------------------------

__CAPE_LIBEX   AdblBindVars    adbl_bindvars_new             (number_t size);

__CAPE_LIBEX   void            adbl_bindvars_del             (AdblBindVars*);

__CAPE_LIBEX   int             adbl_bindvars_get             (AdblBindVars, number_t index, CapeUdc item);  // returns if NULL = false

__CAPE_LIBEX   int             adbl_bindvars_set_from_node   (AdblBindVars, CapeUdc node, int check_for_specials, CapeErr err);

__CAPE_LIBEX   int             adbl_bindvars_add_from_node   (AdblBindVars, CapeUdc node, CapeErr err);

__CAPE_LIBEX   MYSQL_BIND*     adbl_bindvars_binds           (AdblBindVars);

//-----------------------------------------------------------------------------

#endif
