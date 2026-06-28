#include "qcrypt_aes.h"
#include "qcrypt.h"

// cape includes
#include <sys/cape_log.h>

#if defined __WINDOWS_OS

#include <windows.h>
#include <wincrypt.h>
#pragma comment (lib, "Crypt32.lib")

#else

#include <openssl/sha.h>
#include <openssl/md5.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/rand.h>

#endif

#if defined __WINDOWS_OS

//-----------------------------------------------------------------------------

HCRYPTPROV ecencrypt_aes_acquireContext (DWORD provType, CapeErr err)
{
  HCRYPTPROV provHandle = (HCRYPTPROV)NULL;
  
  if (!CryptAcquireContext (&provHandle, NULL, NULL, provType, 0))
  {
    DWORD errCode = GetLastError ();

    if (errCode == NTE_BAD_KEYSET)
    {
      if (!CryptAcquireContext (&provHandle, NULL, NULL, provType, CRYPT_NEWKEYSET))
      {
        cape_err_lastOSError (err);
        return NULL;
      }
    }
  }
  
  return provHandle;
}

//-----------------------------------------------------------------------------

#else

//-----------------------------------------------------------------------------

struct QCryptAESKeys_s
{
    unsigned char* iv_buf;      // binary data
    int iv_len;
    
    unsigned char* salt_buf;    // binary data
    int salt_len;
    
    // the very secret key
    unsigned char* key;         // binary data
    int key_len;
};

//-----------------------------------------------------------------------------

void qcrypt_aes_keys_del (QCryptAESKeys* p_self)
{
    if (*p_self)
    {
        QCryptAESKeys self = *p_self;
        
        if (self->key)
        {
            // this prevents derived AES keys from remaining in heap memory
            OPENSSL_cleanse (self->key, self->key_len);
            CAPE_FREE (self->key);
        }

        if (self->iv_buf)
        {
            OPENSSL_cleanse (self->iv_buf, self->iv_len);
            CAPE_FREE (self->iv_buf);
        }

        if (self->salt_buf)
        {
            OPENSSL_cleanse (self->salt_buf, self->salt_len);
            CAPE_FREE (self->salt_buf);
        }

        CAPE_DEL (p_self, struct QCryptAESKeys_s);
    }
}

//-----------------------------------------------------------------------------

unsigned char* qcrypt_aes_key (QCryptAESKeys self)
{
    return self->key;
}

//-----------------------------------------------------------------------------

unsigned char* qcrypt_aes_iv (QCryptAESKeys self)
{
    return self->iv_buf;
}

//-----------------------------------------------------------------------------

int qcrypt_aes_iv_len (QCryptAESKeys self)
{
    return self->iv_len;
}

//-----------------------------------------------------------------------------

unsigned char* qcrypt_aes_salt (QCryptAESKeys self)
{
    return self->salt_buf;
}

//-----------------------------------------------------------------------------

int qcrypt_aes_salt_len (QCryptAESKeys self)
{
    return self->salt_len;
}

//-----------------------------------------------------------------------------

QCryptAESKeys qcrypt_aes_keys_new__sha256 (const CapeString secret, const EVP_CIPHER* cypher, CapeErr err)
{
  QCryptAESKeys self = NULL;
  
  int key_length = EVP_CIPHER_key_length (cypher);

  // length in 8 bit blocks
  if (key_length != 32)   // 8 * 32 = 256
  {
    cape_err_set_fmt (err, CAPE_ERR_RUNTIME, "cypher has unsupported key-length for padding (SHA256): %i", self->key_len);
    goto exit_and_cleanup;
  }
  
  self = CAPE_NEW (struct QCryptAESKeys_s);

  self->key_len = key_length;
  self->key = NULL;

  self->iv_len = 0;
  self->iv_buf = NULL;

  self->salt_len = 0;
  self->salt_buf = NULL;

  {
    CapeStream h = qcrypt__hash_sha256__bin_o (secret, cape_str_size (secret), err);
    if (NULL == h)
    {
      // destroy the object
      qcrypt_aes_keys_del (&self);
      goto exit_and_cleanup;
    }

    self->key = cape_stream_to_str (&h);
  }
  
exit_and_cleanup:
  
  if (cape_err_code (err))
  {
    cape_log_msg (CAPE_LL_ERROR, "QCRYPT", "aes keys sha256", cape_err_text(err));
  }
  
  return self;
}

//-----------------------------------------------------------------------------

QCryptAESKeys qcrypt_aes_keys_new__md5_en (const CapeString secret, const EVP_CIPHER* cypher, CapeStream product, CapeErr err)
{
    QCryptAESKeys self = CAPE_NEW (struct QCryptAESKeys_s);

    int rounds = 1;
    int len;
    
    self->key_len = EVP_MAX_KEY_LENGTH;
    self->key = CAPE_ALLOC (self->key_len);

    self->iv_len = EVP_MAX_IV_LENGTH;
    self->iv_buf = CAPE_ALLOC (self->iv_len);
    
    self->salt_len = 8;
    self->salt_buf = (unsigned char*)cape_str_random_s (self->salt_len);

    len = EVP_BytesToKey (cypher, EVP_md5(), self->salt_buf, (unsigned char*)secret, (int)cape_str_size (secret), rounds, self->key, self->iv_buf);
    
    // set termination
    self->key[len] = 0;

    cape_stream_append_buf (product, "Salted__", 8);
    cape_stream_append_buf (product, (char*)self->salt_buf, 8);
  
    return self;
}

//-----------------------------------------------------------------------------

QCryptAESKeys qcrypt_aes_keys_new__md5_de (const CapeString secret, const EVP_CIPHER* cypher, const char* bufdat, number_t buflen, CapeErr err)
{
    QCryptAESKeys self = CAPE_NEW (struct QCryptAESKeys_s);

    // do some pre-checks
    if (buflen < 16)
    {
        cape_err_set (err, CAPE_ERR_RUNTIME, "source buffer has less than 16 bytes");
        goto exit_and_cleanup;
    }
    
    // cypher options
    self->key_len = EVP_CIPHER_key_length (cypher);
    self->key = CAPE_ALLOC (self->key_len);

    self->iv_len = 16;
    self->iv_buf = CAPE_ALLOC (self->iv_len);
    
    self->salt_len = 0;
    self->salt_buf = NULL;

    EVP_BytesToKey (cypher, EVP_md5(), (const unsigned char*)bufdat + 8, (unsigned char*)secret, (int)cape_str_size (secret), 1, self->key, self->iv_buf);

exit_and_cleanup:

    if (cape_err_code (err))
    {
      cape_log_msg (CAPE_LL_ERROR, "QCRYPT", "aes keys sha256", cape_err_text(err));
    }

    return self;
}

//-----------------------------------------------------------------------------

QCryptAESKeys qcrypt_aes_keys_new__padding_zero (const CapeString secret, const EVP_CIPHER* cypher)
{
    QCryptAESKeys self = CAPE_NEW (struct QCryptAESKeys_s);

    number_t size = cape_str_size (secret);

    // cypher options
    self->key_len = EVP_CIPHER_key_length (cypher);
    
    // using the whole keylength for padding
    self->key = CAPE_ALLOC (self->key_len);
    
    // add the zeros (padding)
    memset (self->key, 0, self->key_len);

    // fill the buffer with they key
    memcpy (self->key, secret, size);
    
    // the rest is empty
    self->iv_len = 0;
    self->iv_buf = NULL;

    self->salt_len = 0;
    self->salt_buf = NULL;
    
    return self;
}

//-----------------------------------------------------------------------------

void qcrypt_aes_padding__ansiX923_pad (unsigned char* bufdat, number_t buflen, number_t offset)
{
  int64_t diff = buflen - offset;  // calculate the difference
  
  // add the zeros (padding)
  memset (bufdat + offset, 0, diff);

  // add the last byte (padding)
  memset (bufdat + buflen - 1, diff, 1);
}

//-----------------------------------------------------------------------------

QCryptAESKeys qcrypt_aes_keys_new__ansiX923 (const CapeString secret, const EVP_CIPHER* cypher)
{
    QCryptAESKeys self = CAPE_NEW (struct QCryptAESKeys_s);

    number_t size = cape_str_size (secret);

    // cypher options
    self->key_len = EVP_CIPHER_key_length (cypher);
    
    // using the whole keylength for padding
    self->key = CAPE_ALLOC (self->key_len);

    // add the zeros (padding)
    memset (self->key, 0, self->key_len);

    // fill the buffer with they key
    memcpy (self->key, secret, size);
    
    // add the last byte (padding)
    memset (self->key + self->key_len - 1, self->key_len - size, 1);
    
    // the rest is empty
    self->iv_len = 0;
    self->iv_buf = NULL;

    self->salt_len = 0;
    self->salt_buf = NULL;

    return self;
}

//-----------------------------------------------------------------------------

QCryptAESKeys qcrypt_aes_keys_new__padding_pkcs7 (const CapeString secret, const EVP_CIPHER* cypher)
{
    QCryptAESKeys self = CAPE_NEW (struct QCryptAESKeys_s);

    number_t size = cape_str_size (secret);

    // cypher options
    self->key_len = EVP_CIPHER_key_length (cypher);
    
    number_t diff = self->key_len - size;
          
    // using the whole keylength for padding
    self->key = CAPE_ALLOC (self->key_len);

    // add the padding
    memset (self->key, diff, self->key_len);

    // fill the buffer with the key
    memcpy (self->key, secret, size);

    // the rest is empty
    self->iv_len = 0;
    self->iv_buf = NULL;

    self->salt_len = 0;
    self->salt_buf = NULL;

    return self;
}

//-----------------------------------------------------------------------------

#define AES_256_PBKDF2__KEY_LEN      32
//-----------------------------------------------------------------------------

QCryptAESKeys qcrypt_aes_keys_new__pbkdf2 (const CapeString secret, int iterations, CapeErr err)
{
    QCryptAESKeys self = CAPE_NEW (struct QCryptAESKeys_s);

    self->key_len = AES_256_PBKDF2__KEY_LEN;
    self->key = CAPE_ALLOC (self->key_len);
    
    self->salt_len = AES_256_GCM__SALT_LEN;
    self->salt_buf = CAPE_ALLOC (self->salt_len);
    
    self->iv_len = AES_256_GCM__IV_LEN;
    self->iv_buf = CAPE_ALLOC (self->iv_len);

    // create the salt
    if (RAND_bytes (self->salt_buf, self->salt_len) != 1)
    {
        cape_err_set (err, CAPE_ERR_3RDPARTY_LIB, "rand failed on openssl");
        goto cleanup_and_exit;
    }
    
    if (RAND_bytes (self->iv_buf, self->iv_len) != 1)
    {
        cape_err_set (err, CAPE_ERR_3RDPARTY_LIB, "rand failed on openssl");
        goto cleanup_and_exit;
    }
    
    if (PKCS5_PBKDF2_HMAC (secret, (int)cape_str_size (secret), self->salt_buf, self->salt_len, iterations, EVP_sha256(), self->key_len, self->key) != 1)
    {
        cape_err_set (err, CAPE_ERR_3RDPARTY_LIB, "hmac failed on openssl");
        goto cleanup_and_exit;
    }
    
    return self;
    
cleanup_and_exit:
    
    qcrypt_aes_keys_del (&self);
    return NULL;
}

//-----------------------------------------------------------------------------

QCryptAESKeys qcrypt_aes_keys_gen__pbkdf2 (const CapeString secret, int iterations, unsigned char** p_salt_buf, int salt_len, unsigned char** p_iv_buf, int iv_len, CapeErr err)
{
    QCryptAESKeys self = CAPE_NEW (struct QCryptAESKeys_s);

    self->key = CAPE_ALLOC (AES_256_PBKDF2__KEY_LEN);
    self->key_len = AES_256_PBKDF2__KEY_LEN;

    self->salt_len = salt_len;
    self->salt_buf = CAPE_MV (p_salt_buf);

    self->iv_len = iv_len;
    self->iv_buf = CAPE_MV (p_iv_buf);

    if (PKCS5_PBKDF2_HMAC (secret, (int)cape_str_size (secret), self->salt_buf, self->salt_len, iterations, EVP_sha256(), self->key_len, self->key) != 1)
    {
        cape_err_set (err, CAPE_ERR_3RDPARTY_LIB, "hmac failed on openssl");
        goto cleanup_and_exit;
    }

    return self;
    
cleanup_and_exit:
    
    qcrypt_aes_keys_del (&self);
    return NULL;
}

//-----------------------------------------------------------------------------

int qcrypt_aes__handle_error (EVP_CIPHER_CTX* ctx, CapeErr err)
{
  unsigned long errCode;
  int res;
  
  CapeStream stream = cape_stream_new ();
  
  while ((errCode = ERR_get_error()))
  {
    cape_stream_append_str (stream, ERR_error_string (errCode, NULL));
  }
  
  res = cape_err_set (err, CAPE_ERR_PROCESS_FAILED, cape_stream_get (stream));
  
  cape_stream_del (&stream);
  
  return res;
}

//-----------------------------------------------------------------------------

#endif

