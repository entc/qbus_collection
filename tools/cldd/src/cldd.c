#include <sys/cape_exec.h>
#include <sys/cape_file.h>
#include <sys/cape_log.h>
#include <fmt/cape_parser_line.h>
#include <fmt/cape_tokenizer.h>

// qcrypt includes
#include <qcrypt_file.h>

//-----------------------------------------------------------------------------

void cp_file (const CapeString file, const CapeString image_path)
{
  int res;
  CapeErr err = cape_err_new ();  
  CapeString path2 = NULL;
  
  const CapeString filename = cape_fs_split (file, &path2);
  
  CapeString dest_path = cape_fs_path_merge (image_path, path2);
  CapeString dest_file = cape_fs_path_merge (dest_path, filename);
  
  res = cape_fs_path_create_x (dest_path, err);
  if (res)
  {
    cape_log_fmt (CAPE_LL_ERROR, "CLDD", "copy file: %s", cape_err_text (err));
    goto exit_and_cleanup;
  }
  
  res = cape_fs_file_cp (file, dest_file, err);
  if (res)
  {
    cape_log_fmt (CAPE_LL_ERROR, "CLDD", "copy file: %s", cape_err_text (err));
    goto exit_and_cleanup;
  }
  
  printf ("copy %s -> %s\n", file, dest_file);
  
  CapeString hash1 = qcrypt__hash_md5_file (file, err);
  CapeString hash2 = qcrypt__hash_md5_file (dest_file, err);

  cape_log_fmt (CAPE_LL_INFO, "CLDD", "validate", "%s <-> %s", hash1, hash2);    
  
  if (!cape_str_equal (hash1, hash2))
  {
    cape_log_fmt (CAPE_LL_ERROR, "CLDD", "copy file", "hash missmatch");    
  }
  
exit_and_cleanup:

  cape_str_del (&hash1);
  cape_str_del (&hash2);

  cape_str_del (&path2);

  cape_str_del (&dest_file);
  cape_str_del (&dest_path);
  cape_err_del (&err);
}

//-----------------------------------------------------------------------------

void __STDCALL on_newline (void* ptr, const CapeString line)
{  
  CapeString s1 = NULL;
  CapeString s2 = NULL;
  
  if (cape_tokenizer_split (line, '>', &s1, &s2))
  {
    {
      CapeString s3 = NULL;
      CapeString s4 = NULL;
      
      if (cape_tokenizer_split (s2, '(', &s3, &s4))
      {
        CapeString path = cape_str_trim_utf8 (s3);
        
        CapeErr err = cape_err_new ();
        
        cp_file (path, ptr);
        
        cape_err_del (&err);
        
        cape_str_del (&path);
      }

      cape_str_del (&s3);
      cape_str_del (&s4);
    }
  }
  else
  {
    CapeString s3 = NULL;
    CapeString s4 = NULL;
    
    if (cape_tokenizer_split (line, '(', &s3, &s4))
    {
      CapeString path = cape_str_trim_utf8 (s3);
      
      if (path[0] == '/')
      {
        cp_file (path, ptr);
      }
            
      cape_str_del (&path);
    }
    
    cape_str_del (&s3);
    cape_str_del (&s4);
  }
  
  cape_str_del (&s1);
  cape_str_del (&s2);
}

//-----------------------------------------------------------------------------

int main (int argc, char *argv[])
{
  if (argc > 2)
  {
    int res;
    CapeErr err = cape_err_new ();
    
    CapeExec exec = cape_exec_new ();  

    cape_log_set_level (CAPE_LL_INFO);
    
    cape_exec_append_s (exec, argv[1]);
        
    res = cape_exec_run (exec, "ldd", err);
    if (res)
    {
      
    }
    
    res = cape_fs_path_create_x (argv[2], err);
    if (res)
    {
      
    }
    
    const CapeString output = cape_exec_get_stdout (exec);  
   
    {
      CapeParserLine lparser = cape_parser_line_new (argv[2], on_newline);
      
      res = cape_parser_line_process (lparser, output, cape_str_size (output), err);
      if (res)
      {
        
      }
      
      cape_parser_line_del (&lparser);
    }
        
    cape_exec_del (&exec);
    cape_err_del (&err);
  }
      
  return 0;
}

//-----------------------------------------------------------------------------
