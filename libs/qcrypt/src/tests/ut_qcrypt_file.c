#include <qcrypt_file.h>

// cape includes
#include <sys/cape_log.h>
#include <fmt/cape_json.h>

//-----------------------------------------------------------------------------

int main (int argc, char *argv[])
{
  int res;
  CapeErr err = cape_err_new ();
  
  const CapeString data = "{\"hello1\":\"world!!\",\"hello2\":42}";
  
  // local objects
  CapeFileHandle fh = cape_fh_new (NULL, "md5_test");
  CapeUdc content = NULL;
  
  res = cape_fh_open (fh, O_CREAT | O_TRUNC | O_WRONLY, err);
  if (res)
  {
    goto exit_and_cleanup;
  }

  cape_fh_write_buf (fh, data, cape_str_size (data));
  
  cape_fh_del (&fh);
  
  // check md5
  {
    CapeString h = qcrypt__hash_md5_file ("md5_test", err);
    if (h == NULL)
    {
      res = cape_err_code (err);
      goto exit_and_cleanup;
    }
    
    if (FALSE == cape_str_equal (h, "c33f142aea8e46ad1f0fb769ed4a2521"))
    {
      res = cape_err_set (err, CAPE_ERR_WRONG_VALUE, "md5 mismatch");
      goto exit_and_cleanup;
    }
    
    cape_str_del (&h);
  }
  
  res = qcrypt__encrypt_file ("md5_test", "encrypted_test", "mySecret!", err);
  if (res)
  {
    goto exit_and_cleanup;
  }
  
  res = qcrypt__decrypt_file ("encrypted_test", "decrypted_test", "mySecret!", err);
  if (res)
  {
    goto exit_and_cleanup;
  }

  // check md5
  {
    CapeString h = qcrypt__hash_md5_file ("decrypted_test", err);
    if (h == NULL)
    {
      res = cape_err_code (err);
      goto exit_and_cleanup;
    }
    
    if (FALSE == cape_str_equal (h, "c33f142aea8e46ad1f0fb769ed4a2521"))
    {
      res = cape_err_set (err, CAPE_ERR_WRONG_VALUE, "md5 mismatch [decrypted]");
      goto exit_and_cleanup;
    }
    
    cape_str_del (&h);
  }
  
  // load the object from the file
  content = cape_json_from_file ("md5_test", err);
  if (content == NULL)
  {
    res = cape_err_code (err);
    goto exit_and_cleanup;
  }
  
  res = qcrypt__encrypt_json (content, "encrypted_json", "myJsonSecret!", err);
  if (res)
  {
    goto exit_and_cleanup;
  }
  
  cape_udc_del (&content);
  
  res = qcrypt__decrypt_json ("encrypted_json", &content, "myJsonSecret!", err);
  if (res)
  {
    goto exit_and_cleanup;
  }
  
  {
    const CapeString h1 = cape_udc_get_s (content, "hello1", NULL);
    if (!cape_str_equal (h1, "world!!"))
    {
      res = cape_err_set (err, CAPE_ERR_WRONG_VALUE, "json mismatch [h1]");
      goto exit_and_cleanup;
    }
    
    number_t h2 = cape_udc_get_n (content, "hello2", 0);
    if (h2 != 42)
    {
      res = cape_err_set (err, CAPE_ERR_WRONG_VALUE, "json mismatch [h2]");
      goto exit_and_cleanup;
    }
    
    CapeString h3 = cape_json_to_s (content);
    if (!cape_str_equal (h3, data))
    {
      res = cape_err_set (err, CAPE_ERR_WRONG_VALUE, "json mismatch [h3]");
      goto exit_and_cleanup;
    }
  }
  
  res = CAPE_ERR_NONE;
  
exit_and_cleanup:
  
  if (cape_err_code(err))
  {
    cape_log_fmt (CAPE_LL_ERROR, "TEST", "MAIN", "test failed: %s", cape_err_text (err));
  }
  
  cape_fh_del (&fh);
  cape_err_del (&err);
  
  return res;
}

