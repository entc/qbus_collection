#include "qwave_config.h"

//-----------------------------------------------------------------------------

struct QWaveConfig_s
{
    CapeUdc route_list;
    
};

//-----------------------------------------------------------------------------

QWaveConfig qwave_config_new ()
{
    QWaveConfig self = CAPE_NEW (struct QWaveConfig_s);
    
    self->route_list = NULL;
    
    return self;
}

//-----------------------------------------------------------------------------

void qwave_config_del (QWaveConfig* p_self)
{
    if (*p_self)
    {
        QWaveConfig self = *p_self;
        
        cape_udc_del (&(self->route_list));
        
        CAPE_DEL (p_self, struct QWaveConfig_s);
    }
}

//-----------------------------------------------------------------------------

const CapeString qwave_config_site_get (QWaveConfig self, const CapeString part)
{
    /*
    CapeMapNode n = cape_map_find (self->sites, part);
    if (n)
    {
        return cape_map_node_value (n);
    }
    else
    {
        return NULL;
    }
    */
    
    return NULL;
}

//-----------------------------------------------------------------------------

const CapeString qwave_config_site (QWaveConfig self, const char *bufdat, size_t buflen, CapeString* p_url)
{
    const CapeString ret;
    
    // local objects
    CapeString url = cape_str_sub (bufdat, buflen);
    
    if ('/' == *url)
    {
        number_t pos;
        if (cape_str_next (url + 1, '/', &pos))
        {
            CapeString h = cape_str_sub (url, pos + 1);
            
            ret = qwave_config_site_get (self, h);
            
            cape_str_del (&h);
            
            if (ret)
            {
                *p_url = cape_str_sub (url + pos + 1, cape_str_size (url) - pos - 1);
                goto exit_and_cleanup;
            }
        }
        else
        {
            ret = qwave_config_site_get (self, url);
            if (ret)
            {
                // this means the whole url is a site
                // -> re-write to /
                *p_url = cape_str_cp ("/");
                goto exit_and_cleanup;
            }
        }
    }
    
    ret = qwave_config_site_get (self, "/");
    *p_url = cape_str_mv (&url);
    
exit_and_cleanup:
    
    cape_str_del (&url);
    return ret;
}

//-----------------------------------------------------------------------------

int qwave_config_route (QWaveConfig self, const CapeString name)
{
    CapeUdc h = cape_udc_get (self->route_list, name);
    
    return h ? TRUE : FALSE;
}

//-----------------------------------------------------------------------------
