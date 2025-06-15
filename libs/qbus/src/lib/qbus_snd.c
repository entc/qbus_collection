#include "qbus_snd.h"

// cape includes
#include <stc/cape_str.h>
#include <stc/cape_map.h>
#include <sys/cape_log.h>
#include <sys/cape_file.h>
#include <sys/cape_dl.h>

//-----------------------------------------------------------------------------

struct QBusSnd_s
{
  
};

//-----------------------------------------------------------------------------

QBusSnd qbus_snd_new ()
{
  QBusSnd self = CAPE_NEW (struct QBusSnd_s);
  

  return self;
}

//-----------------------------------------------------------------------------

void qbus_snd_del (QBusSnd* p_self)
{
  if (*p_self)
  {
    QBusSnd self = *p_self;
    
    
    CAPE_DEL (p_self, struct QBusSnd_s);
  }
}

//-----------------------------------------------------------------------------
