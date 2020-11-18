#ifndef __ADBL_HPP__H
#define __ADBL_HPP__H 1

#include "adbl.h"
#include "hpp/cape_sys.hpp"
#include "hpp/cape_stc.hpp"

#include <stdexcept>

namespace adbl {
  
  class Session
  {
    
  public:  
  
    //-----------------------------------------------------------------------------
    
    Session () : m_ctx (NULL), m_session (NULL)
    {
    }
    
    //-----------------------------------------------------------------------------
    
    ~Session ()
    {
      if (m_session)
      {
        adbl_session_close (&m_session);
      }
      
      if (m_ctx)
      {
        adbl_ctx_del (&m_ctx);
      }
    }
    
    //-----------------------------------------------------------------------------
    
    int init (CapeErr err)
    {
      // create a new database context
      m_ctx = adbl_ctx_new ("adbl", "adbl2_mysql", err);
      if (m_ctx == NULL)
      {
        return cape_err_code (err);
      }
      
      // connect to database
      m_session = adbl_session_open_file (m_ctx, "adbl_default.json", err);
      if (m_session == NULL)
      {
        return cape_err_code (err);
      }
      
      return CAPE_ERR_NONE;
    }
    
    //-----------------------------------------------------------------------------
    
    cape::Udc query (const char* table, cape::Udc& values)
    {
      cape::ErrHolder errh;
      
      if (NULL == m_session)
      {
        throw std::runtime_error ("ADBL-session was not initialized");
      }
      
      // transfer ownership
      CapeUdc c_values = values.release ();
            
      // execute the query
      CapeUdc h = adbl_session_query (m_session, table, NULL, &c_values, errh.err);
      if (h == NULL)
      {
        throw std::runtime_error (errh.text());
      }
      
      cape::Udc query_results (&h);
      return query_results;
    }
    
    //-----------------------------------------------------------------------------
    
    cape::Udc query (const char* table, cape::Udc& params, cape::Udc& values)
    {
      cape::ErrHolder errh;
      
      if (NULL == m_session)
      {
        throw std::runtime_error ("ADBL-session was not initialized");
      }
      
      // transfer ownership
      CapeUdc c_params = params.release ();
      CapeUdc c_values = values.release ();
      
      // execute the query
      // execute the query
      CapeUdc h = adbl_session_query (m_session, table, &c_params, &c_values, errh.err);
      if (h == NULL)
      {
        throw std::runtime_error (errh.text());
      }

      cape::Udc query_results (&h);
      return query_results;
    }
    
    //-----------------------------------------------------------------------------

    AdblSession c_obj ()
    {
      return m_session;
    }

    //-----------------------------------------------------------------------------

  private:
    
    AdblCtx m_ctx;
    
    AdblSession m_session;
    
    friend class TransactionScope;
    
  };
  
  //-----------------------------------------------------------------------------------------------------
  
  class TransactionScope
  {
    
  public:
    
    TransactionScope (AdblSession session)
    : m_session (session)
    , m_trx (NULL)
    {
    }
    
    //-----------------------------------------------------------------------------
    
    TransactionScope (Session& session)
    : m_session (session.m_session)
    , m_trx (NULL)
    {
    }
    
    //-----------------------------------------------------------------------------
    
    virtual ~TransactionScope ()
    {
      if (m_trx)
      {
        cape::ErrHolder errh;
        
        adbl_trx_rollback (&m_trx, errh.err);
      }
    }
    
    //-----------------------------------------------------------------------------
    
    void start ()
    {
      cape::ErrHolder errh;
      
      if (NULL == m_session)
      {
        throw std::runtime_error ("ADBL-session was not initialized");
      }
      
      m_trx = adbl_trx_new (m_session, errh.err);
      
      if (m_trx == NULL)
      {
        throw std::runtime_error (errh.text());
      }
    }
    
    //-----------------------------------------------------------------------------
    
    void commit ()
    {
      int res;
      cape::ErrHolder errh;
      
      res = adbl_trx_commit (&m_trx, errh.err);
      if (res)
      {
        throw std::runtime_error (errh.text());
      }
    }
    
    //-----------------------------------------------------------------------------
    
    cape::Udc query (const char* table, cape::Udc& values)
    {
      cape::ErrHolder errh;
      
      // transfer ownership
      CapeUdc c_values = values.release ();
      
      CapeUdc h = adbl_trx_query (m_trx, table, NULL, &c_values, errh.err);
      if (h == NULL)
      {
        throw std::runtime_error (errh.text());
      }

      // execute the query
      cape::Udc query_results (&h);
      return query_results;
    }
    
    //-----------------------------------------------------------------------------
    
    cape::Udc query (const char* table, cape::Udc& params, cape::Udc& values)
    {
      cape::ErrHolder errh;
      
      // transfer ownership
      CapeUdc c_params = params.release ();
      CapeUdc c_values = values.release ();
      
      CapeUdc h = adbl_trx_query (m_trx, table, &c_params, &c_values, errh.err);
      if (h == NULL)
      {
        throw std::runtime_error (errh.text());
      }

      // execute the query
      cape::Udc query_results (&h);
      return query_results;
    }
    
    //-----------------------------------------------------------------------------
    
    void update (const char* table, cape::Udc& params, cape::Udc& values)
    {
      cape::ErrHolder errh;
      
      // transfer ownership
      CapeUdc c_params = params.release ();
      CapeUdc c_values = values.release ();

      // execute database statement
      if (adbl_trx_update (m_trx, table, &c_params, &c_values, errh.err))
      {
        throw std::runtime_error (errh.text());
      }
    }
    
    //-----------------------------------------------------------------------------
    
    void del (const char* table, cape::Udc& params)
    {
      cape::ErrHolder errh;

      // transfer ownership
      CapeUdc c_params = params.release ();
      
      if (adbl_trx_delete (m_trx, table, &c_params, errh.err))
      {
        throw std::runtime_error (errh.text());
      }      
    }
    
    //-----------------------------------------------------------------------------
    
    number_t ins (const char* table, cape::Udc& values)
    {
      number_t inserted_id = 0;
      cape::ErrHolder errh;
      
      // transfer ownership
      CapeUdc c_values = values.release ();
            
      // execute database statement
      inserted_id = adbl_trx_insert (m_trx, table, &c_values, errh.err);
      if (inserted_id <= 0)
      {
        std::cout << errh.text() << std::endl;
        
        throw std::runtime_error (errh.text());
      }
      
      return inserted_id;
    }
    
    //-----------------------------------------------------------------------------
    
    number_t insert_or_update (const char* table, cape::Udc& params, cape::Udc& values)
    {
      number_t inserted_id = 0;
      cape::ErrHolder errh;
      
      // transfer ownership
      CapeUdc c_params = params.release ();
      CapeUdc c_values = values.release ();

      // execute database statement
      inserted_id = adbl_trx_inorup (m_trx, table, &c_params, &c_values, errh.err);
      if (inserted_id <= 0)
      {
        std::cout << errh.text() << std::endl;
        
        throw std::runtime_error (errh.text());
      }
      
      return inserted_id;
    }
    
    //-----------------------------------------------------------------------------
    
    number_t insert_or_update__noautoinc (const char* table, cape::Udc& params, cape::Udc& values, const char* column_id)
    {
      number_t inserted_id;
      cape::ErrHolder errh;

      {
        // clone
        CapeUdc c_params = params.clone ().release ();
        CapeUdc c_values = values.clone ().release ();
        
        // for sure set the column id
        cape_udc_put_n (c_values, column_id, 0);
        
        // execute the query
        cape::Udc query_results (adbl_trx_query (m_trx, table, &c_params, &c_values, errh.err));
        if (query_results.empty())
        {
          throw std::runtime_error (errh.text());
        }
        
        cape::Udc first_row = query_results.first();
        if (first_row.valid())
        {
          inserted_id = first_row[column_id];
          
          // transfer ownership
          CapeUdc c_params_upd = params.release ();
          CapeUdc c_values_upd = values.release ();

          // execute database statement
          if (adbl_trx_update (m_trx, table, &c_params_upd, &c_values_upd, errh.err))
          {
            throw std::runtime_error (errh.text());
          }
        }
        else
        {
          // transfer ownership
          CapeUdc c_values_ins = values.release ();
          CapeUdc c_params_ins = params.release ();

          cape_udc_merge_mv (c_values_ins, &c_params_ins);
          
          // execute database statement
          inserted_id = adbl_trx_insert (m_trx, table, &c_values_ins, errh.err);
          if (inserted_id <= 0)
          {
            std::cout << errh.text() << std::endl;
            
            throw std::runtime_error (errh.text());
          }
        }
      }
           
      return inserted_id;
    }

    //-----------------------------------------------------------------------------
    
  protected:
    
    AdblSession m_session;    // reference
    
    AdblTrx m_trx;
    
  };  
  
  //-----------------------------------------------------------------------------------------------------
}

#endif

