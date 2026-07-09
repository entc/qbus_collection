#include "sys/cape_aio.h"
#include "stc/cape_str.h"
#include "sys/cape_log.h"

//-----------------------------------------------------------------------------

void __STDCALL ut_sys_aio__on_event (void* user_ptr, void* handle)
{
    cape_log_fmt (CAPE_LL_DEBUG, "TEST", "on event", "timer event with handle [%lu]", (number_t)handle);
}

//-----------------------------------------------------------------------------

int main (int argc, char *argv[])
{
	int res;
	CapeErr err = cape_err_new();

	CapeAio aio = cape_aio_new ();
	CapeAioItem aio_timer = NULL;

	// initialize main AIO event handler
	res = cape_aio_init (aio, err);
	if (res)
	{
			goto cleanup_and_exit;
	}

	aio_timer = cape_aio_add__timer (aio, 10000, err);
	if (NULL == aio_timer)
	{
			res = cape_err_code (err);
			goto cleanup_and_exit;
	}

    cape_aio_item_set (aio_timer, NULL, ut_sys_aio__on_event, NULL);

	res = cape_aio_wait (aio, err);

cleanup_and_exit:

	cape_aio_del (&aio);

	cape_err_del (&err);
	return res;
}

//-----------------------------------------------------------------------------
