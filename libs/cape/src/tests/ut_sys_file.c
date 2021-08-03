#include "sys/cape_file.h"
#include "sys/cape_log.h"

// c includes
#include <stdio.h>

//-----------------------------------------------------------------------------

int run_test_file (CapeErr err)
{
  int res;
  
  const CapeString content = "Hello World!!";
  
  // local objects
  CapeFileHandle fh1 = cape_fh_new (NULL, "test01.txt");
  CapeFileHandle fh2 = cape_fh_new (NULL, "test02.txt");
  
  // create the object  
  res = cape_fh_open (fh1, O_WRONLY | O_CREAT | O_TRUNC, err);
  if (res)
  {
    goto exit_and_cleanup;
  }
  
  cape_fh_write_buf (fh1, content, cape_str_size (content));
  
  res = cape_fs_file_cp ("test01.txt", "test02.txt", err);
  if (res)
  {
    goto exit_and_cleanup;
  }
  
  // create the object  
  res = cape_fh_open (fh2, O_RDONLY, err);
  if (res)
  {
    goto exit_and_cleanup;
  }
  
  {
    char buf[100];
    number_t bytes = cape_fh_read_buf (fh2, buf, 100);

    buf[bytes] = 0;
    
    if (!cape_str_equal (buf, content))
    {
      res = cape_err_set (err, CAPE_ERR_WRONG_VALUE, "copy mismatch");
    }
  }
  
  res = CAPE_ERR_NONE;
  
exit_and_cleanup:

  if (cape_err_code (err))
  {
    cape_log_msg (CAPE_LL_ERROR, "TEST", "sys file", cape_err_text (err));
  }
  
  cape_fh_del (&fh1);
  cape_fh_del (&fh2);
  
  return res;
}

//-----------------------------------------------------------------------------

int main (int argc, char *argv[])
{
  int res;
  
  // local objects
  CapeErr err = cape_err_new ();

  // folders to play around
  CapeString path_test_folder = cape_fs_path_current ("TestFolder");
  CapeString path_child_folder = cape_fs_path_merge (path_test_folder, "child");
  
  res = cape_fs_path_create (path_test_folder, err);
  if (res)
  {
    goto exit_and_cleanup;
  }

  res = cape_fs_path_create (path_child_folder, err);
  if (res)
  {
    goto exit_and_cleanup;
  }
  
  res = cape_fs_path_rm (path_test_folder, TRUE, err);
  if (res)
  {
    goto exit_and_cleanup;
  }

  res = cape_fs_path_create_x (path_child_folder, err);
  if (res)
  {
    goto exit_and_cleanup;
  }

  res = cape_fs_path_rm (path_test_folder, TRUE, err);
  if (res)
  {
    goto exit_and_cleanup;
  }
  
  res = run_test_file (err);
  if (res)
  {
    goto exit_and_cleanup;
  }
  
  res = CAPE_ERR_NONE;
  
exit_and_cleanup:
  
  if (res)
  {
    printf ("ERROR: %s\n", cape_err_text (err));
  }
  
  cape_str_del (&path_child_folder);
  cape_str_del (&path_test_folder);
  cape_err_del (&err);
  
  return res;
}
