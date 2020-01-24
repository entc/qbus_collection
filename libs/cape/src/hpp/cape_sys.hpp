#ifndef __CAPE_SYS__HPP__H
#define __CAPE_SYS__HPP__H 1

#include <sys/cape_err.h>
#include <sys/cape_mutex.h>

namespace cape
{
  //======================================================================

  struct ErrHolder
  {
    ErrHolder ()
    {
      err = cape_err_new ();
    }
    
    ~ErrHolder ()
    {
      cape_err_del (&err);
    }
    
    const char* text ()
    {
      return cape_err_text (err);
    }
    
    CapeErr err;
    
  };
  
  //======================================================================
  
  class Mutex
  {
    
  public:
    
    Mutex ()
    {
      m_mutex = cape_mutex_new ();
    }
    
    ~Mutex ()
    {
      cape_mutex_del (&m_mutex);
    }
    
    void lock ()
    {
      cape_mutex_lock (m_mutex);
    }
    
    void unlock ()
    {
      cape_mutex_unlock (m_mutex);
    }
    
  private:
    
    CapeMutex m_mutex;
    
  };

  //======================================================================
  
  class ScopeMutex
  {
    
  public:
    
    ScopeMutex (Mutex& mutex) : m_mutex (mutex)
    {
      m_mutex.lock ();
    }
    
    ~ScopeMutex ()
    {
      m_mutex.unlock();
    }
    
  private:
    
    Mutex& m_mutex;
    
  };
  
  //======================================================================
}

#endif

