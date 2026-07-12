#include "qsock_client.h"

#include "fmt/cape_args.h"

//-----------------------------------------------------------------------------

void __STDCALL client__on_conn (void* user_ptr)
{
    CapeStream s = cape_stream_new ();

    cape_stream_append_str (s, "hello world!\n\n");

    qsock_client_send (user_ptr, &s);
}

//-----------------------------------------------------------------------------

void __STDCALL client__on_recv (void* user_ptr, const char* bufdat, number_t buflen)
{

}

//-----------------------------------------------------------------------------

int main (int argc, char *argv[])
{
    int res;

    // convert arguments
    CapeUdc parameters = cape_args_from_args (argc, argv, NULL);

    // local objects
    CapeErr err = cape_err_new ();
    QSockClient client = qsock_client_new (cape_udc_get_s (parameters, "host", "127.0.0.1"), cape_udc_get_n (parameters, "port", 8080));

    qsock_client_cb (client, client, client__on_conn, client__on_recv);


    res = qsock_client_run (client, err);

    cape_udc_del (&parameters);

    qsock_client_del (&client);
    cape_err_del (&err);

    return res;
}
