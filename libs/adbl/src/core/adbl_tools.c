#include "adbl_tools.h"

// cape includes
#include <sys/cape_log.h>
#include <fmt/cape_json.h>

//-----------------------------------------------------------------------------

struct AdblSyncList_s
{
    CapeString table;
    
    void* user_ptr;
    fct_adbl_sync_list_values on_prepare_values;
};

//-----------------------------------------------------------------------------

AdblSyncList adbl_sync_list_new (const CapeString table, void* user_ptr, fct_adbl_sync_list_values on_values)
{
    AdblSyncList self = CAPE_NEW (struct AdblSyncList_s);
    
    self->table = cape_str_cp (table);
    
    self->user_ptr = user_ptr;
    self->on_prepare_values = on_values;
    
    return self;
}

//-----------------------------------------------------------------------------

void adbl_sync_list_del (AdblSyncList* p_self)
{
    if (*p_self)
    {
        AdblSyncList self = *p_self;
      
        cape_str_del (&(self->table));
      
        CAPE_DEL(p_self, struct AdblSyncList_s);
    }
}

//-----------------------------------------------------------------------------

static int adbl_sync_list__update (AdblSyncList self, AdblTrx trx, CapeUdc item_current, CapeUdc item_save, CapeUdc params_list, CapeErr err)
{
    int res;
    
    CapeUdc params = cape_udc_cp (item_current);
    CapeUdc values = cape_udc_cp (params_list);

    if (self->on_prepare_values)
    {
        res = self->on_prepare_values (self->user_ptr, values, item_save, err);
        if (res)
        {
            goto cleanup_exit;
        }
    }
    
    res = adbl_trx_update (trx, self->table, &params, &values, err);
    if (res)
    {
        goto cleanup_exit;
    }
    
    
    res = CAPE_ERR_NONE;

cleanup_exit:
    
    cape_udc_del (&values);
    cape_udc_del (&params);
    return res;
}

//-----------------------------------------------------------------------------

static int adbl_sync_list__insert (AdblSyncList self, AdblTrx trx, CapeUdc item_save, CapeUdc params_list, CapeErr err)
{
    int res;

    number_t id;
    
    CapeUdc values = cape_udc_cp (params_list);

    if (self->on_prepare_values)
    {
        res = self->on_prepare_values (self->user_ptr, values, item_save, err);
        if (res)
        {
            goto cleanup_exit;
        }
    }

    // execute query
    id = adbl_trx_insert (trx, self->table, &values, err);
    if (id == 0)
    {
        res = cape_err_code (err);
        goto cleanup_exit;
    }
    
    
    
    res = CAPE_ERR_NONE;

cleanup_exit:
    
    cape_udc_del (&values);
    return res;
}

//-----------------------------------------------------------------------------

static int adbl_sync_list__delete (AdblSyncList self, AdblTrx trx, CapeUdc item_current, CapeErr err)
{
    CapeUdc params = cape_udc_cp (item_current);
    
    return adbl_trx_delete (trx, self->table, &params, err);
}

//-----------------------------------------------------------------------------

static int adbl_sync_list__process (AdblSyncList self, AdblTrx trx, CapeUdc list_current, CapeUdc list_save, CapeUdc params_list, CapeErr err)
{
    int res;
    
    CapeUdcCursor* cursor_current = cape_udc_cursor_new (list_current, CAPE_DIRECTION_FORW);
    CapeUdcCursor* cursor_save = cape_udc_cursor_new (list_save, CAPE_DIRECTION_FORW);

    // run update
    while (TRUE)
    {
        int r1 = cape_udc_cursor_next (cursor_current);
        int r2 = cape_udc_cursor_next (cursor_save);

        if (r1)
        {
            if (r2)
            {
                cape_log_msg (CAPE_LL_TRACE, "ADBL", "sync list", "update database item");

                res = adbl_sync_list__update (self, trx, cursor_current->item, cursor_save->item, params_list, err);
                if (res)
                {
                    goto cleanup_exit;
                }
            }
            else
            {
                cape_log_msg (CAPE_LL_TRACE, "ADBL", "sync list", "delete database item");

                res = adbl_sync_list__delete (self, trx, cursor_current->item, err);
                if (res)
                {
                    goto cleanup_exit;
                }
            }
        }
        else
        {
            if (r2)
            {
                cape_log_msg (CAPE_LL_TRACE, "ADBL", "sync list", "insert database item");

                res = adbl_sync_list__insert (self, trx, cursor_save->item, params_list, err);
                if (res)
                {
                    goto cleanup_exit;
                }
            }
            else
            {
                break;
            }
        }
    }

    res = CAPE_ERR_NONE;

cleanup_exit:
    
    cape_udc_cursor_del (&cursor_save);
    cape_udc_cursor_del (&cursor_current);
    
    return res;
}

//-----------------------------------------------------------------------------

int adbl_sync_list_run (AdblSyncList self, AdblTrx trx, CapeUdc list, CapeUdc params_list, const CapeString id_column, CapeErr err)
{
    int res;
    
    // local objects
    CapeUdc query_results = NULL;
    
    if (CAPE_UDC_LIST != cape_udc_type (list))
    {
        res = cape_err_set (err, CAPE_ERR_WRONG_VALUE, "ERR.INVALID_LIST");
        goto cleanup_exit;
    }

    {
        CapeString h = cape_json_to_s (params_list);
        
        cape_log_fmt (CAPE_LL_TRACE, "ADBL", "sync list", "fetch current items by: %s", h);
        
        cape_str_del (&h);
    }
    
    // fetch current entries
    {
        CapeUdc params = cape_udc_cp (params_list);
        CapeUdc values = cape_udc_new (CAPE_UDC_NODE, NULL);
        
        // return values
        cape_udc_add_n      (values, id_column            , 0);

        // execute the query
        query_results = adbl_trx_query (trx, self->table, &params, &values, err);
        if (query_results == NULL)
        {
            res = cape_err_code (err);
            goto cleanup_exit;
        }
    }
    
    cape_log_fmt (CAPE_LL_TRACE, "ADBL", "sync list", "current items found: %lu -> save items: %lu", cape_udc_size (query_results), cape_udc_size (list));

    // run update
    res = adbl_sync_list__process (self, trx, query_results, list, params_list, err);
    if (res)
    {
        goto cleanup_exit;
    }
    
    res = CAPE_ERR_NONE;
    
cleanup_exit:
    
    cape_udc_del (&query_results);
    return res;
}

//-----------------------------------------------------------------------------
