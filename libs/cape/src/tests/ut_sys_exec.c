#include "sys/cape_exec.h"

// c includes
#include <stdio.h>

int main (int argc, char *argv[])
{
  int res;
  CapeErr err = cape_err_new ();
  
  CapeExec exec = cape_exec_new ();
  
  cape_exec_append_s (exec, "-cf");
  cape_exec_append_fmt (exec, "%s.tar", "CMakeFiles");
  cape_exec_append_s (exec, "CMakeFiles");
  
  res = cape_exec_run (exec, "tar", err);
  
  // output
  
  printf ("OUT: %s\n", cape_exec_get_stdout (exec));
  printf ("ERR: %s\n", cape_exec_get_stderr (exec));
  
  cape_exec_del (&exec);

  if (res)
  {
    printf ("ERROR: %s\n", cape_err_text(err));
  }
  
  cape_err_del (&err);
  
  return 0;
}

