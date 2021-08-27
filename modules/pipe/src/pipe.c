#include "qbus.h"

// cape includes
#include <aio/cape_aio_ctx.h>
#include <aio/cape_aio_file.h>
#include <sys/cape_err.h>
#include <sys/cape_log.h>
#include <sys/cape_file.h>
#include <sys/cape_mutex.h>
#include <fmt/cape_tokenizer.h>
#include <fmt/cape_json.h>
#include <sys/cape_pipe.h>

// regular expression
#include <pcre.h>

#define MSGD_REGEX_SUBSTR_MAXAMOUNT 12

//-------------------------------------------------------------------------------------

struct PipeContext_s
{
  pcre* re;

  CapeString module;
  CapeString method;

}; typedef struct PipeContext_s* PipeContext;

//-------------------------------------------------------------------------------------

void pipe_context_del (PipeContext* p_self)
{
  if (*p_self)
  {
    PipeContext self = *p_self;

    if (self->re)
    {
      pcre_free(self->re);
    }

    cape_str_del (&(self->module));
    cape_str_del (&(self->method));

    CAPE_DEL (p_self, struct PipeContext_s);
  }
}

//-------------------------------------------------------------------------------------

int qbus_pipe__re (PipeContext self, const char* bufdat, number_t buflen, CapeErr err)
{
  int ovector [MSGD_REGEX_SUBSTR_MAXAMOUNT];

  int rc = pcre_exec (self->re, NULL, bufdat, buflen, 0, 0, ovector, MSGD_REGEX_SUBSTR_MAXAMOUNT);
  if (rc < 0)
  {
    switch (rc)
    {
      case PCRE_ERROR_NOMATCH      : return cape_err_set (err, CAPE_ERR_PROCESS_FAILED, "string did not match the pattern");
      case PCRE_ERROR_NULL         : return cape_err_set (err, CAPE_ERR_PROCESS_FAILED, "something was null");
      case PCRE_ERROR_BADOPTION    : return cape_err_set (err, CAPE_ERR_PROCESS_FAILED, "a bad option was passed");
      case PCRE_ERROR_BADMAGIC     : return cape_err_set (err, CAPE_ERR_PROCESS_FAILED, "magic number bad (compiled re corrupt?)");
      case PCRE_ERROR_UNKNOWN_NODE : return cape_err_set (err, CAPE_ERR_PROCESS_FAILED, "something kooky in the compiled re");
      case PCRE_ERROR_NOMEMORY     : return cape_err_set (err, CAPE_ERR_PROCESS_FAILED, "ran out of memory");
      default                      : return cape_err_set (err, CAPE_ERR_PROCESS_FAILED, "unknown error");
    }
  }
}

//-------------------------------------------------------------------------------------

void __STDCALL qbus_pipe__on_read (void* ptr, CapeAioFileReader freader, const char* bufdat, number_t buflen)
{
  PipeContext self = ptr;

  // local objects
  CapeErr err = cape_err_new ();

  qbus_pipe__re (self, bufdat, buflen, err);

exit_and_cleanup:

  cape_err_del (&err);
}

//-------------------------------------------------------------------------------------

static int __STDCALL qbus_pipe_init (QBus qbus, void* ptr, void** p_ptr, CapeErr err)
{
  int res;

  const CapeString path = qbus_config_s (qbus, "path", "/tmp");
  const CapeString name = qbus_config_s (qbus, "name", "qmod_pipe");
  const CapeString regex = qbus_config_s (qbus, "regex", ":(.*): to=(.*), relay");

  const char* error;
  int erroffset;

  // local objects
  PipeContext self = CAPE_NEW (struct PipeContext_s);

  self->module = cape_str_cp (qbus_config_s (qbus, "module", NULL));
  self->method = cape_str_cp (qbus_config_s (qbus, "method", NULL));

  self->re = pcre_compile (regex, PCRE_CASELESS | PCRE_DOTALL, &error, &erroffset, NULL);
  if (self->re == NULL)
  {
    goto exit_and_cleanup;
  }

  // this should always work
  void* handle = cape_pipe_create_or_connect (path, name, err);
  if (handle == NULL)
  {
    res = cape_err_code (err);
    goto exit_and_cleanup;
  }

  {
    CapeAioFileReader file_reader = cape_aio_freader_new (handle, self, qbus_pipe__on_read);

    if (!cape_aio_freader_add (&file_reader, qbus_aio (qbus)))
    {

    }
  }

  res = CAPE_ERR_NONE;

  // transfer ownership
  *p_ptr = self;
  self = NULL;

exit_and_cleanup:

  if (res)
  {
    cape_log_fmt (CAPE_LL_ERROR, "PIPE", "init", "can't initialize: %s", cape_err_text (err));
  }

  pipe_context_del (&self);
  return res;
}

//-------------------------------------------------------------------------------------

static int __STDCALL qbus_pipe_done (QBus qbus, void* ptr, CapeErr err)
{
  PipeContext self = ptr;

  pipe_context_del (&self);

  return CAPE_ERR_NONE;
}

//-------------------------------------------------------------------------------------

int main (int argc, char *argv[])
{
  qbus_instance ("PIPE", NULL, qbus_pipe_init, qbus_pipe_done, argc, argv);
  return 0;
}

//-------------------------------------------------------------------------------------
