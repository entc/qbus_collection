#include "qcrypt_file.h"
#include "qcrypt_decrypt.h"

// cape includes
#include <sys/cape_log.h>
#include <sys/cape_file.h>
#include <fmt/cape_json.h>
#include <stc/cape_str.h>

//-----------------------------------------------------------------------------

int qcrypt_decrypt_file (const CapeString vsec, const CapeString file, void* ptr, fct_cape_fs_file_load cb, CapeErr err)
{
  int res;
  
  // local objects
  CapeFileHandle fh = cape_fh_new (NULL, file);
  QDecryptAES dec = NULL;
  
  // buffers
  char* buffer = CAPE_ALLOC (1024);
  CapeStream outb = cape_stream_new ();

  res = cape_fh_open (fh, O_RDONLY, err);
  if (res)
  {
    goto exit_and_cleanup;
  }
  
  // create the decryption context
  dec = qdecrypt_aes_new (outb, vsec, 0, 0);
  
  while (TRUE)
  {
    number_t bytes_read = cape_fh_read_buf (fh, buffer, 1024);
    if (bytes_read > 0)
    {
      res = qdecrypt_aes_process (dec, buffer, bytes_read, err);
      if (res)
      {
        goto exit_and_cleanup;
      }
      
      res = cb (ptr, cape_stream_data (outb), cape_stream_size (outb), err);
      if (res)
      {
        goto exit_and_cleanup;
      }
      
      cape_stream_clr (outb);
    }
    else
    {
      break;
    }
  }

  res = qdecrypt_aes_finalize (dec, err);
  if (res)
  {
    goto exit_and_cleanup;
  }

  res = cb (ptr, cape_stream_data (outb), cape_stream_size (outb), err);
  if (res)
  {
    goto exit_and_cleanup;
  }

  res = CAPE_ERR_NONE;
  
exit_and_cleanup:

  qdecrypt_aes_del (&dec);

  cape_fh_del (&fh);

  cape_stream_del (&outb);
  CAPE_FREE (buffer);

  return res;
}

//-----------------------------------------------------------------------------

/*
struct QCryptDecrypt_s
{
  CapeString file;
  
  EcDecryptAES dec;
  
  CapeFileHandle fh;
  
  int state;
};

//-----------------------------------------------------------------------------

QCryptDecrypt qcrypt_decrypt_new (const CapeString path, const CapeString file, const CapeString vsec)
{
  QCryptDecrypt self = CAPE_NEW (struct QCryptDecrypt_s);

  // create the file name
  self->file = cape_fs_path_merge (path, file);
  
  // create the file handle
  self->fh = cape_fh_new (NULL, self->file);
  
  // create the decryption context
  self->dec = ecdecrypt_aes_create (vsec, 0, 0);
  
  self->state = TRUE;

  return self;
}

//-----------------------------------------------------------------------------

void qcrypt_decrypt_del (QCryptDecrypt* p_self)
{
  if (*p_self)
  {
    QCryptDecrypt self = *p_self;
    
    if (self->dec)
    {
      ecdecrypt_aes_destroy (&(self->dec));
    }
    
    cape_fh_del (&(self->fh));
    cape_str_del (&(self->file));
    
    CAPE_DEL (p_self, struct QCryptDecrypt_s);
  }
}

//-----------------------------------------------------------------------------

int qcrypt_decrypt_open (QCryptDecrypt self, CapeErr err)
{
  return cape_fh_open (self->fh, O_RDONLY, err);
}

//-----------------------------------------------------------------------------

number_t qcrypt_decrypt_next (QCryptDecrypt self, char* buffer, number_t len)
{
  number_t ret = 0;
  
  if (self->state)
  {
    // entc error object
    EcErr entc_err = ecerr_create ();
    
    // buffers
    EcBuffer bout = NULL;
    
    // use the extern buffer to load the data
    number_t bytes_read = cape_fh_read_buf (self->fh, buffer, len);
    if (bytes_read > 0)
    {
      EcBuffer_s h;
      
      // use the pseudobuffer for the correct
      // bytes_read value as buffer size
      h.buffer = (unsigned char*)buffer;
      h.size = bytes_read;
      
      bout = ecdecrypt_aes_update (self->dec, &h, entc_err);
      if (bout == NULL)
      {
        //res = cape_err_set_fmt (err, CAPE_ERR_RUNTIME, "update AES decryption: %s", entc_err->text);
        //goto exit_and_cleanup;
      }
      else
      {
        // copy data
        ret = bout->size;
        memcpy (buffer, bout->buffer, ret);
      }
    }
    else
    {
      bout = ecdecrypt_aes_finalize (self->dec, entc_err);
      if (bout == NULL)
      {
        //res = cape_err_set_fmt (err, CAPE_ERR_RUNTIME, "finalize AES decryption: %s", entc_err->text);
        //goto exit_and_cleanup;
      }
      else
      {
        // copy data
        ret = bout->size;
        memcpy (buffer, bout->buffer, ret);
      }
      
      self->state = FALSE;
    }
    
    ecerr_destroy (&entc_err);
  }
  
  return ret;
}

//-----------------------------------------------------------------------------

struct QCryptFile_s
{
  EcEncryptAES enc;

  CapeFileHandle fh;
  
  EcErr entc_err;
};

//-----------------------------------------------------------------------------

QCryptFile qcrypt_file_new (const CapeString file)
{
  QCryptFile self = CAPE_NEW (struct QCryptFile_s);
  
  self->enc = NULL;
  self->fh = cape_fh_new (NULL, file);
  
  self->entc_err = ecerr_create ();
  
  return self;
}

//-----------------------------------------------------------------------------

void qcrypt_file_del (QCryptFile* p_self)
{
  if (*p_self)
  {
    QCryptFile self = *p_self;
    
    if (self->enc)
    {
      ecencrypt_aes_destroy (&(self->enc));
    }
    
    if (self->fh)
    {
      cape_fh_del (&(self->fh));
    }
    
    ecerr_destroy (&(self->entc_err));

    CAPE_DEL (p_self, struct QCryptFile_s);
  }
}

//-----------------------------------------------------------------------------

int qcrypt_file_encrypt (QCryptFile self, const CapeString vsec, CapeErr err)
{
  int res;
  
  if (vsec == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_NO_OBJECT, "vsec is not set");
    goto exit_and_cleanup;
  }
  
  // create encryption engine
  self->enc = ecencrypt_aes_create (0, 0, vsec, 0);
  if (self->enc == NULL)
  {
    res = cape_err_set (err, CAPE_ERR_NO_OBJECT, "can't initialize crypt");
    goto exit_and_cleanup;
  }
  
  res = cape_fh_open (self->fh, O_WRONLY | O_TRUNC | O_CREAT, err);
  if (res)
  {
    goto exit_and_cleanup;
  }
  
  res = CAPE_ERR_NONE;
  
exit_and_cleanup:

  return res;
}

//-----------------------------------------------------------------------------

int qcrypt_file_write (QCryptFile self, const char* bufdat, number_t buflen, CapeErr err)
{
  int res;
  
  EcBuffer_s dat;
  EcBuffer buf = NULL;
  
  dat.buffer = (unsigned char*)bufdat;
  dat.size = buflen;
  
  // encrypt the buffer 'decrypted_text'
  buf = ecencrypt_aes_update (self->enc, &dat, self->entc_err);
  if (buf == NULL)
  {
    res = cape_err_set (err, self->entc_err->code, self->entc_err->text);
    goto exit;
  }
  
  cape_fh_write_buf (self->fh, (const char*)buf->buffer, buf->size);
  
  res = CAPE_ERR_NONE;
exit:
  return res;
}

//-----------------------------------------------------------------------------

int qcrypt_file_finalize (QCryptFile self, CapeErr err)
{
  int res = CAPE_ERR_NONE;
  
  EcBuffer buf = NULL;

  // finalize the encryption
  buf = ecencrypt_aes_finalize (self->enc, self->entc_err);
  if (buf == NULL)
  {
    res = cape_err_set (err, self->entc_err->code, self->entc_err->text);
    goto exit;
  }

  cape_fh_write_buf (self->fh, (const char*)buf->buffer, buf->size);

  res = CAPE_ERR_NONE;
exit:
  return res;
}

//-----------------------------------------------------------------------------
*/
