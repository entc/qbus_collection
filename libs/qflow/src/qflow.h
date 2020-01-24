#ifndef __QFLOW__H
#define __QFLOW__H 1

//-----------------------------------------------------------------------------

// cape includes
#include "sys/cape_export.h"
#include "sys/cape_types.h"
#include <sys/cape_err.h>
#include <stc/cape_str.h>

#define QFLOW_STATE_INITIAL         1
#define QFLOW_STATE_RUNNING         2
#define QFLOW_STATE_COMPLETE        3

//-----------------------------------------------------------------------------

struct QFlow_s; typedef struct QFlow_s* QFlow;

//-----------------------------------------------------------------------------

__CAPE_LIBEX     QFlow       qflow_new      (number_t raid);

__CAPE_LIBEX     void        qflow_del      (QFlow*);

__CAPE_LIBEX     void        qflow_add      (QFlow, const CapeString, number_t max);

__CAPE_LIBEX     void        qflow_set      (QFlow, number_t current);

//-----------------------------------------------------------------------------

#endif
