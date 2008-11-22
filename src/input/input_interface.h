/*****************************************************************************
 * input_interface.h: Input functions usable ouside input code.
 *****************************************************************************
 * Copyright (C) 1998-2008 the VideoLAN team
 * $Id$
 *
 * Authors: Laurent Aimar < fenrir _AT_ videolan _DOT_ org >
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

#if defined(__PLUGIN__) || defined(__BUILTIN__) || !defined(__LIBVLC__)
# error This header file can only be included from LibVLC.
#endif

#ifndef _INPUT_INTERFACE_H
#define _INPUT_INTERFACE_H 1

#include <vlc_input.h>
#include <libvlc.h>

/**********************************************************************
 * Item metadata
 **********************************************************************/
int  input_ArtFind( playlist_t *, input_item_t * );
int  input_DownloadAndCacheArt( playlist_t *, input_item_t * );

void input_item_SetPreparsed( input_item_t *p_i, bool b_preparsed );

typedef struct playlist_album_t
{
    char *psz_artist;
    char *psz_album;
    char *psz_arturl;
    bool b_found;
} playlist_album_t;


void input_item_SetArtNotFound( input_item_t *p_i, bool b_not_found );
void input_item_SetArtFetched( input_item_t *p_i, bool b_art_fetched );

/* misc/stats.c
 * FIXME it should NOT be defined here or not coded in misc/stats.c */
input_stats_t *stats_NewInputStats( input_thread_t *p_input );

/* input.c */
#define input_CreateThreadExtended(a,b,c,d) __input_CreateThreadExtended(VLC_OBJECT(a),b,c,d)
input_thread_t *__input_CreateThreadExtended ( vlc_object_t *, input_item_t *, const char *, sout_instance_t * );

sout_instance_t * input_DetachSout( input_thread_t *p_input );

#endif