#include <qcrypt.h>

// cape includes
#include <sys/cape_log.h>
#include <fmt/cape_json.h>
#include <stc/cape_str.h>

//-----------------------------------------------------------------------------

int main (int argc, char *argv[])
{
  int res;
  CapeErr err = cape_err_new ();
  
  CapeString enc01 = qcrypt__hash_sha256__hex_o ("test42", 6, err);
  
  if (FALSE == cape_str_equal (enc01, "338c0605bab38900480ebcc7fb0651426cc26cd1732579f04b47f779a8962d83"))
  {
    res = cape_err_set (err, CAPE_ERR_WRONG_VALUE, "sha254 missmatch");
    goto exit_and_cleanup;
  }
  
  
  res = CAPE_ERR_NONE;
  
exit_and_cleanup:
  
  if (cape_err_code(err))
  {
    cape_log_fmt (CAPE_LL_ERROR, "TEST", "MAIN", "test failed: %s", cape_err_text (err));
  }
  
  cape_str_del (&enc01);
  
  cape_err_del (&err);
  
  return res;
}

