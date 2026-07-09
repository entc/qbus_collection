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
    QSockClient client = qsock_client_new (cape_udc_get_s(parameters, "host", "127.0.0.1"), cape_udc_get_n (parameters, "port", 8080));

    res = qsock_client_run (client, err);

cleanup_and_exit:
  
    qsock_client_del (&client);
    cape_err_del (&err);

    return res;
}
