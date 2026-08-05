#ifndef __ADBL_TOOLS__H
#define __ADBL_TOOLS__H 1

#include "adbl.h"

#include "sys/cape_export.h"
#include "sys/cape_err.h"
#include "stc/cape_udc.h"

//-----------------------------------------------------------------------------

struct AdblSyncList_s; typedef struct AdblSyncList_s* AdblSyncList;

//-----------------------------------------------------------------------------

typedef int   (__STDCALL *fct_adbl_sync_list_values)          (void* user_ptr, CapeUdc values, CapeUdc item, CapeErr);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   AdblSyncList       adbl_sync_list_new          (const CapeString table, void* user_ptr, fct_adbl_sync_list_values on_values);

__CAPE_LIBEX   void               adbl_sync_list_del          (AdblSyncList*);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   int                adbl_sync_list_run          (AdblSyncList,  AdblTrx, CapeUdc list, CapeUdc params, const CapeString id_column, CapeErr err);

//-----------------------------------------------------------------------------

#endif

