#include "prepare.h"

#include "adbl.h"
#include "adbl_types.h"

// cape includes
#include "stc/cape_stream.h"
#include "fmt/cape_json.h"
#include "sys/cape_log.h"

//-----------------------------------------------------------------------------

struct AdblPrepare_s
{
  MYSQL_STMT* stmt;              // will be transfered
  
  number_t columns_used;
  
  number_t params_used; 
  
  CapeUdc params;                // owned
  CapeUdc values;                // will be transfered
  
  AdblBindVars bindsParams;     // owned
  AdblBindVars bindsValues;     // will be transfered

  CapeString group_by;
  CapeString order_by;

  number_t limit;
  number_t offset;
};

//-----------------------------------------------------------------------------

AdblPrepare adbl_prepare_new (CapeUdc* p_params, CapeUdc* p_values, number_t limit, number_t offset, const CapeString group_by, const CapeString order_by)
{
  AdblPrepare self = CAPE_NEW(struct AdblPrepare_s);
  
  self->stmt = NULL;  
  self->values = NULL;
  self->params = NULL;

  self->group_by = cape_str_cp (group_by);
  self->order_by = cape_str_cp (order_by);

  self->limit = limit;
  self->offset = offset;

  // check all values
  if (p_values)
  {
    CapeUdc values = *p_values;
    
    self->values = cape_udc_new (CAPE_UDC_NODE, NULL);
    
    CapeUdcCursor* cursor = cape_udc_cursor_new (values, CAPE_DIRECTION_FORW);
    
    while (cape_udc_cursor_next (cursor))
    {
      CapeUdc item = cape_udc_cursor_ext (values, cursor);
      
      // the name of the column
      const CapeString name = cape_udc_name (item);
      
      // check for special column entries
      if (cape_str_equal (name, ADBL_SPECIAL__GROUP_BY))
      {
        CapeString h = cape_udc_s_mv (item, NULL);

        cape_str_replace_mv (&(self->group_by), &h);

        cape_udc_del (&item);
      }
      else if (cape_str_equal (name, ADBL_SPECIAL__ORDER_BY))
      {
        CapeString h = cape_udc_s_mv (item, NULL);

        cape_str_replace_mv (&(self->order_by), &h);

        cape_udc_del (&item);
      }
      else switch (cape_udc_type(item))
      {
        case CAPE_UDC_STRING:
        case CAPE_UDC_BOOL:
        case CAPE_UDC_FLOAT:
        case CAPE_UDC_DATETIME:
        case CAPE_UDC_NULL:
        case CAPE_UDC_NODE:
        {
          cape_udc_add (self->values, &item);
          break;
        }
        case CAPE_UDC_LIST:
        {
          cape_udc_add (self->values, &item);
          break;
        }
        case CAPE_UDC_NUMBER:
        {
          // check the value
          number_t val = cape_udc_n (item, ADBL_AUTO_INCREMENT);
          
          if (val == ADBL_AUTO_INCREMENT)
          {
            cape_udc_del (&item);
          }
          else if (val == ADBL_AUTO_SEQUENCE_ID)
          {
            cape_udc_del (&item);
          }
          else
          {
            cape_udc_add (self->values, &item);
          }

          break;
        }
        default:
        {
          cape_udc_del (&item);
          break;
        }
      }
      
    }
    
    cape_udc_cursor_del (&cursor);
    
    cape_udc_del (p_values);
  }
    
  // params are optional
  if (p_params)
  {
    self->params = *p_params;
    *p_params = NULL;
  }
  
  self->columns_used = 0;
  self->params_used = 0;
  
  self->bindsValues = NULL;
  self->bindsParams = NULL;
  
  return self;
}

//-----------------------------------------------------------------------------

int adbl_prepare_init (AdblPrepare self, AdblPvdSession session, MYSQL* mysql, CapeErr err)
{
  if (self->stmt)
  {
    // cleanup results
    mysql_stmt_free_result (self->stmt);
    
    // close old statement
    mysql_stmt_close (self->stmt);
  }
  
  self->stmt = mysql_stmt_init (mysql);
  if (self->stmt == NULL)
  {
    // gather error code
    unsigned int error_code = mysql_stmt_errno (self->stmt);
    
    cape_log_fmt (CAPE_LL_ERROR, "ADBL", "prepare init", "error seen: %i", error_code);    
    
    // use session error handling
    return adbl_check_error (session, error_code, err);
  }
  
  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

void adbl_prepare_del (AdblPrepare* p_self)
{
  if (*p_self)
  {
    AdblPrepare self = *p_self;
    
    cape_str_del (&(self->group_by));
    cape_str_del (&(self->order_by));
    
    cape_udc_del (&(self->params));
    cape_udc_del (&(self->values));
    
    if (self->bindsParams)
    {
      adbl_bindvars_del (&(self->bindsParams));
    }
    
    if (self->bindsValues)
    {
      adbl_bindvars_del (&(self->bindsValues));
    }
    
    if (self->stmt)
    {
      mysql_stmt_free_result (self->stmt);
      
      mysql_stmt_close (self->stmt);
    }
    
    CAPE_DEL(p_self, struct AdblPrepare_s);
  }
}

//-----------------------------------------------------------------------------

void adbl_prepare__replace_binds (AdblBindVars* p_to_replace, AdblBindVars with_replace)
{
  if (*p_to_replace)
  {
    adbl_bindvars_del (p_to_replace);
  }
  
  *p_to_replace = with_replace;
}

//-----------------------------------------------------------------------------

AdblPvdCursor adbl_prepare_to_cursor (AdblPrepare* p_self, CapeMutex mutex)
{
  AdblPrepare self = *p_self;
  
  AdblPvdCursor cursor = CAPE_NEW(struct AdblPvdCursor_s);
  
  cursor->binds = self->bindsValues;
  self->bindsValues = NULL;
  
  cursor->stmt = self->stmt;
  self->stmt = NULL;
  
  cursor->values = self->values;
  self->values = NULL;
  
  cursor->pos = 0;
  cursor->mutex = mutex;
  
  // cleanup
  adbl_prepare_del (p_self);
  
  return cursor;  
}

//-----------------------------------------------------------------------------

int adbl_prepare_binds_params (AdblPrepare self, CapeErr err)
{
  int res;
  
  if (self->params_used)  // optional
  {
    // create bindings for mysql prepared statement engine
    adbl_prepare__replace_binds (&(self->bindsParams), adbl_bindvars_new (self->params_used));
    
    // set bindings for mysql for all parameters
    res = adbl_bindvars_set_from_node (self->bindsParams, self->params, TRUE, err);
    if (res)
    {
      return res;
    }
    
    // try to bind all constraint values
    if (mysql_stmt_bind_param (self->stmt, adbl_bindvars_binds(self->bindsParams)) != 0)
    {
      return cape_err_set_fmt (err, CAPE_ERR_3RDPARTY_LIB, "%i (%s): %s", mysql_stmt_errno (self->stmt), mysql_stmt_sqlstate (self->stmt), mysql_stmt_error (self->stmt));
    }
  }
  
  return CAPE_ERR_NONE;  
}

//-----------------------------------------------------------------------------

int adbl_prepare_binds_values (AdblPrepare self, CapeErr err)
{
  int res;
  
  if (self->columns_used)  // optional
  {
    // create bindings for mysql prepared statement engine
    adbl_prepare__replace_binds (&(self->bindsValues), adbl_bindvars_new (self->columns_used));
    
    // set bindings for mysql for all parameters
    res = adbl_bindvars_set_from_node (self->bindsValues, self->values, FALSE, err);
    if (res)
    {
      return res;
    }
    
    // try to bind all constraint values
    if (mysql_stmt_bind_param (self->stmt, adbl_bindvars_binds(self->bindsValues)) != 0)
    {
      return cape_err_set_fmt (err, CAPE_ERR_3RDPARTY_LIB, "%i (%s): %s", mysql_stmt_errno (self->stmt), mysql_stmt_sqlstate (self->stmt), mysql_stmt_error (self->stmt));
    }
  }
  
  return CAPE_ERR_NONE;  
}

//-----------------------------------------------------------------------------

int adbl_prepare_binds_result (AdblPrepare self, CapeErr err)
{
  int res;
  
  // allocate bind buffer
  adbl_prepare__replace_binds (&(self->bindsValues), adbl_bindvars_new (self->columns_used));
  
  res = adbl_bindvars_add_from_node (self->bindsValues, self->values, err);
  if (res)
  {
    return res;
  }
  
  // try to bind result
  if (mysql_stmt_bind_result (self->stmt, adbl_bindvars_binds(self->bindsValues)) != 0)
  {
    return cape_err_set_fmt (err, CAPE_ERR_3RDPARTY_LIB, "%i (%s): %s", mysql_stmt_errno (self->stmt), mysql_stmt_sqlstate (self->stmt), mysql_stmt_error (self->stmt));
  }
  
  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

int adbl_prepare_binds_all (AdblPrepare self, CapeErr err)
{
  int res;
  number_t size = self->columns_used + self->params_used;
  
  // allocate bind buffer
  adbl_prepare__replace_binds (&(self->bindsValues), adbl_bindvars_new (size));

  if (self->columns_used)
  {
    res = adbl_bindvars_set_from_node (self->bindsValues, self->values, FALSE, err);
    if (res)
    {
      return res;
    }
  }
  
  if (self->params_used)
  {
    res = adbl_bindvars_set_from_node (self->bindsValues, self->params, TRUE, err);
    if (res)
    {
      return res;
    }
  }
  
  // try to bind result
  if (mysql_stmt_bind_param (self->stmt, adbl_bindvars_binds(self->bindsValues)) != 0)
  {
    return cape_err_set_fmt (err, CAPE_ERR_3RDPARTY_LIB, "%i (%s): %s", mysql_stmt_errno (self->stmt), mysql_stmt_sqlstate (self->stmt), mysql_stmt_error (self->stmt));
  }
  
  return CAPE_ERR_NONE;  
}

//-----------------------------------------------------------------------------

int adbl_prepare_execute (AdblPrepare self, AdblPvdSession session, CapeErr err)
{
  int res;
  
  // execute
  if (mysql_stmt_execute (self->stmt) != 0)
  {
    unsigned int error_code = mysql_stmt_errno (self->stmt);
    
    cape_log_fmt (CAPE_LL_ERROR, "ADBL", "prepare execute", "error seen: %i", error_code);    

    // try to figure out if the error was serious
    res = adbl_check_error (session, error_code, err);
    if (res == CAPE_ERR_CONTINUE)
    {
      return CAPE_ERR_CONTINUE;   // statement went wrong, but there is hope to make it right again
    }
    
    return cape_err_set_fmt (err, CAPE_ERR_3RDPARTY_LIB, "%i (%s): %s", error_code, mysql_stmt_sqlstate (self->stmt), mysql_stmt_error (self->stmt));
  }
  
  if (mysql_stmt_store_result (self->stmt) != 0)
  {
    unsigned int error_code = mysql_stmt_errno (self->stmt);
    
    cape_log_fmt (CAPE_LL_ERROR, "ADBL", "store result", "error seen: %i", error_code);    

    // try to figure out if the error was serious
    res = adbl_check_error (session, error_code, err);
    if (res == CAPE_ERR_CONTINUE)
    {
      return CAPE_ERR_CONTINUE;   // statement went wrong, but there is hope to make it right again
    }
    
    return cape_err_set_fmt (err, CAPE_ERR_3RDPARTY_LIB, "%i (%s): %s", mysql_stmt_errno (self->stmt), mysql_stmt_sqlstate (self->stmt), mysql_stmt_error (self->stmt));
  }
  
  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

int adbl_prepare_prepare (AdblPrepare self, AdblPvdSession session, CapeStream stream, CapeErr err)
{
  // debug output
  //cape_log_msg (CAPE_LL_TRACE, "ADBL", "mysql **SQL**", cape_stream_get (stream));

  // execute
  if (mysql_stmt_prepare (self->stmt, cape_stream_get (stream), cape_stream_size (stream)) != 0)
  {
    unsigned int error_code = mysql_stmt_errno (self->stmt);
    
    cape_log_fmt (CAPE_LL_ERROR, "ADBL", "prepare", "error seen: %i", error_code);    
    cape_log_fmt (CAPE_LL_ERROR, "ADBL", "prepare", "sql statement: %s", cape_stream_get (stream));

    // try to figure out if the error was serious
    int res = adbl_check_error (session, error_code, err);
    if (res == CAPE_ERR_CONTINUE)
    {
      cape_log_fmt (CAPE_LL_TRACE, "ADBL", "prepare", "trigger next cycle");    

      return CAPE_ERR_CONTINUE;   // statement went wrong, but there is hope to make it right again
    }
    
    return cape_err_set_fmt (err, CAPE_ERR_3RDPARTY_LIB, "%i (%s): %s", error_code, mysql_stmt_sqlstate (self->stmt), mysql_stmt_error (self->stmt));
  }

  return CAPE_ERR_NONE;
}

//-----------------------------------------------------------------------------

void adbl_pvd_append_columns__add (CapeStream stream, int ansi, const CapeString table, const CapeString column_name)
{
  if (ansi == TRUE)
  {
    cape_stream_append_str (stream, "\"" );
    cape_stream_append_str (stream, table );
    cape_stream_append_str (stream, "\".\"" );
    cape_stream_append_str (stream, column_name);
    cape_stream_append_str (stream, "\"" );
  }
  else
  {
    cape_stream_append_str (stream, table );
    cape_stream_append_str (stream, "." );
    cape_stream_append_str (stream, column_name);
  }
}

//-----------------------------------------------------------------------------

number_t adbl_pvd_append_columns (CapeStream stream, int ansi, int is_query, CapeUdc values, const char* table)
{
  number_t used = 0;
  CapeUdcCursor* cursor = cape_udc_cursor_new (values, CAPE_DIRECTION_FORW);
  
  while (cape_udc_cursor_next (cursor))
  {
    const CapeString column_name = cape_udc_name (cursor->item);
    if (column_name)
    {
      if (cursor->position > 0)
      {
        cape_stream_append_str (stream, ", ");
      }
      
      if (is_query) switch (cape_udc_type (cursor->item))
      {
        case CAPE_UDC_NUMBER:
        {
          switch (cape_udc_n (cursor->item, 0))
          {
            case ADBL_AGGREGATE_SUM:
            {
              cape_stream_append_str (stream, "SUM(" );
              adbl_pvd_append_columns__add (stream, ansi, table, column_name);
              cape_stream_append_str (stream, ")" );

              break;
            }
            case ADBL_AGGREGATE_COUNT:
            {
              cape_stream_append_str (stream, "COUNT(" );
              adbl_pvd_append_columns__add (stream, ansi, table, column_name);
              cape_stream_append_str (stream, ")" );
              
              break;
            }
            case ADBL_AGGREGATE_AVERAGE:
            {
              cape_stream_append_str (stream, "AVG(" );
              adbl_pvd_append_columns__add (stream, ansi, table, column_name);
              cape_stream_append_str (stream, ")" );

              break;
            }
            case ADBL_AGGREGATE_MAX:
            {
              cape_stream_append_str (stream, "MAX(" );
              adbl_pvd_append_columns__add (stream, ansi, table, column_name);
              cape_stream_append_str (stream, ")" );

              break;
            }
            case ADBL_AGGREGATE_MIN:
            {
              cape_stream_append_str (stream, "MIN(" );
              adbl_pvd_append_columns__add (stream, ansi, table, column_name);
              cape_stream_append_str (stream, ")" );

              break;
            }
            default:
            {
              adbl_pvd_append_columns__add (stream, ansi, table, column_name);
              break;
            }
          }
          
          break;
        }
        default:
        {
          adbl_pvd_append_columns__add (stream, ansi, table, column_name);
          break;
        }
      }
      else
      {
        adbl_pvd_append_columns__add (stream, ansi, table, column_name);
      }
      
      used++;
    }
  }
  
  cape_udc_cursor_del (&cursor);
  
  return used;
}

//-----------------------------------------------------------------------------

number_t adbl_pvd_append_update (CapeStream stream, int ansi, CapeUdc values, const char* table)
{
  number_t used = 0;
  CapeUdcCursor* cursor = cape_udc_cursor_new (values, CAPE_DIRECTION_FORW);
  
  while (cape_udc_cursor_next (cursor))
  {
    const CapeString column_name = cape_udc_name (cursor->item);
    
    if (column_name)
    {
      if (cursor->position > 0)
      {
        cape_stream_append_str (stream, ", ");
      }
      
      if (ansi == TRUE)
      {
        cape_stream_append_str (stream, "\"" );
        cape_stream_append_str (stream, table );
        cape_stream_append_str (stream, "\".\"" );
        cape_stream_append_str (stream, column_name);
        cape_stream_append_str (stream, "\" = ?" );
      }
      else
      {
        cape_stream_append_str (stream, table );
        cape_stream_append_str (stream, "." );
        cape_stream_append_str (stream, column_name);
        cape_stream_append_str (stream, " = ?" );
      }
      
      used++;
    }
  }
  
  cape_udc_cursor_del (&cursor);
  
  return used;
}

//-----------------------------------------------------------------------------

number_t adbl_prepare_append_values (CapeStream stream, CapeUdc values)
{
  number_t used = 0;
  
  CapeUdcCursor* cursor = cape_udc_cursor_new (values, CAPE_DIRECTION_FORW);
  
  while (cape_udc_cursor_next (cursor))
  {
    const CapeString param_name = cape_udc_name (cursor->item);
    
    if (param_name)
    {
      if (cursor->position > 0)
      {
        cape_stream_append_str (stream, ", ");
      }
      
      cape_stream_append_str (stream, "?" );
      
      used++;
    }
  }
  
  cape_udc_cursor_del (&cursor);
  
  return used;
}

//-----------------------------------------------------------------------------

void adbl_prepare_append_constraints__param (CapeStream stream, int ansi, const CapeString param_name, const CapeString table)
{
  if (ansi == TRUE)
  {
    cape_stream_append_str (stream, "\"" );
    cape_stream_append_str (stream, table );
    cape_stream_append_str (stream, "\".\"" );
    cape_stream_append_str (stream, param_name);
    cape_stream_append_c (stream, '\"');
  }
  else
  {
    cape_stream_append_str (stream, table );
    cape_stream_append_str (stream, "." );
    cape_stream_append_str (stream, param_name);   
  }
}

//-----------------------------------------------------------------------------

number_t adbl_prepare_append_constraints__node (CapeStream stream, int ansi, const CapeString param_name, const CapeString table, CapeUdcCursor* cursor)
{
  number_t ret = 0;
  
  switch (cape_udc_get_n (cursor->item, ADBL_SPECIAL__TYPE, 0))
  {
    case ADBL_TYPE__BETWEEN:
    {
      CapeUdc from_node = cape_udc_get (cursor->item, ADBL_SPECIAL__BETWEEN_FROM);
      CapeUdc to_node = cape_udc_get (cursor->item, ADBL_SPECIAL__BETWEEN_TO);
      
      if (from_node && to_node)
      {
        if (cursor->position > 0)
        {
          cape_stream_append_str (stream, " AND ");
        }
        
        adbl_prepare_append_constraints__param (stream, ansi, param_name, table);
        cape_stream_append_str (stream, " BETWEEN ? AND ?" );
        
        ret = 2;            
      }
      
      break;
    }
    case ADBL_TYPE__GREATER_THAN:
    {
      CapeUdc greater = cape_udc_get (cursor->item, ADBL_SPECIAL__VALUE);
      
      if (greater)
      {
        if (cursor->position > 0)
        {
          cape_stream_append_str (stream, " AND ");
        }
        
        adbl_prepare_append_constraints__param (stream, ansi, param_name, table);
        cape_stream_append_str (stream, " > ?" );
        
        ret = 1;
      }
      
      break;
    }
    case ADBL_TYPE__LESS_THAN:
    {
      CapeUdc greater = cape_udc_get (cursor->item, ADBL_SPECIAL__VALUE);
      
      if (greater)
      {
        if (cursor->position > 0)
        {
          cape_stream_append_str (stream, " AND ");
        }
        
        adbl_prepare_append_constraints__param (stream, ansi, param_name, table);
        cape_stream_append_str (stream, " < ?" );
        
        ret = 1;            
      }
      
      break;
    }
    case ADBL_TYPE__GREATER_EQUAL_THAN:
    {
      CapeUdc greater = cape_udc_get (cursor->item, ADBL_SPECIAL__VALUE);
      
      if (greater)
      {
        if (cursor->position > 0)
        {
          cape_stream_append_str (stream, " AND ");
        }
        
        adbl_prepare_append_constraints__param (stream, ansi, param_name, table);
        cape_stream_append_str (stream, " >= ?" );
        
        ret = 1;
      }
      
      break;
    }
    case ADBL_TYPE__LESS_EQUAL_THAN:
    {
      CapeUdc greater = cape_udc_get (cursor->item, ADBL_SPECIAL__VALUE);
      
      if (greater)
      {
        if (cursor->position > 0)
        {
          cape_stream_append_str (stream, " AND ");
        }
        
        adbl_prepare_append_constraints__param (stream, ansi, param_name, table);
        cape_stream_append_str (stream, " <= ?" );
        
        ret = 1;
      }
      
      break;
    }
  }
  
  return ret;
}

//-----------------------------------------------------------------------------

number_t adbl_prepare_append_constraints (CapeStream stream, int ansi, CapeUdc params, const char* table)
{
  number_t used = 0;

  CapeUdcCursor* cursor = cape_udc_cursor_new (params, CAPE_DIRECTION_FORW);
  
  while (cape_udc_cursor_next (cursor))
  {
    const CapeString param_name = cape_udc_name (cursor->item);
    
    if (param_name)
    {
      switch (cape_udc_type (cursor->item))
      {
        case CAPE_UDC_NODE:
        {
          used += adbl_prepare_append_constraints__node (stream, ansi, param_name, table, cursor);
          
          break;
        }
        case CAPE_UDC_NULL:
        {
          if (cursor->position > 0)
          {
            cape_stream_append_str (stream, " AND ");
          }
          
          adbl_prepare_append_constraints__param (stream, ansi, param_name, table);
          cape_stream_append_str (stream, " IS NULL" );
          
          break;
        }
        default:
        {
          if (cursor->position > 0)
          {
            cape_stream_append_str (stream, " AND ");
          }
          
          adbl_prepare_append_constraints__param (stream, ansi, param_name, table);
          cape_stream_append_str (stream, " = ?" );
          
          used++;
          
          break;
        }        
      }
    }
  }
  
  cape_udc_cursor_del (&cursor);
  
  return used;
}

//-----------------------------------------------------------------------------

number_t adbl_prepare_append_where_clause (CapeStream stream, int ansi, CapeUdc params, const char* table)
{
  if (params == NULL)
  {
    return 0;
  }
  
  cape_stream_append_str (stream, " WHERE ");
  
  return adbl_prepare_append_constraints (stream, ansi, params, table);
}

//-----------------------------------------------------------------------------

void adbl_prepare_append_group_by (CapeStream stream, int ansi, const CapeString group_by)
{
  if (group_by)
  {
    cape_stream_append_str (stream, " GROUP BY ");
    cape_stream_append_str (stream, group_by);
  }
}

//-----------------------------------------------------------------------------

void adbl_prepare_append_order_by (CapeStream stream, int ansi, const CapeString order_by)
{
  if (order_by)
  {
    if (order_by[0] == '-')
    {
      cape_stream_append_str (stream, " ORDER BY ");
      cape_stream_append_str (stream, order_by + 1);
      cape_stream_append_str (stream, " DESC");
    }
    else
    {
      cape_stream_append_str (stream, " ORDER BY ");
      cape_stream_append_str (stream, order_by);
      cape_stream_append_str (stream, " ASC");
    }
  }
}

//-----------------------------------------------------------------------------

void adbl_prepare_append_limit (CapeStream stream, int ansi, number_t limit)
{
  if (limit > 0)
  {
    cape_stream_append_str (stream, " LIMIT ");
    cape_stream_append_n (stream, limit);
  }
}

//-----------------------------------------------------------------------------

void adbl_prepare_append_offset (CapeStream stream, int ansi, number_t offset)
{
  if (offset > 0)
  {
    cape_stream_append_str (stream, " OFFSET ");
    cape_stream_append_n (stream, offset);
  }
}

//-----------------------------------------------------------------------------

void adbl_pvd_append_table (CapeStream stream, int ansi, const char* schema, const char* table)
{
  // schema and table name
  if (ansi == TRUE)
  {
    cape_stream_append_str (stream, "\"" );
    cape_stream_append_str (stream, schema );
    cape_stream_append_str (stream, "\".\"" );
    cape_stream_append_str (stream, table );
    cape_stream_append_str (stream, "\"" );
  }
  else
  {
    cape_stream_append_str (stream, schema );
    cape_stream_append_str (stream, "." );
    cape_stream_append_str (stream, table );
  }
}

//-----------------------------------------------------------------------------

int adbl_prepare_statement_select (AdblPrepare self, AdblPvdSession session, const char* schema, const char* table, int ansi, CapeErr err)
{
  int res;
  
  CapeStream stream = cape_stream_new ();
  
  cape_stream_append_str (stream, "SELECT ");
  
  // create columns for mysql for all parameters
  self->columns_used = adbl_pvd_append_columns (stream, ansi, TRUE, self->values, table);
  
  cape_stream_append_str (stream, " FROM ");
  
  adbl_pvd_append_table (stream, ansi, schema, table);
  
  self->params_used = adbl_prepare_append_where_clause (stream, ansi, self->params, table);
  
  adbl_prepare_append_group_by (stream, ansi, self->group_by);

  adbl_prepare_append_order_by (stream, ansi, self->order_by);

  adbl_prepare_append_limit (stream, ansi, self->limit);

  adbl_prepare_append_offset (stream, ansi, self->offset);

  res = adbl_prepare_prepare (self, session, stream, err);
  
  cape_stream_del (&stream);
  
  return res;
}

//-----------------------------------------------------------------------------

int adbl_prepare_statement_insert (AdblPrepare self, AdblPvdSession session, const char* schema, const char* table, int ansi, CapeErr err)
{
  int res;
  
  CapeStream stream = cape_stream_new ();

  cape_stream_append_str (stream, "INSERT INTO ");
  
  adbl_pvd_append_table (stream, ansi, schema, table);

  cape_stream_append_str (stream, " (");
  
  // create columns for mysql for all parameters
  self->columns_used = adbl_pvd_append_columns (stream, ansi, FALSE, self->values, table);
  
  cape_stream_append_str (stream, ") VALUES (");

  adbl_prepare_append_values (stream, self->values);

  cape_stream_append_str (stream, ")");
  
  res = adbl_prepare_prepare (self, session, stream, err);

  cape_stream_del (&stream);
  
  return res;
}

//-----------------------------------------------------------------------------

int adbl_prepare_statement_delete (AdblPrepare self, AdblPvdSession session, const char* schema, const char* table, int ansi, CapeErr err)
{
  int res;
  
  CapeStream stream = cape_stream_new ();

  cape_stream_append_str (stream, "DELETE FROM ");

  adbl_pvd_append_table (stream, ansi, schema, table);

  self->params_used = adbl_prepare_append_where_clause (stream, ansi, self->params, table);
  
  res = adbl_prepare_prepare (self, session, stream, err);
  
  cape_stream_del (&stream);

  return res;
}

//-----------------------------------------------------------------------------

int adbl_prepare_statement_update (AdblPrepare self, AdblPvdSession session, const char* schema, const char* table, int ansi, CapeErr err)
{
  int res;
  
  CapeStream stream = cape_stream_new ();
  
  cape_stream_append_str (stream, "UPDATE ");
  
  adbl_pvd_append_table (stream, ansi, schema, table);
  
  cape_stream_append_str (stream, " SET ");

  self->columns_used = adbl_pvd_append_update (stream, ansi, self->values, table);
  
  self->params_used = adbl_prepare_append_where_clause (stream, ansi, self->params, table);
  
  res = adbl_prepare_prepare (self, session, stream, err);
  
  cape_stream_del (&stream);
  
  return res;
}

//-----------------------------------------------------------------------------

int adbl_prepare_statement_setins (AdblPrepare self, AdblPvdSession session, const char* schema, const char* table, int ansi, CapeErr err)
{
  int res;
  
  CapeStream stream = cape_stream_new ();
  
  // due we need to consider the params aswell we just the params
  // for the first part and values for the second
  // params can be merged by values
  cape_udc_merge_cp (self->params, self->values);
  
  // now we need to swap, because the correct order is values before params
  {
    CapeUdc h = self->params;

    self->params = self->values;
    self->values = h;
  }
  
  cape_stream_append_str (stream, "INSERT INTO ");
  
  adbl_pvd_append_table (stream, ansi, schema, table);
  
  cape_stream_append_str (stream, " (");
  
  // create columns for mysql for all parameters
  self->columns_used = adbl_pvd_append_columns (stream, ansi, FALSE, self->values, table);
  
  cape_stream_append_str (stream, ") VALUES (");
  
  adbl_prepare_append_values (stream, self->values);
  
  cape_stream_append_str (stream, ")");
  
  // check if we do have parameters
  if (cape_udc_size (self->params) > 0)
  {
    cape_stream_append_str (stream, " ON DUPLICATE KEY UPDATE id=LAST_INSERT_ID(id), ");
    
    self->params_used = adbl_pvd_append_update (stream, ansi, self->params, table);
  }
  else
  {
    cape_stream_append_str (stream, " ON DUPLICATE KEY UPDATE id=LAST_INSERT_ID(id)");
    
    self->params_used = 0;
  }
  
  res = adbl_prepare_prepare (self, session, stream, err);
  
  cape_stream_del (&stream);
    
  return res;  
}

//-----------------------------------------------------------------------------

int adbl_prepare_statement_atodec (AdblPrepare self, AdblPvdSession session, const char* schema, const char* table, int ansi, const CapeString atomic_value, CapeErr err)
{
  int res;
  
  CapeStream stream = cape_stream_new ();

  cape_stream_append_str (stream, "UPDATE ");
  
  adbl_pvd_append_table (stream, ansi, schema, table);
  
  cape_stream_append_str (stream, " SET ");

  adbl_prepare_append_constraints__param (stream, ansi, atomic_value, table);

  cape_stream_append_str (stream, " = LAST_INSERT_ID(");
  adbl_prepare_append_constraints__param (stream, ansi, atomic_value, table);
  cape_stream_append_str (stream, " - 1)");
  
  self->params_used = adbl_prepare_append_where_clause (stream, ansi, self->params, table);
  
  res = adbl_prepare_prepare (self, session, stream, err);
  
  cape_stream_del (&stream);
    
  return res;
}

//-----------------------------------------------------------------------------

int adbl_prepare_statement_atoinc (AdblPrepare self, AdblPvdSession session, const char* schema, const char* table, int ansi, const CapeString atomic_value, CapeErr err)
{
  int res;
  
  CapeStream stream = cape_stream_new ();

  cape_stream_append_str (stream, "UPDATE ");
  
  adbl_pvd_append_table (stream, ansi, schema, table);
  
  cape_stream_append_str (stream, " SET ");

  adbl_prepare_append_constraints__param (stream, ansi, atomic_value, table);

  cape_stream_append_str (stream, " = LAST_INSERT_ID(");
  adbl_prepare_append_constraints__param (stream, ansi, atomic_value, table);
  cape_stream_append_str (stream, " + 1)");
  
  self->params_used = adbl_prepare_append_where_clause (stream, ansi, self->params, table);
  
  res = adbl_prepare_prepare (self, session, stream, err);
  
  cape_stream_del (&stream);
    
  return res;
}

//-----------------------------------------------------------------------------

int adbl_prepare_statement_atoor (AdblPrepare self, AdblPvdSession session, const char* schema, const char* table, int ansi, const CapeString atomic_value, number_t or_val, CapeErr err)
{
  int res;
  
  CapeStream stream = cape_stream_new ();

  cape_stream_append_str (stream, "UPDATE ");
  
  adbl_pvd_append_table (stream, ansi, schema, table);
  
  cape_stream_append_str (stream, " SET ");

  adbl_prepare_append_constraints__param (stream, ansi, atomic_value, table);

  cape_stream_append_str (stream, " = LAST_INSERT_ID(");
  adbl_prepare_append_constraints__param (stream, ansi, atomic_value, table);
  cape_stream_append_str (stream, " | ");
  cape_stream_append_n (stream, or_val);
  cape_stream_append_str (stream, ")");

  self->params_used = adbl_prepare_append_where_clause (stream, ansi, self->params, table);
  
  res = adbl_prepare_prepare (self, session, stream, err);
  
  cape_stream_del (&stream);
    
  return res;
}

//-----------------------------------------------------------------------------


