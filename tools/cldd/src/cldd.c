#include <sys/cape_exec.h>
#include <sys/cape_file.h>
#include <sys/cape_log.h>
#include <fmt/cape_parser_line.h>
#include <fmt/cape_tokenizer.h>
#include <stc/cape_map.h>

// qcrypt includes
#include <qcrypt_file.h>

//-----------------------------------------------------------------------------

struct ClddCtx_s
{
  const CapeString image_path;
  CapeMap paths;

}; typedef struct ClddCtx_s* ClddCtx;

//-----------------------------------------------------------------------------

int cp_file (const CapeString source_file, const CapeString dest_path, const CapeString dest_file, CapeErr err)
{
  int res;

  printf ("copy %s -> %s\n", source_file, dest_file);

  res = cape_fs_path_create_x (dest_path, err);
  if (res)
  {
    cape_log_fmt (CAPE_LL_ERROR, "CLDD", "create path", "%s: %s", dest_path, cape_err_text (err));
    goto exit_and_cleanup;
  }

  res = cape_fs_file_cp (source_file, dest_file, err);
  if (res)
  {
    cape_log_fmt (CAPE_LL_ERROR, "CLDD", "copy file", "%s -> %s: %s", source_file, dest_file, cape_err_text (err));
    goto exit_and_cleanup;
  }

  CapeFileAc ac = cape_fs_file_ac (source_file, err);

  CapeString hash1 = qcrypt__hash_md5_file (source_file, err);
  CapeString hash2 = qcrypt__hash_md5_file (dest_file, err);

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

  return res;
}

//-----------------------------------------------------------------------------

int has_default_library_path (const CapeString path)
{
  return cape_str_equal (path, "/lib64") || cape_str_equal (path, "/usr/lib64") || cape_str_equal (path, "/lib") || cape_str_equal (path, "/usr/lib");

 // return cape_str_begins (path, "/usr") || cape_str_begins (path, "/lib") || cape_str_begins (path, "/lib64");
}

//-----------------------------------------------------------------------------

int cp_library (const CapeString file, const CapeString expected_filename, ClddCtx ctx, CapeErr err)
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
    dest_path = cape_fs_path_merge (ctx->image_path, path2);

    // create the destination file
    dest_file = cape_fs_path_merge (dest_path, filename);

    res = cp_file (file, dest_path, dest_file, err);
  }
  else
  {
    // create the destination directory
    dest_path = cape_fs_path_merge (ctx->image_path, "lib64");

    // create the destination file
    dest_file = cape_fs_path_merge (dest_path, filename);

    res = cp_file (file, dest_path, dest_file, err);
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

int cp_binary (const CapeString file, const CapeString image_path, CapeErr err)
{
  int res;

  const CapeString filename = cape_fs_split (file, NULL);

  CapeString dest_path = cape_fs_path_merge (image_path, "bin");
  CapeString dest_file = cape_fs_path_merge (dest_path, filename);

  res = cp_file (file, dest_path, dest_file, err);

  cape_str_del (&dest_file);
  cape_str_del (&dest_path);

  return res;
}

//-----------------------------------------------------------------------------

void __STDCALL on_newline (void* ptr, const CapeString line)
{
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

      cp_library (path, s1_cleaned, ptr, err);

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
        cp_library (path, NULL, ptr, err);
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

  // adjust logger output
  cape_log_set_level (CAPE_LL_INFO);

  if (argc <= 2)
  {
    res = cape_err_set (err, CAPE_ERR_MISSING_PARAM, "too few params");
    goto exit_and_cleanup;
  }

  // set the context
  ctx.image_path = argv[2];
  ctx.paths = cape_map_new (cape_map__compare__s, cldd__paths__on_del, NULL);

  // create the distination folder
  res = cape_fs_path_create_x (argv[2], err);
  if (res)
  {
    goto exit_and_cleanup;
  }

  // copy the binary file
  res = cp_binary (argv[1], argv[2], err);
  if (res)
  {
    goto exit_and_cleanup;
  }

  // create a new execution environment
  exec = cape_exec_new ();

  // add the input file as first parameter
  cape_exec_append_s (exec, argv[1]);

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

  cape_parser_line_del (&lparser);
  cape_exec_del (&exec);
  cape_err_del (&err);

  return res;
}

//-----------------------------------------------------------------------------
