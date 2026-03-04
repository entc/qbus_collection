#include "qwave_config.h"

// cape includes
#include <stc/cape_map.h>

//-----------------------------------------------------------------------------

struct QWaveConfig_s
{
    CapeUdc route_list;
    CapeMap sites;
};

//-----------------------------------------------------------------------------

QWaveConfig qwave_config_new ()
{
    QWaveConfig self = CAPE_NEW (struct QWaveConfig_s);
    
    self->route_list = NULL;
    self->sites = NULL;
    
    return self;
}

//-----------------------------------------------------------------------------

void qwave_config_del (QWaveConfig* p_self)
{
    if (*p_self)
    {
        QWaveConfig self = *p_self;
        
        cape_map_del (&(self->sites));
        cape_udc_del (&(self->route_list));
        
        CAPE_DEL (p_self, struct QWaveConfig_s);
    }
}

//-----------------------------------------------------------------------------

void __STDCALL qwave_config__on_del (void* key, void* val)
{
    {
        CapeString h = key; cape_str_del (&h);
    }
    {
        CapeString h = val; cape_str_del (&h);
    }
}

//-----------------------------------------------------------------------------

void qwave_config_set (QWaveConfig self, CapeUdc parameters)
{
    CapeUdc sites = cape_udc_get (parameters, "sites");
    
    // extract route list
    self->route_list = cape_udc_ext (parameters, "route_list");
        
    if (sites)
    {
        self->sites = cape_map_new (cape_map__compare__s, qwave_config__on_del, NULL);
        
        {
            CapeUdcCursor* cursor = cape_udc_cursor_new (sites, CAPE_DIRECTION_FORW);
                        
            while (cape_udc_cursor_next (cursor))
            {
                cape_map_insert (self->sites, cape_str_cp (cape_udc_name (cursor->item)), cape_str_cp (cape_udc_s (cursor->item, NULL)));
            }
            
            cape_udc_cursor_del (&cursor);
        }
    }
}

//-----------------------------------------------------------------------------

int qwave_config_route (QWaveConfig self, const CapeString name)
{
    if (self->route_list)
    {
        return cape_udc_get (self->route_list, name) ? TRUE : FALSE;
    }
    else
    {
        return FALSE;
    }
}

//-----------------------------------------------------------------------------

const CapeString qwave_config_site_get (QWaveConfig self, const CapeString part)
{
    if (self->sites)
    {
        CapeMapNode n = cape_map_find (self->sites, part);
        if (n)
        {
            return cape_map_node_value (n);
        }
        else
        {
            return NULL;
        }
    }
    
    return NULL;
}

//-----------------------------------------------------------------------------

const CapeString qwave_config_site (QWaveConfig self, CapeString* p_url)
{
    const CapeString ret;
    
    // local objects
    const CapeString url = *p_url;
    
    if ('/' == *url)
    {
        number_t pos;
        
        // find the next '/' in the url
        if (cape_str_next (url + 1, '/', &pos))
        {
            CapeString url_part = cape_str_sub (url, pos + 1);
            
            ret = qwave_config_site_get (self, url_part);
            
            cape_str_del (&url_part);
            
            if (ret)
            {
                CapeString url_new = cape_str_sub (url + pos + 1, cape_str_size (url) - pos - 1);

                // replace current url with the adjusted one
                cape_str_replace_mv (p_url, &url_new);
                
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
                cape_str_replace_cp (p_url, "/");
                
                goto exit_and_cleanup;
            }
        }
    }
    
    ret = qwave_config_site_get (self, "/");
    
exit_and_cleanup:
    
    return ret;
}

//-----------------------------------------------------------------------------
