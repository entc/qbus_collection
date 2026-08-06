#include "adbl_tools.h"

// cape includes
#include <sys/cape_log.h>
#include <fmt/cape_json.h>

//-----------------------------------------------------------------------------

struct AdblSyncList_s
{
    CapeString table;
    CapeString id_column;
    
    void* user_ptr;
    fct_adbl_sync_list_values on_prepare_values;
    fct_adbl_sync_list_update on_update;
    fct_adbl_sync_list_insert on_insert;
    fct_adbl_sync_list_delete on_delete;
};

//-----------------------------------------------------------------------------

AdblSyncList adbl_sync_list_new (const CapeString table, const CapeString id_column, void* user_ptr, fct_adbl_sync_list_values on_values, fct_adbl_sync_list_update on_update, fct_adbl_sync_list_insert on_insert, fct_adbl_sync_list_delete on_delete)
{
    AdblSyncList self = CAPE_NEW (struct AdblSyncList_s);
    
    self->table = cape_str_cp (table);
    self->id_column = cape_str_cp (id_column ? id_column : "id");
    
    self->user_ptr = user_ptr;
    self->on_prepare_values = on_values;
    
    self->on_update = on_update;
    self->on_insert = on_insert;
    self->on_delete = on_delete;

    return self;
}

//-----------------------------------------------------------------------------

void adbl_sync_list_del (AdblSyncList* p_self)
{
    if ((p_self != NULL) && (*p_self != NULL))
    {
        AdblSyncList self = *p_self;
      
        cape_str_del (&(self->id_column));
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
    
    if (self->on_update)
    {
        res = self->on_update (self->user_ptr, trx, cape_udc_get_n (item_current, self->id_column, 0), item_save, err);
        if (res)
        {
            goto cleanup_exit;
        }
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
    
    // local objects
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
    
    if (self->on_insert)
    {
        res = self->on_insert (self->user_ptr, trx, id, item_save, err);
        if (res)
        {
            goto cleanup_exit;
        }
    }
    
    res = CAPE_ERR_NONE;

cleanup_exit:
    
    cape_udc_del (&values);
    return res;
}

//-----------------------------------------------------------------------------

static int adbl_sync_list__delete (AdblSyncList self, AdblTrx trx, CapeUdc item_current, CapeErr err)
{
    int res;
    
    // local objects
    CapeUdc params = cape_udc_cp (item_current);

    if (self->on_delete)
    {
        res = self->on_delete (self->user_ptr, trx, cape_udc_get_n (item_current, self->id_column, 0), err);
        if (res)
        {
            goto cleanup_exit;
        }
    }

    res = adbl_trx_delete (trx, self->table, &params, err);
    
cleanup_exit:
    
    cape_udc_del (&params);
    return res;
}

//-----------------------------------------------------------------------------

static int adbl_sync_list__sync (AdblSyncList self, AdblTrx trx, CapeUdc list_current, CapeUdc list_save, CapeUdc params_list, CapeErr err)
{
    int res;
    
    // local objects
    CapeUdcCursor* cursor_current = cape_udc_cursor_new (list_current, CAPE_DIRECTION_FORW);
    CapeUdcCursor* cursor_save = cape_udc_cursor_new (list_save, CAPE_DIRECTION_FORW);

    // lists for actions outside the while loop
    CapeList list_insert = cape_list_new (NULL);
    CapeList list_delete = cape_list_new (NULL);
    
    CapeListCursor* cursor_insert = NULL;
    CapeListCursor* cursor_delete = NULL;

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
                cape_list_push_back (list_delete, cursor_current->item);
            }
        }
        else
        {
            if (r2)
            {
                cape_list_push_back (list_insert, cursor_save->item);
            }
            else
            {
                break;
            }
        }
    }
    
    // insert new items
    cursor_insert = cape_list_cursor_new (list_insert, CAPE_DIRECTION_FORW);

    while (cape_list_cursor_next (cursor_insert))
    {
        cape_log_msg (CAPE_LL_TRACE, "ADBL", "sync list", "insert database item");

        res = adbl_sync_list__insert (self, trx, cape_list_node_data (cursor_insert->node), params_list, err);
        if (res)
        {
            goto cleanup_exit;
        }
    }
    
    // delete items
    cursor_delete = cape_list_cursor_new (list_delete, CAPE_DIRECTION_FORW);

    while (cape_list_cursor_next (cursor_delete))
    {
        cape_log_msg (CAPE_LL_TRACE, "ADBL", "sync list", "delete database item");

        res = adbl_sync_list__delete (self, trx, cape_list_node_data (cursor_delete->node), err);
        if (res)
        {
            goto cleanup_exit;
        }
    }

    res = CAPE_ERR_NONE;

cleanup_exit:
    
    cape_list_cursor_del (&cursor_insert);
    cape_list_cursor_del (&cursor_delete);

    cape_list_del (&list_insert);
    cape_list_del (&list_delete);

    cape_udc_cursor_del (&cursor_save);
    cape_udc_cursor_del (&cursor_current);
    
    return res;
}

//-----------------------------------------------------------------------------

int adbl_sync_list_run (AdblSyncList self, AdblTrx trx, CapeUdc list, CapeUdc params_list, CapeErr err)
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
        cape_udc_add_n      (values, self->id_column            , 0);

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
    res = adbl_sync_list__sync (self, trx, query_results, list, params_list, err);
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
