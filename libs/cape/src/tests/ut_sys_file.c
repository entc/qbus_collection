#include "sys/cape_file.h"

// c includes
#include <stdio.h>

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
