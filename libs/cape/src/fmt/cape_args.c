#include "cape_args.h"

// other cape includes
#include "sys/cape_file.h"
#include "sys/cape_log.h"

//-----------------------------------------------------------------------------------------------------------

CapeUdc cape_args_from_args (int argc, char *argv[], const CapeString name)
{
  CapeUdc params = cape_udc_new (CAPE_UDC_NODE, name);
  CapeUdc current_argument = NULL;
  
  int i;
  
  for (i = 1; i < argc; i++)
  {
    const char* arg = argv[i];
    
    if (cape_str_begins (arg, "--"))
    {
      // do we have a current argument?
      if (current_argument)
      {
        cape_udc_add (params, &current_argument);
      }

      current_argument = cape_udc_new (CAPE_UDC_STRING, arg + 2);
    }
    else if (cape_str_begins (arg, "-"))
    {
      // do we have a current argument?
      if (current_argument)
      {
        cape_udc_add (params, &current_argument);
      }
      
      current_argument = cape_udc_new (CAPE_UDC_STRING, arg + 1);
    }
    else
    {
      if (current_argument)
      {
        cape_udc_set_s_cp (current_argument, arg);
      }
    }
  }
  
  // do we have a current argument?
  if (current_argument)
  {
    cape_udc_add (params, &current_argument);
  }
  
  return params;
}

//-----------------------------------------------------------------------------

CapeString cape_args_config_file__folder (const CapeString folder, const CapeString name, CapeErr err)
{
  CapeString ret = NULL;
  
  // local objects
  CapeString resolved = cape_fs_path_resolve (folder, err);
  CapeDirCursor dc = NULL;
  
  if (resolved)
  {
    cape_log_fmt (CAPE_LL_TRACE, "CAPE", "find config", "scan: %s", resolved);
    
    dc = cape_dc_new (resolved, err);
    if (dc)
    {
      while (cape_dc_next (dc))
      {
        if (cape_str_compare (name, cape_dc_name (dc)))
        {
          ret = cape_fs_path_merge (resolved, cape_dc_name (dc));
          cape_log_fmt (CAPE_LL_TRACE, "CAPE", "config file", "use config: %s", ret);
          
          goto exit_and_cleanup;
        }
      }
    }
  }
  
exit_and_cleanup:

  cape_dc_del (&dc);
  cape_str_del (&resolved);
  return ret;
}

//-----------------------------------------------------------------------------------------------------------

CapeString cape_args_config_file (const CapeString primary, const CapeString name)
{
  CapeString ret = NULL;
  
  // local objects
  CapeErr err = cape_err_new();
  
  // create the certrain folders
  CapeString current_folder = cape_fs_path_current (NULL);
  CapeString primary_folder = cape_fs_path_merge (current_folder, primary);
  
  ret = cape_args_config_file__folder (primary_folder, name, err);
  if (ret)
  {
    goto exit_and_cleanup;
  }
  
  ret = cape_args_config_file__folder (current_folder, name, err);

  if (cape_err_code (err))
  {
    cape_log_fmt (CAPE_LL_WARN, "CAPE", "config file", cape_err_text (err));
  }
  
exit_and_cleanup:
  
  cape_str_del (&current_folder);
  cape_str_del (&primary_folder);
  cape_err_del (&err);
  
  return ret;
}

//-----------------------------------------------------------------------------------------------------------


