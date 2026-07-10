#include "sys/cape_aio.h"
#include "stc/cape_str.h"
#include "sys/cape_log.h"

//-----------------------------------------------------------------------------

int __STDCALL ut_sys_aio__on_recv (void* user_ptr, CapeAioItem item)
{
    cape_log_fmt (CAPE_LL_DEBUG, "TEST", "on event", "timer event with handle [%lu]", (number_t)cape_aio_item_get (item));

    return TRUE;
}

//-----------------------------------------------------------------------------

int __STDCALL ut_sys_aio__on_send (void* user_ptr, CapeAioItem item)
{
    cape_log_fmt (CAPE_LL_DEBUG, "TEST", "on event", "timer event with handle [%lu]", (number_t)cape_aio_item_get (item));

    return TRUE;
}

//-----------------------------------------------------------------------------

int main (int argc, char *argv[])
{
	int res;
	CapeErr err = cape_err_new();

	CapeAio aio = cape_aio_new ();
	CapeAioItem aio_timer = NULL;

	cape_log_fmt(CAPE_LL_TRACE, "TEST", "main", "start initialization");

	// initialize main AIO event handler
	res = cape_aio_init (aio, err);
	if (res)
	{
			goto cleanup_and_exit;
	}

	cape_log_fmt(CAPE_LL_TRACE, "TEST", "main", "add AIO timer");

	aio_timer = cape_aio_add__timer (aio, 10000, err);
	if (NULL == aio_timer)
	{
			res = cape_err_code (err);
			goto cleanup_and_exit;
	}

    cape_aio_item_set (aio_timer, NULL, ut_sys_aio__on_recv, ut_sys_aio__on_send, NULL);

	cape_log_fmt(CAPE_LL_TRACE, "TEST", "main", "wait for events ...");

	res = cape_aio_wait (aio, err);

cleanup_and_exit:

	if (res)
	{
		cape_log_fmt(CAPE_LL_ERROR, "TEST", "main", "error seen: %s", cape_err_text(err));
	}

	cape_aio_del (&aio);

	cape_err_del (&err);
	return res;
}

//-----------------------------------------------------------------------------
