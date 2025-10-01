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

//-----------------------------------------------------------------------------

struct ClddCtx_s
{
  const CapeString binary_src;
  const CapeString image_path;

  CapeMap paths;
  const CapeString vsec;
  
  number_t uid;
  number_t gid;
  number_t mod;
  
}; typedef struct ClddCtx_s* ClddCtx;

//-----------------------------------------------------------------------------

int cp_file (ClddCtx self, const CapeString source_file, const CapeString dest_path, const CapeString dest_file, CapeErr err)
{
  int res;

  // local objects
  CapeFileAc ac = cape_fs_ac_new (self->uid, self->gid, self->mod);
  CapeString hash1 = NULL;
  CapeString hash2 = NULL;
  
  printf ("copy %s -> %s [%li:%li]\n", source_file, dest_file, self->uid, self->gid);
  
  res = cape_fs_path_create_x (dest_path, err);
  if (res)
  {
    cape_log_fmt (CAPE_LL_ERROR, "CLDD", "create path", "%s: %s", dest_path, cape_err_text (err));
    goto exit_and_cleanup;
  }

  res = cape_fs_file_cp__ac (source_file, dest_file, &ac, err);
  if (res)
  {
    cape_log_fmt (CAPE_LL_ERROR, "CLDD", "copy file", "%s -> %s: %s", source_file, dest_file, cape_err_text (err));
    goto exit_and_cleanup;
  }

  hash1 = qcrypt__hash_md5_file (source_file, err);
  hash2 = qcrypt__hash_md5_file (dest_file, err);

  //cape_log_fmt (CAPE_LL_INFO, "CLDD", "validate", "%s <-> %s", hash1, hash2);

  if (!cape_str_equal (hash1, hash2))
  {
    res = cape_err_set (err, CAPE_ERR_RUNTIME, "hash missmatch");
    goto exit_and_cleanup;
  }

  res = CAPE_ERR_NONE;

exit_and_cleanup:

  cape_str_del (&hash1);
  cape_str_del (&hash2);
  cape_fs_ac_del (&ac);

  return res;
}

//-----------------------------------------------------------------------------

int has_default_library_path (const CapeString path)
{
  return cape_str_equal (path, "/lib64") || cape_str_equal (path, "/usr/lib64") || cape_str_equal (path, "/lib") || cape_str_equal (path, "/usr/lib");

 // return cape_str_begins (path, "/usr") || cape_str_begins (path, "/lib") || cape_str_begins (path, "/lib64");
}

//-----------------------------------------------------------------------------

int cp_library (ClddCtx self, const CapeString file, const CapeString expected_filename, CapeErr err)
{
  int res;

  // local objects
  CapeString path2 = NULL;
  CapeString dest_path = NULL;
  CapeString dest_file = NULL;

  // extract the filename and the path
  const CapeString filename = cape_fs_split (file, &path2);

  if (has_default_library_path (path2))
  {
    // create the destination directory
    dest_path = cape_fs_path_merge (self->image_path, path2);

    // create the destination file
    dest_file = cape_fs_path_merge (dest_path, filename);

    res = cp_file (self, file, dest_path, dest_file, err);
  }
  else
  {
    // create the destination directory
    dest_path = cape_fs_path_merge (self->image_path, "lib64");

    // create the destination file
    dest_file = cape_fs_path_merge (dest_path, filename);

    res = cp_file (self, file, dest_path, dest_file, err);
  }

  if (res)
  {
    goto exit_and_cleanup;
  }

  // check if the filename is equal to expected_file
  if (expected_filename)
  {
    if (FALSE == cape_str_equal (expected_filename, filename))
    {
      CapeString dest_link = cape_fs_path_merge (dest_path, expected_filename);

      printf ("link %s -> %s\n", dest_file, dest_link);

      // we need to create an additional symlink
      res = cape_fs_path_ln (filename, dest_link, dest_path, err);

      cape_str_del (&dest_link);
    }
  }

exit_and_cleanup:

  cape_str_del (&dest_path);
  cape_str_del (&dest_file);

  cape_str_del (&path2);
  return res;
}

//-----------------------------------------------------------------------------

int cp_binary (ClddCtx self, const CapeString subdir_path, CapeErr err)
{
  int res;

  const CapeString filename = cape_fs_split (self->binary_src, NULL);

  // local objects
  CapeString dest_path = NULL;
  CapeString dest_file = NULL;
  CapeString filename_encrypted = NULL;
  CapeString dest_encrypted_file = NULL;
  CapeFileAc ac = NULL;
  
  if (subdir_path)
  {
    dest_path = cape_fs_path_merge_3 (self->image_path, "bin", subdir_path);
  }
  else
  {
    dest_path = cape_fs_path_merge (self->image_path, "bin");
  }

  dest_file = cape_fs_path_merge (dest_path, filename);

  res = cp_file (self, self->binary_src, dest_path, dest_file, err);
  if (res)
  {
    goto cleanup_and_exit;
  }
  
  if (self->vsec)
  {
    // create a new filename with the extension '.sec'
    filename_encrypted = cape_str_catenate_2 (filename, ".sec");
    
    // create a new destionation for the encrypted file
    dest_encrypted_file = cape_fs_path_merge (dest_path, filename_encrypted);
    
    // encrypt the file
    res = qcrypt__encrypt_file (dest_file, dest_encrypted_file, self->vsec, err);
    if (res)
    {
      cape_log_fmt (CAPE_LL_ERROR, "CLDD", "encrypt file", "%s: %s", dest_encrypted_file, cape_err_text (err));
      goto cleanup_and_exit;
    }
    
    // remove original
    res = cape_fs_file_rm (dest_file, err);
    if (res)
    {
      cape_log_fmt (CAPE_LL_ERROR, "CLDD", "remove file", "%s: %s", dest_file, cape_err_text (err));
      goto cleanup_and_exit;
    }
    
    ac = cape_fs_ac_new (self->uid, self->gid, self->mod);
    
    // set ac
    res = cape_fs_file_ac_set (dest_encrypted_file, &ac, err);
    if (res)
    {
      cape_log_fmt (CAPE_LL_ERROR, "CLDD", "chown file", "%s: %s", dest_encrypted_file, cape_err_text (err));
      goto cleanup_and_exit;
    }
    
    cape_log_fmt (CAPE_LL_INFO, "CLDD", "encrypt file", "replace %s -> %s", dest_file, dest_encrypted_file);
  }
  
  res = CAPE_ERR_NONE;
  
cleanup_and_exit:

  cape_fs_ac_del (&ac);

  cape_str_del (&filename_encrypted);
  cape_str_del (&dest_encrypted_file);

  cape_str_del (&dest_file);
  cape_str_del (&dest_path);

  return res;
}

//-----------------------------------------------------------------------------

void __STDCALL on_newline (void* ptr, const CapeString line)
{
  ClddCtx self = ptr;
  
  CapeString s1 = NULL;
  CapeString s2 = NULL;

  if (cape_tokenizer_split (line, '>', &s1, &s2))
  {
    // clean the expected library which was found by LDD
    CapeString s1_cleaned = cape_str_trim_lrstrict (s1);

    CapeString s3 = NULL;
    CapeString s4 = NULL;

    if (cape_tokenizer_split (s2, '(', &s3, &s4))
    {
      CapeString path = cape_str_trim_utf8 (s3);
      CapeErr err = cape_err_new ();

      cp_library (self, path, s1_cleaned, err);

      cape_err_del (&err);
      cape_str_del (&path);
    }

    cape_str_del (&s3);
    cape_str_del (&s4);

    cape_str_del (&s1_cleaned);
  }
  else
  {
    CapeString s3 = NULL;
    CapeString s4 = NULL;

    if (cape_tokenizer_split (line, '(', &s3, &s4))
    {
      CapeString path = cape_str_trim_utf8 (s3);
      CapeErr err = cape_err_new ();

      if (path[0] == '/')
      {
        cp_library (self, path, NULL, err);
      }

      cape_err_del (&err);
      cape_str_del (&path);
    }

    cape_str_del (&s3);
    cape_str_del (&s4);
  }

  cape_str_del (&s1);
  cape_str_del (&s2);
}

//-----------------------------------------------------------------------------

void __STDCALL cldd__paths__on_del (void* key, void* val)
{
  {
    CapeString h = key; cape_str_del (&h);
  }
}

//-----------------------------------------------------------------------------

int main (int argc, char *argv[])
{
  int res;
  const CapeString output;
  struct ClddCtx_s ctx;

  // local objects
  CapeErr err = cape_err_new ();
  CapeExec exec = NULL;
  CapeParserLine lparser = NULL;
  CapeUdc params = NULL;

  // adjust logger output
  cape_log_set_level (CAPE_LL_INFO);
  cape_log_msg (CAPE_LL_INFO, "CLDD", "main", "start CLDD app");

  params = cape_args_from_args (argc, argv, NULL);
  
  {
    CapeString s = cape_json_to_s (params);
    
    printf ("params = %s\n", s);
    
    cape_str_del (&s);
  }
  
  // check the parameters
  ctx.binary_src = cape_udc_get_s (params, "_1", NULL);
  if (NULL == ctx.binary_src)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "no source");
    goto exit_and_cleanup;
  }

  ctx.image_path = cape_udc_get_s (params, "_2", NULL);
  if (NULL == ctx.image_path)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "no destination");
    goto exit_and_cleanup;
  }
  
  // optional
  ctx.vsec = cape_udc_get_s (params, "sec", NULL);

  // optional
  ctx.uid = cape_udc_get_n (params, "uid", 0);

  // optional
  ctx.gid = cape_udc_get_n (params, "gid", 0);

  // optional
  ctx.mod = cape_udc_get_n (params, "mod", 0);
  
  // set the context
  ctx.paths = cape_map_new (cape_map__compare__s, cldd__paths__on_del, NULL);

  // create the destination folder
  res = cape_fs_path_create_x (ctx.image_path, err);
  if (res)
  {
    goto exit_and_cleanup;
  }

  // copy the binary file
  res = cp_binary (&ctx, cape_udc_get_s (params, "p", NULL), err);
  if (res)
  {
    goto exit_and_cleanup;
  }

  // create a new execution environment
  exec = cape_exec_new ();

  // add the input file as first parameter
  cape_exec_append_s (exec, ctx.binary_src);

  // run the external program
  res = cape_exec_run (exec, "ldd", err);
  if (res)
  {
    goto exit_and_cleanup;
  }

  // retrieve the std::out output
  output = cape_exec_get_stdout (exec);

  // create a new line parser for the output
  lparser = cape_parser_line_new (&ctx, on_newline);

  // run the parser
  res = cape_parser_line_process (lparser, output, cape_str_size (output), err);
  if (res)
  {
    goto exit_and_cleanup;
  }

  res = CAPE_ERR_NONE;

exit_and_cleanup:

  if (res)
  {
    cape_log_fmt (CAPE_LL_ERROR, "CLDD", "error", "%s", cape_err_text (err));
  }

  cape_map_del (&(ctx.paths));

  cape_udc_del (&params);
  cape_parser_line_del (&lparser);
  cape_exec_del (&exec);
  cape_err_del (&err);

  return res;
}

//-----------------------------------------------------------------------------
