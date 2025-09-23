#include <sys/cape_exec.h>
#include <sys/cape_file.h>
#include <sys/cape_log.h>
#include <fmt/cape_parser_line.h>
#include <fmt/cape_tokenizer.h>
#include <stc/cape_map.h>
#include <fmt/cape_args.h>
#include <fmt/cape_json.h>

// qcrypt includes
#include <qcrypt_file.h>
#include <sys/stat.h>
#include <unistd.h>

//-----------------------------------------------------------------------------

#if defined __LINUX_OS

#include <sys/mman.h>

//-----------------------------------------------------------------------------

void* file_handle_get_in_memeory_filefd (CapeErr err)
{
  CapeString name = cape_str_uuid ();
  
  int fd = memfd_create (name, MFD_CLOEXEC | MFD_HUGETLB | MFD_HUGE_2MB);
  if (-1 == fd)
  {
    cape_err_lastOSError (err);
    return NULL;
  }
  
  return (void*)fd;
}

//-----------------------------------------------------------------------------

#else

//-----------------------------------------------------------------------------

void* file_handle_get_in_memeory_filefd (CapeErr err)
{
  cape_err_set (err, CAPE_ERR_NOT_SUPPORTED, "not supported on this OS");
  
  return NULL;
}

#endif

//-----------------------------------------------------------------------------

#define BUFFER_SIZE  10000

int decrypt_int_fd (const CapeString source, int fd_dest, const CapeString vsec, CapeErr err)
{
  int res;
  
  // local objects
  char* buffer = CAPE_ALLOC (BUFFER_SIZE);
  QCryptDecrypt decrypt = qcrypt_decrypt_new (NULL, source, vsec);
  
  //cape_log_fmt (CAPE_LL_TRACE, "QCRYPT", "decrypt file", "using file = '%s'", source);

  res = qcrypt_decrypt_open (decrypt, err);
  if (res)
  {
    goto exit_and_cleanup;
  }
  
  while (TRUE)
  {
    number_t bytes_decrypted = qcrypt_decrypt_next (decrypt, buffer, BUFFER_SIZE);
    if (bytes_decrypted)
    {
      write (fd_dest, buffer, bytes_decrypted);
    }
    else
    {
      break;
    }
  }
  
  res = CAPE_ERR_NONE;
  
exit_and_cleanup:
  
  qcrypt_decrypt_del (&decrypt);
  CAPE_FREE (buffer);
  
  return res;
}

//-----------------------------------------------------------------------------

int main (int argc, char *argv[])
{
  int res;

  // local objects
  CapeErr err = cape_err_new ();
  CapeExec exec = NULL;
  CapeUdc params = NULL;
  CapeString exec_source = NULL;

  const CapeString binary_src;
  int mmfd;

  // adjust logger output
  cape_log_set_level (CAPE_LL_INFO);
  cape_log_msg (CAPE_LL_INFO, "EXECS", "main", "start ExecuterSec app");

  params = cape_args_from_args (argc, argv, NULL);
  
  {
    CapeString s = cape_json_to_s (params);
    
    printf ("params = %s\n", s);
    
    cape_str_del (&s);
  }
  
  binary_src = cape_udc_get_s (params, "_1", NULL);
  if (NULL == binary_src)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "no source");
    goto exit_and_cleanup;
  }
  
  // create in memory file descriptor
  mmfd = file_handle_get_in_memeory_filefd (err);
  if (0 == mmfd)
  {
    res = cape_err_code (err);
    goto exit_and_cleanup;
  }
  
  res = decrypt_int_fd (binary_src, mmfd, "HelloWorld!", err);
  if (res)
  {
    goto exit_and_cleanup;
  }
  
  exec_source = cape_str_fmt ("/proc/%jd/fd/%d", (intmax_t) getpid(), mmfd);
  
  // create a new execution environment
  exec = cape_exec_new ();

  // run the external program
  res = cape_exec_run (exec, exec_source, err);
  if (res)
  {
    goto exit_and_cleanup;
  }


  res = CAPE_ERR_NONE;

exit_and_cleanup:

  cape_str_del (&exec_source);
  
  cape_exec_del (&exec);
  cape_err_del (&err);

  return res;
}

//-----------------------------------------------------------------------------
