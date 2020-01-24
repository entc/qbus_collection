#include "hpp/cape_stc.hpp"
#include "sys/cape_log.h"

#include <iostream>

int main (int argc, char *argv[])
{
  // ****** UDC *******
  
  // default constructor
  cape::Udc h1;
  
  if (h1.valid())
  {
    cape_log_msg (CAPE_LL_ERROR, "UT", "hpp stc", "default constructor created valid object");
    return 1;
  }
  
  // exception test
  try
  {
    h1.add (23);    

    cape_log_msg (CAPE_LL_ERROR, "UT", "hpp stc", "exception #1");
    return 1;
  }
  catch (cape::Exception& e)
  {
    
  }
  
  h1 = cape::Udc (CAPE_UDC_LIST);

  if (!h1.valid())
  {
    cape_log_msg (CAPE_LL_ERROR, "UT", "hpp stc", "assignment #1");
    return 1;
  }
  
  // add 
  for (int i = 0; i < 12; i++)
  {
    h1.add (i);
  }
  
  // use the to string  
  std::cout << h1.to_string () << std::endl;

  // use the << operator
  std::cout << h1 << std::endl;
  
  cape::Udc val01 (CAPE_UDC_NODE);

  val01.add ("col01", "hello");
  val01.add ("col02", std::string ("world"));
  val01.add ("col03", 10);
  val01.add ("col04", 3.141592653589793238462643383279502884197169399375105820974944592307816406286);
  val01.add ("col05", true);
  
  // do some checking
  cape::Udc col01;
  
  col01 = val01.get ("col01");
  
  if (!col01.valid())
  {
    cape_log_msg (CAPE_LL_ERROR, "UT", "hpp stc", "get #1");
    return 1;   
  }

  // do some checking
  col01 = val01["col01"];
  
  if (!col01.valid())
  {
    cape_log_msg (CAPE_LL_ERROR, "UT", "hpp stc", "get #2");
    return 1;   
  }

  // retrieve
  std::string s2 = col01;
  
  std::cout << "col01 = " << s2 << std::endl;
  
  double d1 = val01["col04"];

  std::cout << "col04 = " << d1 << std::endl;
  
  std::string s3 = val01["col04"].as("");
  
  std::cout << "col04 = " << s3 << std::endl;

  try
  {
    col01 = val01["col007"];
    
    cape_log_msg (CAPE_LL_ERROR, "UT", "hpp stc", "get #3");
    return 1;   
  }
  catch (cape::Exception& e)
  {
    
  }
  
  
  
  h1.add (val01);
 
  // use the << operator
  std::cout << h1 << std::endl;
  
  
  cape::Stream stream1;
  
  stream1 << "Hello" << std::string("World") << '\n' << d1 << '\n';
  
  
  std::cout << "S:'" << stream1 << "'" << std::endl;
}

