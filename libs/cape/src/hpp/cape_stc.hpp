#ifndef __CAPE_STC__HPP__H
#define __CAPE_STC__HPP__H 1

#include <stc/cape_stream.h>
#include <stc/cape_list.h>
#include <stc/cape_udc.h>
#include <fmt/cape_json.h>
#include <sys/cape_log.h>

// STL includes
#include <limits>
#include <string>
#include <iostream>
#include <sstream>
#include <type_traits>

#include <hpp/cape_sys.hpp>

namespace cape
{
  // forward declaration
  class UdcCursor;

  //-----------------------------------------------------------------------------------------------------

  struct ListHolder
  {
    ListHolder (CapeList obj) : m_obj (obj) {}

    ~ListHolder () { cape_list_del (&m_obj); }

    CapeList m_obj;
  };

  //-----------------------------------------------------------------------------------------------------

  struct String
  {
    String (CapeString obj) : m_obj (obj) {}
    String (CapeString* p_obj) : m_obj (cape_str_mv (p_obj)) {}

    ~String () { cape_str_del (&m_obj); }

    std::string to_string () { return std::string (m_obj); }

    CapeString m_obj;
  };

  //-----------------------------------------------------------------------------------------------------

  template <typename S> struct StringTrans
  {
    static const char* c_str (S const &s) { return s.c_str(); }
  };

  // a traits prototype for UDC types
  template <typename T> struct UdcTransType
  {
    //static void add_cp (CapeUdc, const char*, const T&) {  }
    //static void add_mv (CapeUdc, const char*, T&) {  }
    //static T as (CapeUdc) {}
  };

  //-----------------------------------------------------------------------------------------------------

  // a traits prototype for UDC types
  template <typename T> struct StreamTransType { };

  //-----------------------------------------------------------------------------------------------------

  class Stream
  {

  public:

    Stream () : m_obj (cape_stream_new ())
    {
    }

    //-----------------------------------------------------------------------------

    // copy constructor
    Stream (const Stream& rhs) : m_obj (NULL)
    {
      if (rhs.m_obj)
      {
        m_obj = cape_stream_new ();

        cape_stream_append_stream (m_obj, rhs.m_obj);
      }
    }

    //-----------------------------------------------------------------------------

    // move constructor
    Stream (Stream&& rhs) : m_obj (rhs.release())
    {
    }

    //-----------------------------------------------------------------------------

    ~Stream ()
    {
      cape_stream_del (&m_obj);
    }

    //-----------------------------------------------------------------------------

    CapeStream obj ()
    {
      return m_obj;
    }

    //-----------------------------------------------------------------------------

    CapeStream release ()
    {
      if (m_obj == NULL)
      {
        throw cape::Exception (CAPE_ERR_NO_OBJECT, "Stream object has no content");
      }

      CapeStream ret = m_obj;

      m_obj = NULL;

      return ret;
    }

    //-----------------------------------------------------------------------------

    Stream& operator =(const Stream& rhs)
    {
      if(this == &rhs)
      {
        return *this;
      }

      // always create a copy
      cape_stream_append_stream (m_obj, rhs.m_obj);

      return *this;
    }

    //-----------------------------------------------------------------------------

    Stream& operator =(Stream&& rhs)
    {
      if(this == &rhs)
      {
        return *this;
      }

      // move
      m_obj = rhs.release();

      return *this;
    }

    //-----------------------------------------------------------------------------

    void clear () const
    {
      if (m_obj == NULL)
      {
        throw cape::Exception (CAPE_ERR_NO_OBJECT, "Stream object has no content");
      }

      cape_stream_clr (m_obj);
    }

    //-----------------------------------------------------------------------------

    const char* data () const
    {
      if (m_obj == NULL)
      {
        throw cape::Exception (CAPE_ERR_NO_OBJECT, "Stream object has no content");
      }

      return cape_stream_data (m_obj);
    }

    //-----------------------------------------------------------------------------

    number_t size ()
    {
      if (m_obj == NULL)
      {
        throw cape::Exception (CAPE_ERR_NO_OBJECT, "Stream object has no content");
      }

      return cape_stream_size (m_obj);
    }

    //-----------------------------------------------------------------------------

    friend std::ostream& operator<<(std::ostream& os, const Stream& stream)
    {
      if (stream.m_obj == NULL)
      {
        throw cape::Exception (CAPE_ERR_NO_OBJECT, "Stream object has no content");
      }

      const CapeString h = cape_stream_get (stream.m_obj);

      return os << h;
    }

    //-----------------------------------------------------------------------------

    template <typename T> cape::Stream& operator<<(T val)
    {
      if (m_obj == NULL)
      {
        throw cape::Exception (CAPE_ERR_NO_OBJECT, "Stream object has no content");
      }

      cape::StreamTransType<T>::append (m_obj, val);

      return *this;
    }

    //-----------------------------------------------------------------------------

    cape::Stream& operator<<(const char* val)
    {
      if (m_obj == NULL)
      {
        throw cape::Exception (CAPE_ERR_NO_OBJECT, "Stream object has no content");
      }

      cape_stream_append_str (m_obj, val);

      return *this;
    }

    //-----------------------------------------------------------------------------

    operator const char*()
    {
      if (m_obj == NULL)
      {
        throw cape::Exception (CAPE_ERR_NO_OBJECT, "Stream object has no content");
      }

      return cape_stream_get (m_obj);
    }

    //-----------------------------------------------------------------------------

    /*
    cape::Stream& operator<<(std::ostream&(*f)(std::ostream&))
    {
      static_assert (false, "ostream is not supported for cape::Stream at the moment -- * if know the black magic please change it in cape_stc.hpp *");

      return *this;
    }
    */

    //-----------------------------------------------------------------------------

    bool empty ()
    {
      return m_obj == NULL;
    }

    //-----------------------------------------------------------------------------

    bool valid ()
    {
      return m_obj != NULL;
    }

    //-----------------------------------------------------------------------------

    void append (const char* text)
    {
      if (m_obj == NULL)
      {
        throw cape::Exception (CAPE_ERR_NO_OBJECT, "Stream object has no content");
      }

      cape_stream_append_str (m_obj, text);
    }

    //-----------------------------------------------------------------------------

    void append (number_t val)
    {
      if (m_obj == NULL)
      {
        throw cape::Exception (CAPE_ERR_NO_OBJECT, "Stream object has no content");
      }

      cape_stream_append_n (m_obj, val);
    }

    //-----------------------------------------------------------------------------

    void append (char val)
    {
      if (m_obj == NULL)
      {
        throw cape::Exception (CAPE_ERR_NO_OBJECT, "Stream object has no content");
      }

      cape_stream_append_c (m_obj, val);
    }

    //-----------------------------------------------------------------------------

    template <typename T> void append (T& val)
    {
      if (m_obj == NULL)
      {
        throw cape::Exception (CAPE_ERR_NO_OBJECT, "Stream object has no content");
      }

      cape::StreamTransType<T>::append (m_obj, val);
    }

    //-----------------------------------------------------------------------------

    number_t append (const char* bufdat, number_t buflen)
    {
      if (m_obj == NULL)
      {
        throw cape::Exception (CAPE_ERR_NO_OBJECT, "Stream object has no content");
      }

      cape_stream_append_buf (m_obj, bufdat, buflen);
      
      return buflen;
    }

    //-----------------------------------------------------------------------------

    void append_u08 (cape_uint8 val)
    {
      if (m_obj == NULL)
      {
        throw cape::Exception (CAPE_ERR_NO_OBJECT, "Stream object has no content");
      }

      cape_stream_append_08 (m_obj, val);
    }

    //-----------------------------------------------------------------------------

    void append_u16 (cape_uint16 val, bool network_byte_order)
    {
      if (m_obj == NULL)
      {
        throw cape::Exception (CAPE_ERR_NO_OBJECT, "Stream object has no content");
      }

      cape_stream_append_16 (m_obj, val, network_byte_order);
    }

    //-----------------------------------------------------------------------------

    void append_u32 (cape_uint32 val, bool network_byte_order)
    {
      if (m_obj == NULL)
      {
        throw cape::Exception (CAPE_ERR_NO_OBJECT, "Stream object has no content");
      }

      cape_stream_append_32 (m_obj, val, network_byte_order ? TRUE : FALSE);
    }

    //-----------------------------------------------------------------------------

    void append_u64 (cape_uint64 val, bool network_byte_order)
    {
      if (m_obj == NULL)
      {
        throw cape::Exception (CAPE_ERR_NO_OBJECT, "Stream object has no content");
      }

      cape_stream_append_64 (m_obj, val, network_byte_order ? TRUE : FALSE);
    }

    //-----------------------------------------------------------------------------

    cape::String to_str ()
    {
      return cape::String (cape_stream_to_str (&m_obj));
    }

    //-----------------------------------------------------------------------------

    const CapeString to_cstr () const
    {
      return cape_stream_to_s (m_obj);
    }

    //-----------------------------------------------------------------------------

    CapeString to_cstr ()
    {
      return cape_stream_to_str (&m_obj);
    }

  private:

    CapeStream m_obj;

  };

  //-----------------------------------------------------------------------------------------------------

  template <> struct StreamTransType<int>
  {
    static void append (CapeStream obj, int value)
    {
      cape_stream_append_n (obj, value);
    }
  };

  //-----------------------------------------------------------------------------------------------------

  template <> struct StreamTransType<unsigned int>
  {
    static void append (CapeStream obj, unsigned int value)
    {
      cape_stream_append_n (obj, (int)value);
    }
  };

  //-----------------------------------------------------------------------------------------------------

  template <> struct StreamTransType<number_t>
  {
    static void append (CapeStream obj, number_t value)
    {
      cape_stream_append_n (obj, value);
    }
  };

  //-----------------------------------------------------------------------------------------------------

  template <> struct StreamTransType<double>
  {
    static void append (CapeStream obj, double value)
    {
      cape_stream_append_f (obj, value);
    }
  };

  //-----------------------------------------------------------------------------------------------------

  template <> struct StreamTransType<float>
  {
    static void append (CapeStream obj, float value)
    {
      cape_stream_append_f (obj, (double)value);
    }
  };

  //-----------------------------------------------------------------------------------------------------

  template <> struct StreamTransType<char>
  {
    static void append (CapeStream obj, char value)
    {
      cape_stream_append_c (obj, value);
    }
  };

  //-----------------------------------------------------------------------------------------------------

  template <> struct StreamTransType<std::string>
  {
    static void append (CapeStream obj, std::string& value)
    {
      cape_stream_append_buf (obj, value.c_str(), value.size());
    }
  };

  //-----------------------------------------------------------------------------------------------------

  template <> struct StreamTransType<std::stringstream>
  {
    static void append (CapeStream obj, std::stringstream& value)
    {
      // WARNING: old compilers use a static buffer to convert to std::string
      cape_stream_append_str (obj, value.str().c_str());
    }
  };

  //-----------------------------------------------------------------------------------------------------

  class Udc
  {

  public:

    //------------------------ CONSTRUCTOR -----------------------

    Udc (bool owned = false) : m_owned (owned), m_obj (NULL)  // empty
    {
    }

    //-----------------------------------------------------------------------------

    Udc (CapeUdc* p_obj) : m_owned (true), m_obj (cape_udc_mv (p_obj)) {}

    //-----------------------------------------------------------------------------

    Udc (CapeUdc obj) : m_owned (false), m_obj (obj)
    {
    }

    //-----------------------------------------------------------------------------

    template <typename S> Udc (int type, S const &name) : m_owned (true), m_obj (cape_udc_new (type, StringTrans<S>::c_str(name))) {}
    Udc (int type, const char* name) : m_owned (true), m_obj (cape_udc_new (type, name)) {}

    //-----------------------------------------------------------------------------

    Udc (int type) : m_owned (true), m_obj (cape_udc_new (type, NULL))
    {
    }

    //-----------------------------------------------------------------------------

    // copy constructor
    Udc (const Udc& e) : m_owned (true), m_obj (cape_udc_cp (e.m_obj))
    {
    }

    //-----------------------------------------------------------------------------

    // move constructor
    Udc (Udc&& e) : m_owned (e.m_owned), m_obj (cape_udc_mv (&(e.m_obj)))
    {
    }

    //-----------------------------------------------------------------------------

    // destructor
    ~Udc ()
    {
      if (m_owned)
      {
        cape_udc_del (&m_obj);
      }
    }

    //------------------------ ADD -----------------------

    void add (const char* name, const char* value)
    {
      if (m_obj == NULL)
      {
        std::string error_message = "UDC object has no content: {add} name = " + std::string(name) + ", values = " + std::string(value);

        throw cape::Exception (CAPE_ERR_NO_OBJECT, error_message.c_str());
      }

      cape_udc_add_s_cp (m_obj, name, value);
    }

    //-----------------------------------------------------------------------------

    void add (const char* name, CapeString* p_value)
    {
      if (m_obj == NULL)
      {
        std::string error_message = "UDC object has no content: {add} name = " + std::string(name);

        throw cape::Exception (CAPE_ERR_NO_OBJECT, error_message.c_str());
      }

      cape_udc_add_s_mv (m_obj, name, p_value);
    }

    //-----------------------------------------------------------------------------

    void add (const char* name, CapeDatetime* value)
    {
      if (m_obj == NULL)
      {
        std::string error_message = "UDC object has no content: {add} name = " + std::string(name);

        throw cape::Exception (CAPE_ERR_NO_OBJECT, error_message.c_str());
      }

      cape_udc_add_d (m_obj, name, value);
    }

    //-----------------------------------------------------------------------------

    template <typename S, typename T> void add (S const &name, T const &val)
    {
      if (m_obj == NULL)
      {
        std::string error_message = "UDC object has no content: {add} name = " + std::string(name);

        throw cape::Exception (CAPE_ERR_NO_OBJECT, error_message.c_str());
      }

      UdcTransType<T>::add_cp (m_obj, StringTrans<S>::c_str (name), val);
    }

    //-----------------------------------------------------------------------------

    template <typename T> void add (const char* name, T const &val)
    {
      if (m_obj == NULL)
      {
        std::string error_message = "UDC object has no content: {add} name = " + std::string(name);

        throw cape::Exception (CAPE_ERR_NO_OBJECT, error_message.c_str());
      }

      UdcTransType<T>::add_cp (m_obj, name, val);
    }

    //-----------------------------------------------------------------------------

    template <typename T> void add (const char* name, T &val)
    {
      if (m_obj == NULL)
      {
        std::string error_message = "UDC object has no content: {add} name = " + std::string(name);

        throw cape::Exception (CAPE_ERR_NO_OBJECT, error_message.c_str());
      }

      UdcTransType<T>::add_cp (m_obj, name, val);
    }

    //-----------------------------------------------------------------------------

    template <typename T> void add (const char* name, T&& val)
    {
      if (m_obj == NULL)
      {
        std::string error_message = "UDC object has no content: {add} name = " + std::string(name);

        throw cape::Exception (CAPE_ERR_NO_OBJECT, error_message.c_str());
      }

      UdcTransType<T>::add_mv (m_obj, name, val);
    }

    //-----------------------------------------------------------------------------

    template <typename T> void add (T&& val)
    {
      if (m_obj == NULL)
      {
        std::string error_message = "UDC object has no content: {add}";

        throw cape::Exception (CAPE_ERR_NO_OBJECT, error_message.c_str());
      }

      UdcTransType<T>::add_mv (m_obj, NULL, val);
    }

    //-----------------------------------------------------------------------------

    template <typename T> const T& operator =(T const &val)
    {
      if (m_obj == NULL)
      {
        std::string error_message = "UDC object has no content: {=}";

        throw cape::Exception (CAPE_ERR_NO_OBJECT, error_message.c_str());
      }

      UdcTransType<T>::set (m_obj, val);

      return val;
    }

    //-----------------------------------------------------------------------------

    Udc& operator =(const Udc& rhs)
    {
      if(this == &rhs)
      {
        return *this;
      }

      // always create a copy
      cape_udc_replace_cp (&m_obj, rhs.m_obj);
      m_owned = true;

      return *this;
    }

    //-----------------------------------------------------------------------------

    Udc& operator =(Udc& rhs)
    {
      if(this == &rhs)
      {
        return *this;
      }

      cape_udc_replace_cp (&m_obj, rhs.m_obj);
      m_owned = true;

      return *this;
    }

    //-----------------------------------------------------------------------------

    Udc& operator =(Udc&& rhs)
    {
      if(this == &rhs)
      {
        return *this;
      }

      if (rhs.m_owned)
      {
        // the other object ownes it -> transfer ownership
        cape_udc_replace_mv (&m_obj, &(rhs.m_obj));
      }
      else
      {
        // copy the reference
        m_obj = rhs.m_obj;
      }

      m_owned = rhs.m_owned;

      return *this;
    }

    //-----------------------------------------------------------------------------

    void reset_as_reference (const Udc& rhs)
    {
      clear ();

      m_obj = rhs.m_obj;
      m_owned = false;
    }

    //-----------------------------------------------------------------------------

    void reset_as_reference (CapeUdc obj)
    {
      clear ();

      m_obj = obj;
      m_owned = false;
    }

    //-----------------------------------------------------------------------------

    void reset (Udc&& rhs)
    {
      clear ();

      if (rhs.m_owned)
      {
        m_obj = cape_udc_mv (&(rhs.m_obj));
      }
      else
      {
        // copy the reference
        m_obj = rhs.m_obj;
      }

      m_owned = rhs.m_owned;
    }

    //-----------------------------------------------------------------------------

    void reset (CapeUdc* p_obj)
    {
      clear ();

      m_owned = true;
      m_obj = cape_udc_mv (p_obj);
    }

    //-----------------------------------------------------------------------------

    friend std::ostream& operator<<(std::ostream& os, const Udc& udc)
    {
      return os << udc.to_string();
    }

    //-----------------------------------------------------------------------------

    template <typename T> operator T()
    {
      return UdcTransType<T>::as (m_obj);
    }

    //-----------------------------------------------------------------------------

    template <typename T> T as (T const& default_value)
    {
      if (m_obj == NULL)
      {
        std::string error_message = "UDC object has no content: {as}";

        throw cape::Exception (CAPE_ERR_NO_OBJECT, error_message.c_str());
      }

      return UdcTransType<T>::as (m_obj, default_value);
    }

    //-----------------------------------------------------------------------------

    const char* as (const char* default_value)
    {
      if (m_obj == NULL)
      {
        std::string error_message = "UDC object has no content: {as} value = " + std::string(default_value);

        throw cape::Exception (CAPE_ERR_NO_OBJECT, error_message.c_str());
      }

      return cape_udc_s (m_obj, default_value);
    }

    //-----------------------------------------------------------------------------

    operator const char*()
    {
      return cape_udc_s (m_obj, NULL);
    }

    //-----------------------------------------------------------------------------

    Udc first () const
    {
      if (m_obj == NULL)
      {
        std::string error_message = "UDC object has no content: {first}";

        throw cape::Exception (CAPE_ERR_NO_OBJECT, error_message.c_str());
      }

      return Udc (cape_udc_get_first (m_obj));
    }

    //-----------------------------------------------------------------------------

    Udc first_ext ()
    {
      if (m_obj == NULL)
      {
        std::string error_message = "UDC object has no content: {first}";

        throw cape::Exception (CAPE_ERR_NO_OBJECT, error_message.c_str());
      }

      CapeUdc h = cape_udc_ext_first (m_obj);
      return Udc (&h);
    }

    //-----------------------------------------------------------------------------
    
    Udc last () const
    {
      if (m_obj == NULL)
      {
        std::string error_message = "UDC object has no content: {last}";
        
        throw cape::Exception (CAPE_ERR_NO_OBJECT, error_message.c_str());
      }
      
      return Udc (cape_udc_get_last (m_obj));
    }
    
    //-----------------------------------------------------------------------------

    template <typename S> Udc get (const S& name)
    {
      if (m_obj == NULL)
      {
        std::string error_message = "UDC object has no content: {get} name = " + std::string(name);

        throw cape::Exception (CAPE_ERR_NO_OBJECT, error_message.c_str());
      }

      return Udc (cape_udc_get (m_obj, StringTrans<S>::c_str(name)));
    }

    //-----------------------------------------------------------------------------

    Udc get (const char* name)
    {
      return Udc (cape_udc_get (m_obj, name));
    }

    //-----------------------------------------------------------------------------

    template <typename S, typename T> T get (const S& name, T const& default_value)
    {
      if (m_obj == NULL)
      {
        std::string error_message = "UDC object has no content: {as}";

        throw cape::Exception (CAPE_ERR_NO_OBJECT, error_message.c_str());
      }

      CapeUdc h = cape_udc_get (m_obj, StringTrans<S>::c_str(name));
      if (h)
      {
        return UdcTransType<T>::as (h, default_value);
      }
      else
      {
        return default_value;
      }
    }

    //-----------------------------------------------------------------------------

    template <typename T> T get (const char* name, T const& default_value)
    {
      if (m_obj == NULL)
      {
        std::string error_message = "UDC object has no content: {as}";

        throw cape::Exception (CAPE_ERR_NO_OBJECT, error_message.c_str());
      }

      CapeUdc h = cape_udc_get (m_obj, name);
      if (h)
      {
        return UdcTransType<T>::as (h, default_value);
      }
      else
      {
        return default_value;
      }
    }

    //-----------------------------------------------------------------------------

    Udc get_or_create (const char* name, int type)
    {
      if (m_obj == NULL)
      {
        std::string error_message = "UDC object has no content: {get} name = " + std::string(name);

        throw cape::Exception (CAPE_ERR_NO_OBJECT, error_message.c_str());
      }

      CapeUdc h = cape_udc_get (m_obj, name);
      if (NULL == h)
      {
        CapeUdc h2 = cape_udc_new (type, name);

        h = cape_udc_add (m_obj, &h2);
      }

      return Udc (h);
    }

    //-----------------------------------------------------------------------------

    Udc operator[] (const char* name)
    {
      CapeUdc h = cape_udc_get (m_obj, name);

      if (h == NULL)
      {
        std::string h ("UDC has not such an entry: ");
        h.append (name);

        throw cape::Exception (CAPE_ERR_NOT_FOUND, h.c_str());
      }

      return Udc (h);
    }

    //-----------------------------------------------------------------------------

    template <typename S> Udc ext (const S& name)
    {
      CapeUdc h = cape_udc_ext (m_obj, StringTrans<S>::c_str(name));

      return h == NULL ? Udc () : Udc (&h);
    }

    //-----------------------------------------------------------------------------

    Udc ext (const char* name)
    {
      CapeUdc h = cape_udc_ext (m_obj, name);

      return h == NULL ? Udc () : Udc (&h);
    }

    //-----------------------------------------------------------------------------

    template <typename S, typename T> void put (const S& name, T const& value)
    {
      if (m_obj == NULL)
      {
        std::string error_message = "UDC object has no content: {put} name = " + std::string(name);

        throw cape::Exception (CAPE_ERR_NO_OBJECT, error_message.c_str());
      }

      UdcTransType<T>::put (m_obj, StringTrans<S>::c_str(name), value);
    }

    //-----------------------------------------------------------------------------

    template <typename T> void put (const char* name, T const& value)
    {
      if (m_obj == NULL)
      {
        std::string error_message = "UDC object has no content: {put} name = " + std::string(name);

        throw cape::Exception (CAPE_ERR_NO_OBJECT, error_message.c_str());
      }

      UdcTransType<T>::put (m_obj, name, value);
    }

    //-----------------------------------------------------------------------------

    const CapeUdc obj () const
    {
      if (m_obj == NULL)
      {
        std::string error_message = "UDC object has no content: {obj}";

        throw cape::Exception (CAPE_ERR_NO_OBJECT, error_message.c_str());
      }

      return m_obj;
    }

    //-----------------------------------------------------------------------------

    CapeUdc release ()
    {
      if (m_obj == NULL)
      {
        std::string error_message = "UDC object has no content: {release}";

        throw cape::Exception (CAPE_ERR_NO_OBJECT, error_message.c_str());
      }

      if (false == m_owned)
      {
        throw cape::Exception (CAPE_ERR_RUNTIME, "UDC object is not owned");
      }

      return cape_udc_mv (&m_obj);
    }

    //-----------------------------------------------------------------------------

    void clear ()
    {
      if (m_obj)
      {
        if (m_owned)
        {
          cape_udc_del (&m_obj);

          m_owned = false;
        }

        m_obj = NULL;
      }
    }

    //-----------------------------------------------------------------------------

    Udc clone () const
    {
      if (m_obj == NULL)
      {
        std::string error_message = "UDC object has no content: {clone}";

        throw cape::Exception (CAPE_ERR_NO_OBJECT, error_message.c_str());
      }

      CapeUdc h = cape_udc_cp (m_obj);

      return Udc (&h);
    }

    //-----------------------------------------------------------------------------

    CapeUdc clone_or_release ()
    {
      if (m_obj == NULL)
      {
        std::string error_message = "UDC object has no content: {clone_or_release}";

        throw cape::Exception (CAPE_ERR_NO_OBJECT, error_message.c_str());
      }

      if (m_owned)
      {
        return cape_udc_mv (&m_obj);
      }
      else
      {
        return cape_udc_cp (m_obj);
      }
    }

    //-----------------------------------------------------------------------------

    void merge (const cape::Udc& merger)
    {
      cape_udc_merge_cp (m_obj, merger.m_obj);
    }

    //-----------------------------------------------------------------------------

    const char* name () const
    {
      if (m_obj == NULL)
      {
        std::string error_message = "UDC object has no content: {name}";

        throw cape::Exception (CAPE_ERR_NO_OBJECT, error_message.c_str());
      }

      return cape_udc_name (m_obj);
    }

    //-----------------------------------------------------------------------------

    std::string to_string () const
    {
      if (m_obj == NULL)
      {
        std::string error_message = "UDC object has no content: {to_string}";

        throw cape::Exception (CAPE_ERR_NO_OBJECT, error_message.c_str());
      }

      cape::String h (cape_json_to_s (m_obj));

      return h.to_string ();
    }

    //-----------------------------------------------------------------------------

    void from_string (const char* json_as_text)
    {
      if (false == m_owned)
      {
        throw cape::Exception (CAPE_ERR_WRONG_STATE, "UDC object is not owned");
      }

      {
        // parse into a new CapeUdc
        CapeUdc h = cape_json_from_s (json_as_text);

        if (h == NULL)
        {
          throw cape::Exception (CAPE_ERR_PARSER, "Can't parse json string");
        }

        // replace the object with the outcome of the JSON parser
        cape_udc_replace_mv (&m_obj, &h);
      }
    }

    //-----------------------------------------------------------------------------

    void from_file (const char* json_file)
    {
      if (false == m_owned)
      {
        throw cape::Exception (CAPE_ERR_WRONG_STATE, "UDC object is not owned");
      }

      {
        ErrHolder errh;

        // try to read the json structure from the file
        CapeUdc h = cape_json_from_file (json_file, errh.err);

        if (NULL == h)
        {
          throw cape::Exception (errh.code(), errh.text());
        }

        // replace the object with the outcome of the JSON parser
        cape_udc_replace_mv (&m_obj, &h);
      }
    }

    //-----------------------------------------------------------------------------

    bool empty () const
    {
      return m_obj == NULL;
    }

    //-----------------------------------------------------------------------------

    bool valid () const
    {
      return m_obj != NULL;
    }

    //-----------------------------------------------------------------------------

    int type () const
    {
      if (m_obj == NULL)
      {
        std::string error_message = "UDC object has no content: {type}";

        throw cape::Exception (CAPE_ERR_NO_OBJECT, error_message.c_str());
      }

      return cape_udc_type (m_obj);
    }

    //-----------------------------------------------------------------------------

    number_t size () const
    {
      return cape_udc_size (m_obj);
    }

  private:

    bool m_owned;

    // the cape object
    CapeUdc m_obj;

    friend UdcCursor;
  };

  //-----------------------------------------------------------------------------------------------------

  template <> struct UdcTransType<int>
  {
    static void add_cp (CapeUdc obj, const char* name, const int& value) { cape_udc_add_n (obj, name, value); }
    static void add_mv (CapeUdc obj, const char* name, int& value) { cape_udc_add_n (obj, name, value); }
    static int as (CapeUdc obj, int dv = 0) { return (int)cape_udc_n (obj, dv); }
    static void put (CapeUdc obj, const char* name, const int& value) { cape_udc_put_n (obj, name, value); }
    static void set (CapeUdc obj, const int& value) { cape_udc_set_n (obj, value); }
  };

  //-----------------------------------------------------------------------------------------------------

  template <> struct UdcTransType<int&>
  {
    static void add_mv (CapeUdc obj, const char* name, int& value) { cape_udc_add_n (obj, name, value); }
  };

  //-----------------------------------------------------------------------------------------------------

  template <> struct UdcTransType<number_t>
  {
    static void add_cp (CapeUdc obj, const char* name, const long& value) { cape_udc_add_n (obj, name, value); }
    static void add_mv (CapeUdc obj, const char* name, number_t& value) { cape_udc_add_n (obj, name, value); }
    static long as (CapeUdc obj, long dv = 0) { return cape_udc_n (obj, dv); }
    static void put (CapeUdc obj, const char* name, const number_t& value) { cape_udc_put_n (obj, name, value); }
    static void set (CapeUdc obj, const number_t value) { cape_udc_set_n (obj, value); }
  };

  template <> struct UdcTransType<number_t&>
  {
    static void add_cp (CapeUdc obj, const char* name, const long& value) { cape_udc_add_n (obj, name, value); }
    static void add_mv (CapeUdc obj, const char* name, number_t& value) { cape_udc_add_n (obj, name, value); }
    static long as (CapeUdc obj, long dv = 0) { return cape_udc_n (obj, dv); }
    static void put (CapeUdc obj, const char* name, const number_t& value) { cape_udc_put_n (obj, name, value); }
    static void set (CapeUdc obj, const number_t& value) { cape_udc_set_n (obj, value); }
  };

  template <> struct UdcTransType<unsigned int&>
  {
    static void add_cp (CapeUdc obj, const char* name, const unsigned int value) { cape_udc_add_n (obj, name, value); }
    static void add_mv (CapeUdc obj, const char* name, unsigned int value) { cape_udc_add_n (obj, name, value); }
    static long as (CapeUdc obj, long dv = 0) { return cape_udc_n (obj, dv); }
  };

  template <> struct UdcTransType<unsigned int>
  {
    static void add_cp (CapeUdc obj, const char* name, unsigned int const & value) { cape_udc_add_n (obj, name, (int)value); }
    static void add_mv (CapeUdc obj, const char* name, unsigned int& value) { cape_udc_add_n (obj, name, (int&)value); }
    static unsigned int as (CapeUdc obj, unsigned int dv = 0) { return (unsigned int)cape_udc_n (obj, dv); }
  };

  template <> struct UdcTransType<double>
  {
    static void add_cp (CapeUdc obj, const char* name, const double& value) { cape_udc_add_f (obj, name, value); }
    static void add_mv (CapeUdc obj, const char* name, double& value) { cape_udc_add_f (obj, name, value); }
    static double as (CapeUdc obj, double dv = .0) { return cape_udc_f (obj, dv); }
  };

  template <> struct UdcTransType<double&>
  {
//    static void add_cp (CapeUdc obj, const char* name, const double& value) { cape_udc_add_f (obj, name, value); }
    static void add_mv (CapeUdc obj, const char* name, double& value) { cape_udc_add_f (obj, name, value); }
//    static double as (CapeUdc obj, double dv = .0) { return cape_udc_f (obj, dv); }
  };

  template <> struct UdcTransType<float>
  {
    static void add_cp (CapeUdc obj, const char* name, float const& value) { cape_udc_add_f (obj, name, (double)value); }
    static void add_mv (CapeUdc obj, const char* name, float& value) { cape_udc_add_f (obj, name, (double&)value); }
    static float as (CapeUdc obj, float dv = .0) { return (float)cape_udc_f (obj, dv); }
  };

  template <> struct UdcTransType<bool>
  {
    static void add_cp (CapeUdc obj, const char* name, const bool& value) { cape_udc_add_b (obj, name, value ? TRUE : FALSE); }
    static void add_mv (CapeUdc obj, const char* name, bool& value) { cape_udc_add_b (obj, name, value ? TRUE : FALSE); }
    static bool as (CapeUdc obj, bool dv = false) { return cape_udc_b (obj, dv ? TRUE : FALSE) == TRUE ? true : false; }
    static void put (CapeUdc obj, const char* name, const bool& value) { cape_udc_put_b (obj, name, value ? TRUE : FALSE); }
    static void set (CapeUdc obj, const bool& value) { cape_udc_set_b (obj, value ? TRUE : FALSE); }
  };

  template <> struct UdcTransType<const CapeDatetime*>
  {
    static void add_cp (CapeUdc obj, const char* name, const CapeDatetime* value) { cape_udc_add_d (obj, name, value); }
    static const CapeDatetime* as (CapeUdc obj, const CapeDatetime* dv = NULL) { return cape_udc_d (obj, dv); }
  };

  template <> struct UdcTransType<std::string>
  {
    static void add_cp (CapeUdc obj, const char* name, const std::string& value) { cape_udc_add_s_cp (obj, name, value.c_str()); }
    static void add_mv (CapeUdc obj, const char* name, std::string& value) { cape_udc_add_s_cp (obj, name, value.c_str()); }
    static std::string as (CapeUdc obj, const char* dv = "") { return std::string (cape_udc_s (obj, dv)); }
  };

  template <> struct UdcTransType<std::string&>
  {
    static void add_cp (CapeUdc obj, const char* name, const std::string& value) { cape_udc_add_s_cp (obj, name, value.c_str()); }
    static void add_mv (CapeUdc obj, const char* name, std::string& value) { cape_udc_add_s_cp (obj, name, value.c_str()); }
    static std::string as (CapeUdc obj, const char* dv = "") { return std::string (cape_udc_s (obj, dv)); }
  };

  template <> struct UdcTransType<cape::String>
  {
    static void add_cp (CapeUdc obj, const char* name, const cape::String& value) { cape_udc_add_s_cp (obj, name, value.m_obj); }
    static void add_mv (CapeUdc obj, const char* name, cape::String& value) { cape_udc_add_s_mv (obj, name, &(value.m_obj)); }
    static cape::String as (CapeUdc obj, const char* dv = "") { return cape::String (cape_str_cp (cape_udc_s (obj, dv))); }
  };

  template <> struct UdcTransType<Udc>
  {
    static void add_cp (CapeUdc obj, const char* name, const Udc& value)
    {
      CapeUdc h = cape_udc_cp (value.obj());

      if (name)
      {
        cape_udc_add_name (obj, &h, name);
      }
      else
      {
        cape_udc_add (obj, &h);
      }
    }
    static void add_mv (CapeUdc obj, const char* name, Udc& value)
    {
      CapeUdc h = value.clone_or_release ();

      if (name)
      {
        cape_udc_add_name (obj, &h, name);
      }
      else
      {
        cape_udc_add (obj, &h);
      }
    }
    static Udc as (CapeUdc obj) { return Udc (obj); }
  };

  template <> struct UdcTransType<Udc&>
  {
    static void add_mv (CapeUdc obj, const char* name, Udc& value)
    {
      CapeUdc h = value.clone_or_release();

      if (name)
      {
        cape_udc_add_name (obj, &h, name);
      }
      else
      {
        cape_udc_add (obj, &h);
      }
    }
  };

  template <> struct UdcTransType<cape::Stream&>
  {
    static void add_cp (CapeUdc obj, const char* name, const cape::Stream& value)
    {
      const CapeString h = value.to_cstr ();

      cape_udc_add_s_cp (obj, name, h);
    }

    static void add_mv (CapeUdc obj, const char* name, cape::Stream& value)
    {
      CapeString h = value.to_cstr ();

      cape_udc_add_s_mv (obj, name, &h);
    }
  };

  //-----------------------------------------------------------------------------------------------------

  class UdcCursor
  {

  public:

    UdcCursor (cape::Udc& udc, int dir = CAPE_DIRECTION_FORW)
    : m_udc (udc)
    , m_cursor (cape_udc_cursor_new (udc.m_obj, dir))
    {
    }

    ~UdcCursor ()
    {
      cape_udc_cursor_del (&m_cursor);
    }

    //-----------------------------------------------------------------------------

    bool next ()
    {
      bool ret = cape_udc_cursor_next (m_cursor) == TRUE;

      if (ret)
      {
        m_item.reset (m_cursor->item);
      }
      else
      {
        m_item.clear ();
      }

      return ret;
    }

    //-----------------------------------------------------------------------------

    Udc operator[] (const char* name)
    {
      return m_item[name];
    }

    //-----------------------------------------------------------------------------

    cape::Udc& item ()
    {
      return m_item;
    }

    //-----------------------------------------------------------------------------

    number_t position () const
    {
      return m_cursor->position;
    }

    //-----------------------------------------------------------------------------

    void remove_at_position ()
    {
      cape_udc_cursor_rm (m_udc.m_obj, m_cursor);

      m_item.reset_as_reference (m_cursor->item);
    }

    //-----------------------------------------------------------------------------

    std::string to_string () const
    {
      return m_item.to_string ();
    }

    //-----------------------------------------------------------------------------

    friend std::ostream& operator<<(std::ostream& os, const UdcCursor& cursor)
    {
      return os << cursor.to_string();
    }

    //-----------------------------------------------------------------------------

  private:

    cape::Udc& m_udc;
    CapeUdcCursor* m_cursor;

    cape::Udc m_item;

  };


  //-----------------------------------------------------------------------------------------------------

  class UdcCursorConst
  {

  public:

    UdcCursorConst (const CapeUdc& udc)
    : m_cursor (cape_udc_cursor_new (udc, CAPE_DIRECTION_FORW))
    {
    }

    ~UdcCursorConst ()
    {
      cape_udc_cursor_del (&m_cursor);
    }

    //-----------------------------------------------------------------------------

    bool next ()
    {
      return cape_udc_cursor_next (m_cursor) == TRUE;
    }

    //-----------------------------------------------------------------------------

    cape::Udc item () const
    {
      return cape::Udc (m_cursor->item);
    }

    //-----------------------------------------------------------------------------

    number_t position () const
    {
      return m_cursor->position;
    }

    //-----------------------------------------------------------------------------

  private:

    CapeUdcCursor* m_cursor;

  };

  //-----------------------------------------------------------------------------------------------------

  struct UdcCursorHolder
  {
    UdcCursorHolder (CapeUdcCursor* obj) : m_obj (obj) {}

    UdcCursorHolder (cape::Udc& udc, int dir = CAPE_DIRECTION_FORW) : m_obj (cape_udc_cursor_new (udc.obj(), dir)) {}

    UdcCursorHolder (const cape::Udc& udc, int dir = CAPE_DIRECTION_FORW) : m_obj (cape_udc_cursor_new (udc.obj(), dir)) {}

    ~UdcCursorHolder () { cape_udc_cursor_del (&m_obj); }

    bool next () { return TRUE == cape_udc_cursor_next (m_obj); }

    CapeUdc item () { return m_obj->item; }

    void rm_item (cape::Udc& udc)
    {
      CapeUdc h = cape_udc_cursor_ext (udc.obj(), m_obj);
      cape_udc_del (&h);
    }

    CapeUdcCursor* m_obj;
  };

}

#endif
