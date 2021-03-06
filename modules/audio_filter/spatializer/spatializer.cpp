/*****************************************************************************
 * spatializer.cpp: sound reverberation
 *****************************************************************************
 * Copyright (C) 2004, 2006, 2007 the VideoLAN team
 *
 * Google Summer of Code 2007
 *
 * Authors: Biodun Osunkunle <biodun@videolan.org>
 *
 * Mentor : Jean-Baptiste Kempf <jb@videolan.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

/*****************************************************************************
 * Preamble
 *****************************************************************************/
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdlib.h>                                      /* malloc(), free() */
#include <math.h>

#include <new>
using std::nothrow;

#include <vlc_common.h>
#include <vlc_plugin.h>
#include <vlc_aout.h>

#include "revmodel.hpp"
#define SPAT_AMP 0.3

/*****************************************************************************
 * Module descriptor
 *****************************************************************************/
static int  Open ( vlc_object_t * );
static void Close( vlc_object_t * );

#define ROOMSIZE_TEXT N_("Room size")
#define ROOMSIZE_LONGTEXT N_("Defines the virtual surface of the room" \
                             " emulated by the filter." )

#define WIDTH_TEXT N_("Room width")
#define WIDTH_LONGTEXT N_("Width of the virtual room")

#define WET_TEXT N_("Wet")
#define WET_LONGTEXT NULL

#define DRY_TEXT N_("Dry")
#define DRY_LONGTEXT NULL

#define DAMP_TEXT N_("Damp")
#define DAMP_LONGTEXT NULL

vlc_module_begin ()
    set_description( N_("Audio Spatializer") )
    set_shortname( N_("Spatializer" ) )
    set_capability( "audio filter", 0 )
    set_category( CAT_AUDIO )
    set_subcategory( SUBCAT_AUDIO_AFILTER )

    set_callbacks( Open, Close )
    add_shortcut( "spatializer" )
    add_float( "spatializer-roomsize", 1.05, NULL, ROOMSIZE_TEXT,
               ROOMSIZE_LONGTEXT, true )
    add_float( "spatializer-width", 10., NULL, WIDTH_TEXT,WIDTH_LONGTEXT, true )
    add_float( "spatializer-wet", 3., NULL, WET_TEXT,WET_LONGTEXT, true )
    add_float( "spatializer-dry", 2., NULL, DRY_TEXT,DRY_LONGTEXT, true )
    add_float( "spatializer-damp", 1., NULL, DAMP_TEXT,DAMP_LONGTEXT, true )
vlc_module_end ()

/*****************************************************************************
 * Local prototypes
 *****************************************************************************/
struct aout_filter_sys_t
{
    vlc_mutex_t lock;
    revmodel *p_reverbm;
};

#define DECLARECB(fn) static int fn (vlc_object_t *,char const *, \
                                     vlc_value_t, vlc_value_t, void *)
DECLARECB( RoomCallback  );
DECLARECB( WetCallback   );
DECLARECB( DryCallback   );
DECLARECB( DampCallback  );
DECLARECB( WidthCallback );

#undef  DECLARECB

struct callback_s {
  const char *psz_name;
  int (*fp_callback)(vlc_object_t *,const char *,
                     vlc_value_t,vlc_value_t,void *);
  void (revmodel::* fp_set)(float);
};

static const callback_s callbacks[] = {
    { "spatializer-roomsize", RoomCallback,  &revmodel::setroomsize },
    { "spatializer-width",    WidthCallback, &revmodel::setwidth },
    { "spatializer-wet",      WetCallback,   &revmodel::setwet },
    { "spatializer-dry",      DryCallback,   &revmodel::setdry },
    { "spatializer-damp",     DampCallback,  &revmodel::setdamp }
};
enum { num_callbacks=sizeof(callbacks)/sizeof(callback_s) };

static void DoWork( aout_instance_t *, aout_filter_t *,
                    aout_buffer_t *, aout_buffer_t * );

/*****************************************************************************
 * Open:
 *****************************************************************************/
static int Open( vlc_object_t *p_this )
{
    aout_filter_t     *p_filter = (aout_filter_t *)p_this;
    aout_filter_sys_t *p_sys;
    aout_instance_t   *p_aout = (aout_instance_t *)p_filter->p_parent;
    bool               b_fit = true;
    msg_Dbg( p_this, "Opening filter spatializer" );

    if( p_filter->input.i_format != VLC_CODEC_FL32 ||
        p_filter->output.i_format != VLC_CODEC_FL32 )
    {
        b_fit = false;
        p_filter->input.i_format = VLC_CODEC_FL32;
        p_filter->output.i_format = VLC_CODEC_FL32;
        msg_Warn( p_filter, "bad input or output format" );
    }
    if ( !AOUT_FMTS_SIMILAR( &p_filter->input, &p_filter->output ) )
    {
        b_fit = false;
        memcpy( &p_filter->output, &p_filter->input,
                sizeof(audio_sample_format_t) );
        msg_Warn( p_filter, "input and output formats are not similar" );
    }

    if ( ! b_fit )
    {
        return VLC_EGENERIC;
    }

    p_filter->pf_do_work = DoWork;
    p_filter->b_in_place = true;

     /* Allocate structure */
    p_sys = p_filter->p_sys = (aout_filter_sys_t*)malloc( sizeof( aout_filter_sys_t ) );
    if( !p_sys )
        return VLC_ENOMEM;

    /* Force new to return 0 on failure instead of throwing, since we don't
       want an exception to leak back to C code. Bad things would happen. */
    p_sys->p_reverbm = new (nothrow) revmodel;
    if( !p_sys->p_reverbm )
    {
        free( p_sys );
        return VLC_ENOMEM;
    }

    vlc_mutex_init( &p_sys->lock );

    for(unsigned i=0;i<num_callbacks;++i)
    {
        /* NOTE: C++ pointer-to-member function call from table lookup. */
        (p_sys->p_reverbm->*(callbacks[i].fp_set))
            (var_CreateGetFloatCommand(p_aout,callbacks[i].psz_name));
        var_AddCallback( p_aout, callbacks[i].psz_name,
                         callbacks[i].fp_callback, p_sys );
    }

    return VLC_SUCCESS;
}

/*****************************************************************************
 * Close: close the plugin
 *****************************************************************************/
static void Close( vlc_object_t *p_this )
{
    aout_filter_t     *p_filter = (aout_filter_t *)p_this;
    aout_filter_sys_t *p_sys = p_filter->p_sys;
    aout_instance_t *p_aout = (aout_instance_t *)p_filter->p_parent;

    /* Delete the callbacks */
    for(unsigned i=0;i<num_callbacks;++i)
    {
        var_DelCallback( p_aout, callbacks[i].psz_name,
                         callbacks[i].fp_callback, p_sys );
    }

    delete p_sys->p_reverbm;
    vlc_mutex_destroy( &p_sys->lock );
    free( p_sys );
    msg_Dbg( p_this, "Closing filter spatializer" );
}

/*****************************************************************************
 * SpatFilter: process samples buffer
 * DoWork: call SpatFilter
 *****************************************************************************/

static inline
void SpatFilter( aout_instance_t *p_aout, aout_filter_t *p_filter,
                 float *out, float *in, int i_samples, int i_channels )
{
    aout_filter_sys_t *p_sys = p_filter->p_sys;
    vlc_mutex_locker locker( &p_sys->lock );

    int i, ch;
    for( i = 0; i < i_samples; i++ )
    {
        for( ch = 0 ; ch < 2; ch++)
        {
            in[ch] = in[ch] * SPAT_AMP;
        }
        p_sys->p_reverbm->processreplace( in, out , 1, i_channels);
        in  += i_channels;
        out += i_channels;
    }
}

static void DoWork( aout_instance_t * p_aout, aout_filter_t * p_filter,
                    aout_buffer_t * p_in_buf, aout_buffer_t * p_out_buf )
{
    p_out_buf->i_nb_samples = p_in_buf->i_nb_samples;
    p_out_buf->i_nb_bytes = p_in_buf->i_nb_bytes;

    SpatFilter( p_aout, p_filter, (float*)p_out_buf->p_buffer,
               (float*)p_in_buf->p_buffer, p_in_buf->i_nb_samples,
               aout_FormatNbChannels( &p_filter->input ) );
}


/*****************************************************************************
 * Variables callbacks
 *****************************************************************************/

static int RoomCallback( vlc_object_t *p_this, char const *psz_cmd,
                         vlc_value_t oldval, vlc_value_t newval, void *p_data )
{
    (void)psz_cmd;    (void)oldval;
    aout_filter_sys_t *p_sys = (aout_filter_sys_t*)p_data;
    vlc_mutex_locker locker( &p_sys->lock );

    p_sys->p_reverbm->setroomsize(newval.f_float);
    msg_Dbg( p_this, "room size is now %3.1f", newval.f_float );
    return VLC_SUCCESS;
}

static int WidthCallback( vlc_object_t *p_this, char const *psz_cmd,
                         vlc_value_t oldval, vlc_value_t newval, void *p_data )
{
    (void)psz_cmd;    (void)oldval;
    aout_filter_sys_t *p_sys = (aout_filter_sys_t*)p_data;
    vlc_mutex_locker locker( &p_sys->lock );

    p_sys->p_reverbm->setwidth(newval.f_float);
    msg_Dbg( p_this, "width is now %3.1f", newval.f_float );
    return VLC_SUCCESS;
}
static int WetCallback( vlc_object_t *p_this, char const *psz_cmd,
                         vlc_value_t oldval, vlc_value_t newval, void *p_data )
{
    (void)psz_cmd;    (void)oldval;
    aout_filter_sys_t *p_sys = (aout_filter_sys_t*)p_data;
    vlc_mutex_locker locker( &p_sys->lock );

    p_sys->p_reverbm->setwet(newval.f_float);
    msg_Dbg( p_this, "'wet' value is now %3.1f", newval.f_float );
    return VLC_SUCCESS;
}
static int DryCallback( vlc_object_t *p_this, char const *psz_cmd,
                         vlc_value_t oldval, vlc_value_t newval, void *p_data )
{
    (void)psz_cmd;    (void)oldval;
    aout_filter_sys_t *p_sys = (aout_filter_sys_t*)p_data;
    vlc_mutex_locker locker( &p_sys->lock );

    p_sys->p_reverbm->setdry(newval.f_float);
    msg_Dbg( p_this, "'dry' value is now %3.1f", newval.f_float );
    return VLC_SUCCESS;
}
static int DampCallback( vlc_object_t *p_this, char const *psz_cmd,
                         vlc_value_t oldval, vlc_value_t newval, void *p_data )
{
    (void)psz_cmd;    (void)oldval;
    aout_filter_sys_t *p_sys = (aout_filter_sys_t*)p_data;
    vlc_mutex_locker locker( &p_sys->lock );

    p_sys->p_reverbm->setdamp(newval.f_float);
    msg_Dbg( p_this, "'damp' value is now %3.1f", newval.f_float );
    return VLC_SUCCESS;
}

