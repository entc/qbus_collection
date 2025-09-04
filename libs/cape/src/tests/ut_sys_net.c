#include "sys/cape_net.h"
#include "stc/cape_str.h"

//-----------------------------------------------------------------------------

int main (int argc, char *argv[])
{
  int res;
  CapeErr err = cape_err_new ();

  CapeString h1 = cape_net__resolve ("google.com", FALSE, err);

  printf ("HOST1: %s\n", h1);


exit_and_cleanup:

  cape_str_del (&h1);

  cape_err_del (&err);
  return res;
}

//-----------------------------------------------------------------------------
