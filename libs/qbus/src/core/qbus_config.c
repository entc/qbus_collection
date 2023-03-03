#include "qbus_config.h"
#include "qbus_pvd.h"

// cape includes
#include <sys/cape_file.h>
#include <sys/cape_log.h>
#include <sys/cape_thread.h>
#include <fmt/cape_args.h>
#include <fmt/cape_json.h>
#include <fmt/cape_tokenizer.h>

//-----------------------------------------------------------------------------

struct QBusConfig_s
{
  CapeString name;
  CapeString config_file;
  
  CapeUdc config;
  
  CapeUdc pvds;                   // a list for all PVDs
};

//-----------------------------------------------------------------------------

QBusConfig qbus_config_new (const char* module_origin)
{
  QBusConfig self = CAPE_NEW (struct QBusConfig_s);

  self->name = cape_str_cp (module_origin);
  cape_str_to_upper (self->name);

  self->config = cape_udc_new (CAPE_UDC_NODE, NULL);
  self->config_file = NULL;
  
  self->pvds = cape_udc_new (CAPE_UDC_LIST, NULL);
  
  return self;
}

//-----------------------------------------------------------------------------

void qbus_config_del (QBusConfig* p_self)
{
  if (*p_self)
  {
    QBusConfig self = *p_self;
    
    cape_udc_del (&(self->pvds));
    cape_udc_del (&(self->config));

    cape_str_del (&(self->config_file));
    cape_str_del (&(self->name));
    
    CAPE_DEL (p_self, struct QBusConfig_s);
  }
}

//-----------------------------------------------------------------------------

void qbus_config_load__find_file (QBusConfig self, CapeErr err)
{
  CapeString filename = cape_str_catenate_2 (self->name, ".json");

  self->config_file = cape_args_config_file ("etc", filename);
  
  cape_str_del (&filename);
}

//-----------------------------------------------------------------------------

CapeUdc qbus_config__load_local (QBusConfig self)
{
  CapeUdc ret = NULL;
  CapeErr err = cape_err_new ();

  // create or find the config file
  qbus_config_load__find_file (self, err);
  
  if (self->config_file)
  {
    // try to load
    ret = cape_json_from_file (self->config_file, err);
  }
  
  if (cape_err_code (err))
  {
    // dump error text
    cape_log_msg (CAPE_LL_ERROR, self->name, "config load", cape_err_text (err));
  }
  
  cape_err_del (&err);
  return ret;
}

//-----------------------------------------------------------------------------

CapeUdc qbus_config__load_global (QBusConfig self)
{
  CapeUdc ret = NULL;

  // local objects
  CapeErr err = cape_err_new ();
  CapeString global_config_file = cape_args_config_file ("etc", "qbus_default.json");

  if (global_config_file)
  {
    // try to load
    ret = cape_json_from_file (global_config_file, err);
  }
  
  if (cape_err_code (err))
  {
    // dump error text
    cape_log_msg (CAPE_LL_ERROR, self->name, "config load", cape_err_text (err));
  }

  cape_str_del (&global_config_file);
  cape_err_del (&err);
  
  return ret;
}

//-----------------------------------------------------------------------------

void qbus_config__save (QBusConfig self, CapeUdc config)
{
  // try to load
  if (self->config && cape_udc_size (config))
  {
    int res;
    CapeErr err = cape_err_new ();
    
    // TODO:
    // create a nice output
    res = cape_json_to_file (self->config_file, config, err);
    if (res)
    {
      // dump error text
      cape_log_fmt (CAPE_LL_ERROR, self->name, "config save: %s", cape_err_text (err), self->config_file);
    }
    
    cape_err_del (&err);
  }
}

//-----------------------------------------------------------------------------

void qbus_config_set_args (QBusConfig self, CapeUdc* p_args)
{
  cape_udc_merge_mv (self->config, p_args);
}

//-----------------------------------------------------------------------------

int qbus_config_init (QBusConfig self, CapeErr err)
{
  int res;
  
  {
    // load config from local file
    CapeUdc config_part = qbus_config__load_local (self);
    
    if (config_part)
    {
      // save the config back
      qbus_config__save (self, config_part);
      
      cape_udc_merge_mv (self->config, &config_part);
    }
  }

  {
    // load config from a global file
    CapeUdc config_part = qbus_config__load_global (self);
    
    if (config_part)
    {
      cape_udc_merge_mv (self->config, &config_part);
    }
  }

  // filelogging
  /*
  {
    CapeUdc arg_l = cape_udc_get (self->config, "log_file");
    
    if (arg_l) switch (cape_udc_type (arg_l))
    {
      case CAPE_UDC_STRING:
      {
        log = cape_log_new (cape_udc_s (arg_l, NULL));
        break;
      }
    }
  }
   */

  // debug output
  {
    // use this as default
    CapeLogLevel log_level = CAPE_LL_INFO;
    
    CapeUdc arg_d = cape_udc_get (self->config, "log_level");
    
    if (arg_d) switch (cape_udc_type (arg_d))
    {
      case CAPE_UDC_STRING:
      {
        log_level = cape_log_level_from_s (cape_udc_s (arg_d, NULL), log_level);
        break;
      }
      case CAPE_UDC_NUMBER:
      {
        log_level = cape_udc_n (arg_d, log_level);
        break;
      }
    }
    
    // adjust the global log level
    cape_log_set_level (log_level);
  }
  
  // debug
  {
    CapeString h = cape_json_to_s (self->config);
    
    cape_log_fmt (CAPE_LL_INFO, self->name, "qbus_instance", "params: %s", h);
    
    cape_str_del (&h);
  }
  
  res = CAPE_ERR_NONE;
  
exit_and_cleanup:
  
  return res;
}

//-----------------------------------------------------------------------------

void qbus_check_param__string (CapeUdc data, const CapeUdc string_param, number_t mode)
{
  const CapeString h = cape_udc_s (string_param, NULL);
  if (h)
  {
    CapeList options = cape_tokenizer_buf (h, strlen(h), ':');
    
    if (cape_list_size(options) == 3)
    {
      CapeUdc n = cape_udc_new (CAPE_UDC_NODE, NULL);
      
      CapeListCursor* cursor = cape_list_cursor_create (options, CAPE_DIRECTION_FORW);
      
      if (cape_list_cursor_next (cursor))
      {
        const CapeString type = cape_list_node_data (cursor->node);
        
        if (cape_str_equal (type, "socket"))
        {
          cape_udc_add_s_cp (n, "type", "tcp");
        }
        else
        {
          cape_udc_add_s_cp (n, "type", type);
        }
      }
      
      if (cape_list_cursor_next (cursor))
      {
        cape_udc_add_s_cp (n, "host", cape_list_node_data (cursor->node));
      }
      
      if (cape_list_cursor_next (cursor))
      {
        cape_udc_add_n (n, "port", strtol (cape_list_node_data (cursor->node), NULL, 10));
      }
      
      cape_udc_add_n (n, "mode", mode);
      
      cape_list_cursor_destroy (&cursor);
      
      cape_udc_add (data, &n);
    }
    
    cape_list_del (&options);
  }
}

//-----------------------------------------------------------------------------

void qbus_config__check_param (CapeUdc data, const CapeUdc param, number_t mode)
{
  switch (cape_udc_type (param))
  {
    case CAPE_UDC_NODE:
    {
      CapeUdc h = cape_udc_cp (param);
      cape_udc_add (data, &h);
      break;
    }
    case CAPE_UDC_LIST:
    {
      CapeUdcCursor* cursor = cape_udc_cursor_new (param, CAPE_DIRECTION_FORW);
      
      while (cape_udc_cursor_next (cursor))
      {
        qbus_config__check_param (data, cursor->item, mode);
      }
      
      cape_udc_cursor_del (&cursor);
      break;
    }
    case CAPE_UDC_STRING:
    {
      qbus_check_param__string (data, param, mode);
      break;
    }
  }
}

//-----------------------------------------------------------------------------

CapeUdc qbus_config_get_pvds (QBusConfig self)
{
  // check for remotes (will be depricated)
  {
    CapeUdc arg_r = cape_udc_get (self->config, "d");
    if (arg_r)
    {
      qbus_config__check_param (self->pvds, arg_r, QBUS_PVD_MODE_CLIENT);
    }
  }
  
  // check for binds (will be depricated)
  {
    CapeUdc arg_b = cape_udc_get (self->config, "b");
    if (arg_b)
    {
      qbus_config__check_param (self->pvds, arg_b, QBUS_PVD_MODE_LISTEN);
    }
  }
  
  return self->pvds;
}

//-----------------------------------------------------------------------------

CapeUdc qbus_config_get_logger (QBusConfig self)
{
  return cape_udc_get (self->config, "logger");
}

//-----------------------------------------------------------------------------

number_t qbus_config_get_threads (QBusConfig self)
{
  return cape_udc_get_n (self->config, "threads", cape_thread_concurrency () * 2);
}

//-----------------------------------------------------------------------------

const CapeString qbus_config_get_name (QBusConfig self)
{
  return self->name;
}

//-----------------------------------------------------------------------------

const CapeString qbus_config_s (QBusConfig self, const char* name, const CapeString default_val)
{
  CapeUdc config_node;
  
  if (self->config == NULL)
  {
    return default_val;
  }
  
  // search for UDC
  config_node = cape_udc_get (self->config, name);
  
  if (config_node)
  {
    return cape_udc_s (config_node, default_val);
  }
  else
  {
    cape_udc_add_s_cp (self->config, name, default_val);

    return default_val;
  }
}

//-----------------------------------------------------------------------------

CapeUdc qbus_config_node (QBusConfig self, const char* name)
{
  CapeUdc config_node;

  if (self->config == NULL)
  {
    return NULL;
  }
  
  // search for UDC
  config_node = cape_udc_get (self->config, name);
  
  if (config_node)
  {
    return config_node;
  }
  else
  {
    return cape_udc_add_node (self->config, name);
  }
}

//-----------------------------------------------------------------------------

number_t qbus_config_n (QBusConfig self, const char* name, number_t default_val)
{
  CapeUdc config_node;
  
  if (self->config == NULL)
  {
    return default_val;
  }
  
  // search for UDC
  config_node = cape_udc_get (self->config, name);
  
  if (config_node)
  {
    return cape_udc_n (config_node, default_val);
  }
  else
  {    
    cape_udc_add_n (self->config, name, default_val);

    return default_val;
  }
}

//-----------------------------------------------------------------------------

double qbus_config_f (QBusConfig self, const char* name, double default_val)
{
  CapeUdc config_node;
  
  if (self->config == NULL)
  {
    return default_val;
  }
  
  // search for UDC
  config_node = cape_udc_get (self->config, name);
  
  if (config_node)
  {
    return cape_udc_f (config_node, default_val);
  }
  else
  {    
    cape_udc_add_f (self->config, name, default_val);
    
    return default_val;
  }
}

//-----------------------------------------------------------------------------

int qbus_config_b (QBusConfig self, const char* name, int default_val)
{
  CapeUdc config_node;
  
  if (self->config == NULL)
  {
    return default_val;
  }
  
  // search for UDC
  config_node = cape_udc_get (self->config, name);
  
  if (config_node)
  {
    return cape_udc_b (config_node, default_val);
  }
  else
  {    
    cape_udc_add_b (self->config, name, default_val);
    
    return default_val;
  }
}

//-----------------------------------------------------------------------------
