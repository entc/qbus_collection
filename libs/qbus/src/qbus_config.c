#include "qbus_config.h"

// cape includes
#include <sys/cape_log.h>
#include <fmt/cape_args.h>
#include <fmt/cape_json.h>

//-----------------------------------------------------------------------------

struct QBusConfig_s
{
  CapeString module_name;     // owned
  CapeUdc config_node;        // config
  CapeUdc config_part;
  CapeString config_file;     // the use config file
};

//-----------------------------------------------------------------------------

void qbus_config_load (QBusConfig self)
{
  CapeErr err = cape_err_new ();

  if (self->config_file)
  {
    // try to load
    cape_json_from_file (self->config_file, err);
  }

  if (cape_err_code (err))
  {
    // dump error text
    cape_log_msg (CAPE_LL_ERROR, self->module_name, "config load", cape_err_text (err));
  }

  cape_err_del (&err);
}

//-----------------------------------------------------------------------------

QBusConfig qbus_config_new (const CapeString module_name)
{
  QBusConfig self = CAPE_NEW (struct QBusConfig_s);
  
  self->module_name = cape_str_cp (module_name);
  self->config_node = cape_udc_new (CAPE_UDC_NODE, NULL);
  self->config_part = NULL;
  self->config_file = NULL;

  return self;
}

//-----------------------------------------------------------------------------

void qbus_config_del (QBusConfig* p_self)
{
  if (*p_self)
  {
    QBusConfig self = *p_self;
    
    cape_udc_del (&(self->config_part));
    cape_udc_del (&(self->config_node));
    cape_str_del (&(self->config_file));
    cape_str_del (&(self->module_name));

    CAPE_DEL (p_self, struct QBusConfig_s);
  }
}

//-----------------------------------------------------------------------------

CapeUdc qbus_config_load__local (QBusConfig self)
{
  CapeUdc ret = NULL;
  CapeErr err = cape_err_new ();
  
  if (self->config_file)
  {
    // try to load
    ret = cape_json_from_file (self->config_file, err);
  }
  
  if (cape_err_code (err))
  {
    // dump error text
    cape_log_msg (CAPE_LL_ERROR, self->module_name, "config load", cape_err_text (err));
  }
  
  cape_err_del (&err);
  return ret;
}

//-----------------------------------------------------------------------------

CapeUdc qbus_config_load__global (QBusConfig self)
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
    cape_log_msg (CAPE_LL_ERROR, self->module_name, "config load", cape_err_text (err));
  }

  cape_str_del (&global_config_file);
  cape_err_del (&err);
  
  return ret;
}

//-----------------------------------------------------------------------------

void qbus_config_save (QBusConfig self)
{
  // try to save
  if (self->config_part && cape_udc_size (self->config_part))
  {
    int res;
    CapeErr err = cape_err_new ();
    
    res = cape_json_to_file (self->config_file, self->config_part, TRUE, err);
    if (res)
    {
      // dump error text
      cape_log_fmt (CAPE_LL_ERROR, self->module_name, "config save: %s", cape_err_text (err), self->config_file);
    }
    
    cape_err_del (&err);
  }
}


//-----------------------------------------------------------------------------

void qbus_config_init (QBusConfig self, int argc, char *argv[])
{
  // local objects
  CapeUdc args = NULL;

  // convert program arguments into a node with parameters
  args = cape_args_from_args (argc, argv, NULL);

  // replace name from the parameters
  cape_str_replace_cp (&(self->module_name), cape_udc_get_s (args, "n", self->module_name));

  if (args)
  {
    cape_udc_merge_mv (self->config_node, &args);
  }

  {
    CapeString filename = cape_str_catenate_2 (self->module_name, ".json");

    self->config_file = cape_args_config_file ("etc", filename);
    
    cape_str_del (&filename);
  }

  {
    // load config from local file
    self->config_part = qbus_config_load__local (self);

    if (self->config_part)
    {
      // save the config back
      qbus_config_save (self);

      cape_udc_merge_cp (self->config_node, self->config_part);
    }
  }

  {
    // load config from a global file
    CapeUdc config_part = qbus_config_load__global (self);

    if (config_part)
    {
      cape_udc_merge_mv (self->config_node, &config_part);
    }
  }

  // debug output
  {
    // use this as default
    CapeLogLevel log_level = CAPE_LL_INFO;
    
    CapeUdc arg_d = cape_udc_get (self->config_node, "log_level");
    
    if (arg_d) switch (cape_udc_type (arg_d))
    {
      case CAPE_UDC_STRING:
      {
        log_level = cape_log_level_from_s (cape_udc_s (arg_d, NULL), log_level);
        break;
      }
      case CAPE_UDC_NUMBER:
      {
        log_level = (CapeLogLevel)cape_udc_n (arg_d, log_level);
        break;
      }
    }
    
    // adjust the global log level
    cape_log_set_level (log_level);
  }

  // debug
  {
    CapeString h = cape_json_to_s (self->config_node);
    
    cape_log_fmt (CAPE_LL_INFO, self->module_name, "qbus_instance", "params: %s", h);
    
    cape_str_del (&h);
  }
}

//-----------------------------------------------------------------------------

const CapeString qbus_config_name (QBusConfig self)
{
  return self->module_name;
}

//-----------------------------------------------------------------------------

const CapeString qbus_config_s (QBusConfig self, const char* name, const CapeString default_val)
{
  CapeUdc config_node;
  
  if (self->config_node == NULL)
  {
    return default_val;
  }
  
  // search for UDC
  config_node = cape_udc_get (self->config_node, name);
  
  if (config_node)
  {
    return cape_udc_s (config_node, default_val);
  }
  else
  {
    cape_udc_add_s_cp (self->config_node, name, default_val);

    return default_val;
  }
}

//-----------------------------------------------------------------------------

CapeUdc qbus_config_node (QBusConfig self, const char* name)
{
  CapeUdc config_node;

  if (self->config_node == NULL)
  {
    return NULL;
  }
  
  // search for UDC
  config_node = cape_udc_get (self->config_node, name);
  
  if (config_node)
  {
    return config_node;
  }
  else
  {
    return cape_udc_add_node (self->config_node, name);
  }
}

//-----------------------------------------------------------------------------

number_t qbus_config_n (QBusConfig self, const char* name, number_t default_val)
{
  CapeUdc config_node;
  
  if (self->config_node == NULL)
  {
    return default_val;
  }
  
  // search for UDC
  config_node = cape_udc_get (self->config_node, name);
  
  if (config_node)
  {
    return cape_udc_n (config_node, default_val);
  }
  else
  {
    cape_udc_add_n (self->config_node, name, default_val);

    return default_val;
  }
}

//-----------------------------------------------------------------------------

double qbus_config_f (QBusConfig self, const char* name, double default_val)
{
  CapeUdc config_node;
  
  if (self->config_node == NULL)
  {
    return default_val;
  }
  
  // search for UDC
  config_node = cape_udc_get (self->config_node, name);
  
  if (config_node)
  {
    return cape_udc_f (config_node, default_val);
  }
  else
  {
    cape_udc_add_f (self->config_node, name, default_val);
    
    return default_val;
  }
}

//-----------------------------------------------------------------------------

int qbus_config_b (QBusConfig self, const char* name, int default_val)
{
  CapeUdc config_node;
  
  if (self->config_node == NULL)
  {
    return default_val;
  }
  
  // search for UDC
  config_node = cape_udc_get (self->config_node, name);
  
  if (config_node)
  {
    return cape_udc_b (config_node, default_val);
  }
  else
  {
    cape_udc_add_b (self->config_node, name, default_val);
    
    return default_val;
  }
}

//-----------------------------------------------------------------------------

