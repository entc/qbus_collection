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

int run_test_01 (CapeErr err)
{
  int res = CAPE_ERR_NONE;
  
  {
		CapeString path = cape_str_fmt("%chome%ccape%c..%c..%cetc", CAPE_FS_FOLDER_SEP, CAPE_FS_FOLDER_SEP, CAPE_FS_FOLDER_SEP, CAPE_FS_FOLDER_SEP, CAPE_FS_FOLDER_SEP);
    CapeString rb01 = cape_fs_path_rebuild (path, err);
    
    if (rb01)
    {
      printf ("RB01: %s\n", rb01);
      
      cape_str_del (&rb01);
    }

		cape_str_del(&path);
  }
  {
		CapeString path = cape_str_fmt("home%ccape%c..%c..%cetc", CAPE_FS_FOLDER_SEP, CAPE_FS_FOLDER_SEP, CAPE_FS_FOLDER_SEP, CAPE_FS_FOLDER_SEP, CAPE_FS_FOLDER_SEP);
		CapeString rb02 = cape_fs_path_rebuild (path, err);
    
    if (rb02)
    {
      printf ("RB02: %s\n", rb02);

      cape_str_del (&rb02);
    }
	
		cape_str_del(&path);
	}
  {
		CapeString path = cape_str_fmt("%ccape%c..%c..%cetc", CAPE_FS_FOLDER_SEP, CAPE_FS_FOLDER_SEP, CAPE_FS_FOLDER_SEP, CAPE_FS_FOLDER_SEP);
		CapeString rb03 = cape_fs_path_rebuild (path, err);
    
    if (rb03)
    {
      printf ("RB03: %s\n", rb03);

      cape_str_del (&rb03);
      
      res = cape_err_set (err, CAPE_ERR_RUNTIME, "wrong result #1");
    }
    else
    {
      cape_err_clr (err);
    }
	
		cape_str_del(&path);
	}
  
  return res;
}

//-----------------------------------------------------------------------------

int run_test_02 (CapeErr err)
{
  int res = CAPE_ERR_NONE;

  {
    CapeString h = cape_fs_filename ("/tmp/testfile.txt");
    
    if (h)
    {
      printf ("FILENAME: %s\n", h);
      
      cape_str_del (&h);
    }
  }
  {
    const CapeString h = cape_fs_extension ("/tmp/testfile.txt");
    
    if (h)
    {
      printf ("EXTENSION: %s\n", h);
    }
  }

  return res;
}

//-----------------------------------------------------------------------------

int run_test_03 (CapeErr err)
{
  int res = CAPE_ERR_NONE;

  CapeString path0 = cape_fs_path_current (NULL);
  CapeString path1 = cape_fs_path_current ("path1");
  CapeString path2 = cape_fs_path_merge (path1, "path2");
  CapeString path3 = cape_fs_path_merge_3 (path0, "path1", "path2");
  CapeString path4 = cape_fs_path_reduce (path3);

  if (FALSE == cape_str_equal (path2, path3))
  {
    res = cape_err_set (err, CAPE_ERR_RUNTIME, "compare paths #1");
    goto exit_and_cleanup;
  }

  if (FALSE == cape_str_equal (path4, path1))
  {
    res = cape_err_set (err, CAPE_ERR_RUNTIME, "compare paths #2");
    goto exit_and_cleanup;
  }

exit_and_cleanup:
  
  cape_str_del (&path0);
  cape_str_del (&path1);
  cape_str_del (&path2);
  cape_str_del (&path3);
  cape_str_del (&path4);

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
  CapeString path_test_clone = cape_fs_path_current ("TestClone");
  CapeString path_child_folder = cape_fs_path_merge (path_test_folder, "child");
  
  res = run_test_01 (err);
  if (res)
  {
    goto exit_and_cleanup;
  }

  res = run_test_02 (err);
  if (res)
  {
    goto exit_and_cleanup;
  }
  
  res = run_test_03 (err);
  if (res)
  {
    goto exit_and_cleanup;
  }

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
  
  res = cape_fs_path_cp (path_test_folder, path_test_clone, err);
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
  
  /*
  res = cape_fs_path_rm (path_test_folder, TRUE, err);
  if (res)
  {
    goto exit_and_cleanup;
  }
   */
  
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
