#include "cape_args.h"

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

//-----------------------------------------------------------------------------------------------------------


