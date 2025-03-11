#include "sys/cape_time.h"

// c includes
#include <stdio.h>

int main (int argc, char *argv[])
{
  CapeDatetime utc_time;
  
  cape_datetime_utc (&utc_time);
  
  {
    CapeString s = cape_datetime_s__fmt_lcl (&utc_time, "%Y-%m-%d %H:%M:%S");

    printf ("-> FMT LCL : '%s'\n", s);
    
    cape_str_del (&s);
  }

  {
    CapeString s = cape_datetime_s__fmt_utc (&utc_time, "%Y-%m-%d %H:%M:%S");
    
    printf ("-> FMT UTC : '%s'\n", s);
    
    cape_str_del (&s);
  }

  // print certain formats
  {
    CapeString s = cape_datetime_s__str (&utc_time);
    
    printf ("-> STR     : '%s'\n", s);
    
    cape_str_del (&s);
  }
 
 // print certain formats
 {
   CapeString s = cape_datetime_s__log (&utc_time);
   
   printf ("-> LOG     : '%s'\n", s);
   
   cape_str_del (&s);
 }
 
 // print certain formats
 {
   CapeString s = cape_datetime_s__gmt (&utc_time);
   
   printf ("-> GMT     : '%s'\n", s);
   
   cape_str_del (&s);
 }

 // print certain formats
 {
   CapeString s = cape_datetime_s__pre (&utc_time);
   
   printf ("-> PRE     : '%s'\n", s);
   
   cape_str_del (&s);
 }
 
 // print certain formats
 {
   CapeString s = cape_datetime_s__ISO8601 (&utc_time);
   
   printf ("-> ISO8601 : '%s'\n", s);
   
   cape_str_del (&s);
 }
 
 {
   time_t unix_timestamp = cape_datetime_n__unix (&utc_time); 
   
   printf ("-> UNIX    : '%lu'\n", unix_timestamp);
 }
 
  // cross out datetime
  {
    CapeDatetime tmp_time;
    cape_datetime_utc (&tmp_time);

    cape_datetime_cross__set (&tmp_time);
    
    {
      CapeString s = cape_datetime_s__fmt_lcl (&tmp_time, "%Y-%m-%d %H:%M:%S");
      
      printf ("-> CROSS L : '%s'\n", s);
      
      cape_str_del (&s);
    }
    {
      CapeString s = cape_datetime_s__fmt_utc (&tmp_time, "%x %X");
      
      printf ("-> CROSS U : '%s'\n", s);
      
      cape_str_del (&s);
    }
  }
  
  {
    CapeDatetime tmp_time;
    cape_datetime_utc (&tmp_time);

    cape_datetime_cross__set (&tmp_time);

    {
      CapeDatetime cross_time;
      CapeString h = cape_datetime_s__fmt_lcl (&tmp_time, "%Y-%m-%d %H:%M:%S");

      printf ("-> CROSS O : '%s'\n", h);
      
      // convert text into dateformat
      if (cape_datetime__str_msec (&cross_time, h) || cape_datetime__str (&cross_time, h) || cape_datetime__std_msec (&cross_time, h) || cape_datetime__date_de (&cross_time, h))
      {
        CapeString s = cape_datetime_s__fmt_utc (&cross_time, "%d%m%Y");
        
        printf ("-> CROSS H : '%s'\n", s);
        
        cape_str_del (&s);
      }
      else
      {
        printf ("ERROR: can't convert into date format\n");
      }
      
      cape_str_del (&h);
    }
    {
      CapeDatetime cross_time;
      const CapeString h = "XXXX-XX-03 XX:XX:00";
      
      // convert text into dateformat
      if (cape_datetime__str_msec (&cross_time, h) || cape_datetime__str (&cross_time, h) || cape_datetime__std_msec (&cross_time, h) || cape_datetime__date_de (&cross_time, h))
      {
        // set the original as UTC
        cross_time.is_utc = TRUE;
        
        // convert into local time
        cape_datetime_to_local (&cross_time);

        {
          CapeString s = cape_datetime_s__fmt_utc (&cross_time, "%d%m%Y");
          
          printf ("-> CROSS H : '%s'\n", s);
          
          cape_str_del (&s);
        }
      }
      else
      {
        printf ("ERROR: can't convert into date format\n");
      }
    }
  }
  
  // BCD test
  {
    CapeDatetime dt;
    
    cape_datetime__date_bcd (&dt, 0x20240311, 0x19052400);
    
    {
      CapeString s = cape_datetime_s__str (&dt);
      
      printf ("-> BCD     : '%s'\n", s);
      
      cape_str_del (&s);
    }
  }
  
 return 0;
}
