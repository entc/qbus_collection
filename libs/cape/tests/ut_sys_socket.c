#include "sys/cape_socket.h"
#include "stc/cape_str.h"

//-----------------------------------------------------------------------------

int main (int argc, char *argv[])
{
  int res;
  CapeErr err = cape_err_new ();

  void* socket = cape_sock__tcp__srv_new ("localhost", 11111, err);

  cape_sock__close (socket);

exit_and_cleanup:

  cape_err_del (&err);
  return res;
}

//-----------------------------------------------------------------------------
