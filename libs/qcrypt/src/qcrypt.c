#include "qcrypt.h"
#include "qcrypt_decrypt.h"
#include "qcrypt_encrypt.h"

// cape includes
#include <sys/cape_log.h>
#include <sys/cape_file.h>
#include <fmt/cape_json.h>
#include <stc/cape_str.h>

#if defined __WINDOWS_OS

#include <windows.h>
#include <wincrypt.h>
#pragma comment (lib, "Crypt32.lib")

#else

#include <openssl/sha.h>

#endif

//-----------------------------------------------------------------------------

int qcrypt__decrypt_process (QDecryptAES dec, const char* bufdat, number_t buflen, CapeErr err)
{
  int res;

  // local objects
  CapeStream data = NULL;
  
  // convert from base64 to binary
  data = qcrypt__decode_base64_o (bufdat, buflen);
  if (data == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_RUNTIME, "can't decode base64");
    goto exit_and_cleanup;
  }
  
  res = qdecrypt_aes_process (dec, cape_stream_data (data), cape_stream_size (data), err);
  if (res)
  {
    goto exit_and_cleanup;
  }
  
  res = qdecrypt_aes_finalize (dec, err);
  if (res)
  {
    goto exit_and_cleanup;
  }
  
  res = CAPE_ERR_NONE;
  
exit_and_cleanup:
  
  cape_stream_del (&data);
  return res;
}

//-----------------------------------------------------------------------------

CapeString qcrypt__decrypt (const CapeString vsec, const CapeString encrypted_text, CapeErr err)
{
  int res;
  QDecryptAES dec;
  CapeStream s;
  
  CapeString ret = NULL;
  
  if (vsec == NULL)
  {
    cape_err_set (err, CAPE_ERR_NO_OBJECT, "vsec is not set");
    return NULL;
  }
  
  s = cape_stream_new ();
  
  dec = qdecrypt_aes_new (s, vsec, QCRYPT_AES_TYPE_CFB, QCRYPT_KEY_PASSPHRASE_MD5);
  
  res = qcrypt__decrypt_process (dec, encrypted_text, cape_str_size (encrypted_text), err);
  
  qdecrypt_aes_del (&dec);
  
  if (res)
  {
    cape_stream_del (&s);
    return NULL;
  }
  else
  {
    // transform into a string
    CapeString h = cape_stream_to_str (&s);
    
    // we need to trim, because sometimes the decryption results in a wrong ending
    ret = cape_str_trim_utf8 (h);
    
    cape_str_del (&h);
    
    return ret;
  }
}

//-----------------------------------------------------------------------------

int qcrypt__decrypt_node (const CapeString vsec, const CapeString encrypted_text, CapeUdc dest_list, CapeErr err)
{
  CapeString decrypted = qcrypt__decrypt (vsec, encrypted_text, err);
  
  if (decrypted)
  {
    // convert to cape node
    CapeUdc k = cape_json_from_s (decrypted);
    
    cape_udc_merge_mv (dest_list, &k);
    
    cape_str_del (&decrypted);
    
    return CAPE_ERR_NONE;
  }
  else
  {
    return cape_err_code (err);
  }
}

//-----------------------------------------------------------------------------

int qcrypt__decrypt_row_text (const CapeString vsec, const CapeUdc row, const CapeString data_name, CapeErr err)
{
  int res;
  
  // extract the node
  CapeUdc extracted_text_node = cape_udc_ext (row, data_name);
  
  // do we have a node
  if (extracted_text_node)
  {
    // assume that the node is a text node
    const CapeString encrypted_text = cape_udc_s (extracted_text_node, NULL);
    if (encrypted_text && encrypted_text[0])
    {
      CapeString decrypted = qcrypt__decrypt (vsec, encrypted_text, err);
      if (decrypted)
      {
        cape_udc_add_s_mv (row, data_name, &decrypted);
      }
      else
      {
        cape_log_fmt (CAPE_LL_WARN, "QCRYPT", "decrypt row text", "can't decrypt '%s': %s", encrypted_text, cape_err_text (err));
        
        res = cape_err_code (err);
        goto exit_and_cleanup;
      }
    }
  }
  
  res = CAPE_ERR_NONE;
  
exit_and_cleanup:
  
  cape_udc_del (&extracted_text_node);

  return res;
}

//-----------------------------------------------------------------------------

int qcrypt__decrypt_row_node (const CapeString vsec, const CapeUdc row, const CapeString data_name, CapeErr err)
{
  int res;
  
  // extract the node
  CapeUdc extracted_text_node = cape_udc_ext (row, data_name);
  
  // do we have a node
  if (extracted_text_node)
  {
    // assume that the node is a text node
    const CapeString encrypted_text = cape_udc_s (extracted_text_node, NULL);
    if (encrypted_text)
    {
      CapeString decrypted = qcrypt__decrypt (vsec, encrypted_text, err);
      
      if (decrypted)
      {
        // convert to cape node
        CapeUdc k = cape_json_from_s (decrypted);
        
        cape_udc_add_name (row, &k, data_name);
        
        cape_str_del (&decrypted);
      }
      else
      {
        cape_log_fmt (CAPE_LL_WARN, "QCRYPT", "decrypt row node", "can't decrypt '%s': %s", encrypted_text, cape_err_text (err));

        res = cape_err_code (err);
        goto exit_and_cleanup;
      }
    }
  }
  
  res = CAPE_ERR_NONE;
  
exit_and_cleanup:
  
  cape_udc_del (&extracted_text_node);
  
  return res;
}

//-----------------------------------------------------------------------------

CapeString qcrypt__encrypt (const CapeString vsec, const CapeString decrypted_text, CapeErr err)
{
  int res;
  CapeString ret = NULL;
  
  // local objects
  CapeStream s = cape_stream_new ();
  QEncryptAES enc = NULL;
  
  if (vsec == NULL)
  {
    cape_err_set (err, CAPE_ERR_NO_OBJECT, "vsec is not set");
    goto exit_and_cleanup;
  }
  
  // create encryption engine
  enc = qencrypt_aes_new (s, QCRYPT_AES_TYPE_CFB, QCRYPT_PADDING_ANSI_X923, vsec, QCRYPT_KEY_PASSPHRASE_MD5);
  
  // encrypt the buffer 'decrypted_text'
  res = qencrypt_aes_process (enc, decrypted_text, cape_str_size (decrypted_text), err);
  if (res)
  {
    goto exit_and_cleanup;
  }
  
  // finalize the encryption
  res = qencrypt_aes_finalize (enc, err);
  if (res)
  {
    goto exit_and_cleanup;
  }
  
  // encode bytes into base64 string
  ret = qcrypt__encode_base64_m (s);
  
exit_and_cleanup:

  cape_stream_del (&s);
  qencrypt_aes_del (&enc);
  
  return ret;
}

//-----------------------------------------------------------------------------

CapeString qcrypt__encode_base64_o (const char* bufdat, number_t buflen)
{
  number_t len = ((buflen + 2) / 3 * 4) + 1;
  CapeString ret = CAPE_ALLOC (len);
  
  // openssl function
  int decodedSize = EVP_EncodeBlock ((unsigned char*)ret, (const unsigned char*)bufdat, buflen);
  
  // everything worked fine
  if ((decodedSize > 0) && (decodedSize < len))
  {
    ret[decodedSize] = 0;
    return ret;
  }

  CAPE_FREE (ret);
  return NULL;
}

//-----------------------------------------------------------------------------

CapeString qcrypt__encode_base64_m (const CapeStream source)
{
  return qcrypt__encode_base64_o (cape_stream_data (source), cape_stream_size (source));
}

//-----------------------------------------------------------------------------

CapeStream qcrypt__decode_base64_o (const char* bufdat, number_t buflen)
{
  number_t len = ((buflen + 3) / 4 * 3) + 1;
  CapeStream ret = cape_stream_new ();

  // reserve memory
  cape_stream_cap (ret, len);
  
  // openssl function
  int decodedSize = EVP_DecodeBlock ((unsigned char*)cape_stream_pos (ret), (const unsigned char*)bufdat, buflen);
  
  // everything worked fine
  if ((decodedSize > 0) && (decodedSize < len))
  {
    // trim the last bytes which are 0
    while ((cape_stream_pos (ret)[decodedSize - 1] == '\0') && (decodedSize > 0))
    {
      decodedSize--;
    }
    
    cape_stream_set (ret, decodedSize);
    return ret;
  }
  
  cape_stream_del (&ret);
  return NULL;
}

//-----------------------------------------------------------------------------

CapeStream qcrypt__decode_base64_s (const CapeString source)
{
  return qcrypt__decode_base64_o (source, cape_str_size (source));
}

//-----------------------------------------------------------------------------

CapeStream qcrypt__hash_sha256__bin_o (const char* bufdat, number_t buflen, CapeErr err)
{
#if defined __WINDOWS_OS

  int res;
  EcBuffer ret = NULL;
  
  HCRYPTPROV provHandle = (HCRYPTPROV)NULL;
  HCRYPTHASH hashHandle;
  
  provHandle = echash_prov_handle (PROV_RSA_AES, err);
  if (provHandle == NULL)
  {
    return NULL;
  }
  
  if (!CryptCreateHash (provHandle, CALG_SHA_256, 0, 0, &hashHandle))
  {
    ecerr_lastErrorOS (err, ENTC_LVL_ERROR);
    CryptReleaseContext (provHandle, 0);
    
    return NULL;
  }
  
  // create the buffer for the hash
  ret = ecbuf_create (32);
  
  // retrieve the hash from windows
  res = echash_hash_retrieve (source, ret, hashHandle, err);
  if (res)
  {
    // some error happened -> destroy the buffer
    ecbuf_destroy (&ret);
  }
  
  CryptDestroyHash (hashHandle);
  CryptReleaseContext (provHandle, 0);
  
  return ret;

#else
  
  CapeStream ret = NULL;
  
  // local objects
  CapeStream buffer = cape_stream_new ();
  SHA256_CTX sha256;

  // reserve bytes
  cape_stream_cap (buffer, SHA256_DIGEST_LENGTH);

  // initialization
  if (SHA256_Init (&sha256) == 0)
  {
    cape_err_set (err, CAPE_ERR_RUNTIME, "can't initialize SHA256");
    goto exit_and_cleanup;
  }
  
  // collect all data
  if (SHA256_Update (&sha256, bufdat, buflen) == 0)
  {
    cape_err_set (err, CAPE_ERR_RUNTIME, "can't update SHA256");
    goto exit_and_cleanup;
  }
  
  // create the SHA256 hash value and store it into the buffer stream
  if (SHA256_Final((unsigned char*)cape_stream_pos (buffer), &sha256) == 0)
  {
    cape_err_set (err, CAPE_ERR_RUNTIME, "can't finalize SHA256");
    goto exit_and_cleanup;
  }
  
  // correct the length of the stream
  cape_stream_set (buffer, SHA256_DIGEST_LENGTH);
  
  ret = buffer;
  buffer = NULL;
  
exit_and_cleanup:
  
  cape_stream_del (&buffer);
  return ret;
  
#endif
}

//-----------------------------------------------------------------------------

CapeStream qcrypt__hash_sha256__bin_m (const CapeStream source, CapeErr err)
{
  return qcrypt__hash_sha256__bin_o (cape_stream_data (source), cape_stream_size (source), err);
}

//-----------------------------------------------------------------------------

CapeString qcrypt__hash_sha256__hex_o (const char* bufdat, number_t buflen, CapeErr err)
{
  CapeString ret = NULL;
  
  // local objects
  CapeStream s = qcrypt__hash_sha256__bin_o (bufdat, buflen, err);

  if (NULL == s)
  {
    goto exit_and_cleanup;
  }
  
  ret = cape_str_hex ((const unsigned char*)cape_stream_data (s), cape_stream_size (s));
  
exit_and_cleanup:

  cape_stream_del (&s);
  return ret;
}

//-----------------------------------------------------------------------------

CapeString qcrypt__hash_sha256__hex_m (const CapeStream source, CapeErr err)
{
  return qcrypt__hash_sha256__hex_o (cape_stream_data (source), cape_stream_size (source), err);
}

//-----------------------------------------------------------------------------

CapeStream qcrypt__hash_sha__bin_m (const CapeStream source, CapeErr err)
{
  return qcrypt__hash_sha__bin_o (cape_stream_data (source), cape_stream_size (source), err);
}

//-----------------------------------------------------------------------------

CapeStream qcrypt__hash_sha__bin_o (const char* bufdat, number_t buflen, CapeErr err)
{
#if defined __WINDOWS_OS

#else
  
  CapeStream ret = NULL;
  
  // local objects
  CapeStream buffer = cape_stream_new ();
  SHA_CTX sha;
  
  // reserve bytes
  cape_stream_cap (buffer, SHA_DIGEST_LENGTH);
  
  // initialization
  if (SHA1_Init (&sha) == 0)
  {
    cape_err_set (err, CAPE_ERR_RUNTIME, "can't initialize SHA");
    goto exit_and_cleanup;
  }
  
  // collect all data
  if (SHA1_Update (&sha, bufdat, buflen) == 0)
  {
    cape_err_set (err, CAPE_ERR_RUNTIME, "can't update SHA");
    goto exit_and_cleanup;
  }
  
  // create the SHA256 hash value and store it into the buffer stream
  if (SHA1_Final((unsigned char*)cape_stream_pos (buffer), &sha) == 0)
  {
    cape_err_set (err, CAPE_ERR_RUNTIME, "can't finalize SHA256");
    goto exit_and_cleanup;
  }
  
  // correct the length of the stream
  cape_stream_set (buffer, SHA_DIGEST_LENGTH);
  
  ret = buffer;
  buffer = NULL;
  
exit_and_cleanup:
  
  cape_stream_del (&buffer);
  return ret;

#endif
}

//-----------------------------------------------------------------------------

CapeString __STDCALL qcrypt__stream_base64_encode (const CapeStream source)
{
  return qcrypt__encode_base64_m (source);
}

//-----------------------------------------------------------------------------

CapeStream __STDCALL qcrypt__stream_base64_decode (const CapeString source)
{
  return qcrypt__decode_base64_s (source);
}

//-----------------------------------------------------------------------------
