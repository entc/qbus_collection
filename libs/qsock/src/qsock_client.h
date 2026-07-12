#ifndef __QSOCK_CLIENT__H
#define __QSOCK_CLIENT__H 1

// cape includes
#include "sys/cape_types.h"
#include <sys/cape_err.h>
#include <stc/cape_str.h>
#include <stc/cape_stream.h>
//-----------------------------------------------------------------------------

struct QSockClient_s; typedef struct QSockClient_s* QSockClient;

                                    /* constructor: create a new instance of the client class */
__CAPE_LIBEX     QSockClient        qsock_client_new    (const CapeString host, int port);

                                    /* destructor: cleans and frees all memory */
__CAPE_LIBEX     void               qsock_client_del    (QSockClient*);

__CAPE_LIBEX     int                qsock_client_run    (QSockClient, CapeErr err);

//-----------------------------------------------------------------------------

typedef void     (__STDCALL *fct_qsock_client__on_conn)      (void* user_ptr);
typedef void     (__STDCALL *fct_qsock_client__on_recv)      (void* user_ptr, const char* bufdat, number_t buflen);

__CAPE_LIBEX     void               qsock_client_cb     (QSockClient, void* user_ptr, fct_qsock_client__on_conn on_conn, fct_qsock_client__on_recv on_recv);

__CAPE_LIBEX     void               qsock_client_send   (QSockClient, CapeStream* p_buffer);

//-----------------------------------------------------------------------------

#endif
