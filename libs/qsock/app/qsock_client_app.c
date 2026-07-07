#include "qsock_client.h"

#include "fmt/cape_args.h"

//-----------------------------------------------------------------------------

int main (int argc, char *argv[])
{
    int res;

    // convert arguments
    CapeUdc parameters = cape_args_from_args (argc, argv, NULL);

    // local objects
    CapeErr err = cape_err_new ();
  
cleanup_and_exit:
  
    cape_err_del (&err);

    return res;
}
