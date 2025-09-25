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

#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>


//-----------------------------------------------------------------------------

void* file_handle_get_in_memeory_filefd (const CapeString name, CapeErr err)
{
  number_t fd = memfd_create (name, 0);
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

void* file_handle_get_in_memeory_filefd (const CapeString name, CapeErr err)
{
  cape_err_set (err, CAPE_ERR_NOT_SUPPORTED, "not supported on this OS");
  
  return NULL;
}

#endif

//-----------------------------------------------------------------------------

#define BUFFER_SIZE  10000

int decrypt_int_fd (const CapeString source, number_t fd_dest, const CapeString vsec, CapeErr err)
{
  int res;
  
  // local objects
  char* buffer = CAPE_ALLOC (BUFFER_SIZE);
  QCryptDecrypt decrypt = qcrypt_decrypt_new (NULL, source, vsec);
  
  //cape_log_fmt (CAPE_LL_WARN, "QCRYPT", "decrypt file", "using file = '%s', pass = '%s'", source, vsec);

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
      number_t written_bytes = write (fd_dest, buffer, bytes_decrypted);
      if (written_bytes == -1)
      {
        cape_err_lastOSError (err);
        goto exit_and_cleanup;
      }
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

CapeString fetch_sec (const CapeUdc params, CapeErr err)
{
  int res;
  CapeString ret = NULL;
  
  // local objects
  CapeFileHandle fh = NULL;
  CapeStream s = NULL;
  
  ret = cape_udc_ext_s (params, "sec");
  if (ret)
  {
    goto cleaup_and_exit;
  }

  fh = cape_fh_new ("etc", "sec");

  res = cape_fh_open (fh, O_RDONLY, err);
  if (res)
  {
    goto cleaup_and_exit;
  }

  s = cape_stream_new ();
  
  cape_stream_cap (s, 1024);
  
  {
    number_t bytes_read = cape_fh_read_buf (fh, cape_stream_pos (s), 1024);
    if (bytes_read < 1)
    {
      // some error happened
      res = cape_err_lastOSError (err);
      goto cleaup_and_exit;
    }
    
    cape_stream_set (s, bytes_read);
  }

  // trim and copy the string
  ret = cape_str_trim_utf8 (cape_stream_get (s));
  
cleaup_and_exit:
  
  cape_stream_del (&s);
  cape_fh_del (&fh);
  return ret;
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
  CapeString name = cape_str_uuid ();
  CapeString sec = NULL;
  CapeString binary_src = NULL;
  number_t mmfd;

  // adjust logger output
  cape_log_set_level (CAPE_LL_INFO);
  cape_log_msg (CAPE_LL_INFO, "EXECS", "main", "start ExecuterSec app");

  params = cape_args_from_args (argc, argv, NULL);
  
  /*
  {
    CapeString s = cape_json_to_s (params);
    
    printf ("params = %s\n", s);
    
    cape_str_del (&s);
  }
  */
  
  binary_src = cape_udc_ext_s (params, "_1");
  if (NULL == binary_src)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "no source");
    goto exit_and_cleanup;
  }
  
  // create in memory file descriptor
  mmfd = (number_t)file_handle_get_in_memeory_filefd (name, err);
  if (0 == mmfd)
  {
    res = cape_err_code (err);
    goto exit_and_cleanup;
  }
  
  // try to get the sec 
  sec = fetch_sec (params, err);
  if (NULL == sec)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "no sec");
    goto exit_and_cleanup;
  }
  
  res = decrypt_int_fd (binary_src, mmfd, sec, err);
  if (res)
  {
    goto exit_and_cleanup;
  }
  
//  exec_source = cape_str_fmt ("/proc/%jd/fd/%d", (intmax_t) getpid(), mmfd);
  
  // create a new execution environment
  exec = cape_exec_new ();

  // set the path to find the libraries
  cape_exec_env_set (exec, "PATH", "/bin;/lib;/lib64");
  cape_exec_env_set (exec, "LD_LIBRARY_PATH", "/lib64;/usr/lib/gcc/x86_64-pc-linux-gnu/12/");
  
  // forward parameters
  cape_exec_append_node (exec, params);
  
  // run the external program
  res = cape_exec_run_direct (exec, (void*)mmfd, name, err);
  if (res)
  {
    goto exit_and_cleanup;
  }


  res = CAPE_ERR_NONE;

exit_and_cleanup:

  cape_str_del (&sec);
  cape_str_del (&exec_source);
  cape_str_del (&binary_src);
  
  cape_exec_del (&exec);
  cape_err_del (&err);

  return res;
}

//-----------------------------------------------------------------------------
