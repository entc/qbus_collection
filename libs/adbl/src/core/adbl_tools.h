#ifndef __ADBL_TOOLS__H
#define __ADBL_TOOLS__H 1

#include "adbl.h"

#include "sys/cape_export.h"
#include "sys/cape_err.h"
#include "stc/cape_udc.h"

#define ADBLSYNC_TYPE__UPDATE    0
#define ADBLSYNC_TYPE__INSERT    1
#define ADBLSYNC_TYPE__DELETE    2

//-----------------------------------------------------------------------------

struct AdblSyncList_s; typedef struct AdblSyncList_s* AdblSyncList;

//-----------------------------------------------------------------------------

typedef int   (__STDCALL *fct_adbl_sync_list_values)          (void* user_ptr, CapeUdc values, CapeUdc item, CapeErr);
typedef int   (__STDCALL *fct_adbl_sync_list_update)          (void* user_ptr, AdblTrx, number_t id, CapeUdc item, CapeErr);
typedef int   (__STDCALL *fct_adbl_sync_list_insert)          (void* user_ptr, AdblTrx, number_t id, CapeUdc item, CapeErr);
typedef int   (__STDCALL *fct_adbl_sync_list_delete)          (void* user_ptr, AdblTrx, number_t id, CapeErr);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   AdblSyncList       adbl_sync_list_new          (const CapeString table, const CapeString id_column, void* user_ptr, fct_adbl_sync_list_values on_values, fct_adbl_sync_list_update on_update, fct_adbl_sync_list_insert on_insert, fct_adbl_sync_list_delete on_delete);

__CAPE_LIBEX   void               adbl_sync_list_del          (AdblSyncList*);

//-----------------------------------------------------------------------------

__CAPE_LIBEX   int                adbl_sync_list_run          (AdblSyncList, AdblTrx, CapeUdc list, CapeUdc params, CapeErr err);

//-----------------------------------------------------------------------------

#endif

