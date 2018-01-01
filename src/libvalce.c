/* libvalce.c *
 * Copyright (C) 2017 Nick *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#include <glib.h>
#include <glib/gprintf.h>
#include <glib-object.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <float.h>
#include <math.h>
#include <vlc/vlc.h>
#include "libvalce.h"

#define _g_free0(var) (var = (g_free (var), NULL))
#define _g_object_unref0(var) ((var == NULL) ? NULL : (var = (g_object_unref (var), NULL)))

enum  {
	      VALCE_PLAYER_0_PROPERTY,
        VALCE_PLAYER_VOLUME_PROPERTY,
        VALCE_PLAYER_CURRENT_CHAPTER_INDEX_PROPERTY,
        VALCE_PLAYER_MEDIA_INFO_PROPERTY,
       	VALCE_PLAYER_STATE_PROPERTY,
	      VALCE_PLAYER_MUTE_PROPERTY,
	      VALCE_PLAYER_NUM_PROPERTIES
};

static guint valce_player_signals[VALCE_PLAYER_NUM_SIGNALS] = {0};

struct _ValcePlayerPrivate {

        libvlc_instance_t*    inst;
        libvlc_media_player_t*  mp;
        libvlc_media_t*         md;
        libvlc_event_manager_t* ev;
        gint64                 dur;
        gdouble volume;
        ValceMediaInfo media_info;
       	gint curent_chapter;
       	ValcePlayerState state;
	      gboolean muted;

};

static gpointer valce_player_parent_class = NULL;
static GParamSpec* valce_player_properties[VALCE_PLAYER_NUM_PROPERTIES];

GType valce_player_state_get_type (void) G_GNUC_CONST;
GType valce_player_get_type (void) G_GNUC_CONST;

#define VALCE_PLAYER_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), VALCE_TYPE_PLAYER, ValcePlayerPrivate))

static void g_cclosure_user_marshal_VOID__INT64 (GClosure* closure, GValue* return_value,
                                                  guint n_param_values, const GValue* param_values,
                                                  gpointer invocation_hint, gpointer marshal_data);
static void g_cclosure_user_marshal_VOID__INT (GClosure* closure, GValue* return_value,
                                                  guint n_param_values, const GValue* param_values,
                                                  gpointer invocation_hint, gpointer marshal_data);
static GObject* valce_player_constructor (GType type, guint n_construct_properties,
                                           GObjectConstructParam* construct_properties);
static ValcePlayer* valce_player_construct (GType object_type);

static void valce_player_get_media_info (ValcePlayer* self, ValceMediaInfo* result);

static gboolean valce_player_get_muted (ValcePlayer* self);
static void valce_player_set_muted (ValcePlayer* self, gboolean value);

static ValcePlayerState valce_player_get_state  (ValcePlayer* self);
static void valce_player_set_state (ValcePlayer* self, ValcePlayerState value);

static ValceChapter*  valce_chapter_dup      (const ValceChapter* self);
static void           valce_chapter_destroy  (ValceChapter* self);

static ValceMediaInfo* valce_media_info_dup (const ValceMediaInfo* self);
static void            valce_media_info_destroy (ValceMediaInfo* self);

static libvlc_instance_t* vlc_instance_new (void);

static void mp_time_changed_cb    (const struct libvlc_event_t* event, void* self);
static void mp_length_changed_cb  (const struct libvlc_event_t* event, void* self);
static void mp_volume_changed_cb  (const struct libvlc_event_t* event, void* self);
static void mp_meta_changed_cb    (const struct libvlc_event_t* event, void* self);
static void mp_state_changed_cb   (const struct libvlc_event_t* event, void* self);
static void mp_chapter_changed_cb (const struct libvlc_event_t* event, void* self);
static void mp_media_changed_cb   (const struct libvlc_event_t* event, void* self);
static void mp_muted_cb           (const struct libvlc_event_t* event, void* self);
static void mp_unmuted_cb         (const struct libvlc_event_t* event, void* self);

static void valce_player_finalize (GObject* obj);
static void _vala_valce_player_get_property (GObject * object, guint property_id, GValue * value, GParamSpec * pspec);
static void _vala_valce_player_set_property (GObject * object, guint property_id, const GValue * value, GParamSpec * pspec);



void valce_chapter_copy (const ValceChapter* self, ValceChapter* dest) {

  (*dest).offset = (*self).offset;
	(*dest).duration = (*self).duration;
	_g_free0 ((*dest).title);
	(*dest).title = g_strdup ( (*self).title );
}

static void valce_chapter_destroy (ValceChapter* self) {

	_g_free0 ((*self).title);
}

static ValceChapter* valce_chapter_dup (const ValceChapter* self) {

  ValceChapter* dup;
	dup = g_new0 (ValceChapter, 1);
	valce_chapter_copy (self, dup);
	return dup;
}

void valce_chapter_free (ValceChapter* self) {

	valce_chapter_destroy (self);
	g_free (self);
}

GType valce_chapter_get_type (void) {
	static volatile gsize valce_chapter_type_id__volatile = 0;
	if (g_once_init_enter (&valce_chapter_type_id__volatile)) {
		GType valce_chapter_type_id;
		valce_chapter_type_id = g_boxed_type_register_static ("ValceChapter", (GBoxedCopyFunc) valce_chapter_dup,
                                                                       (GBoxedFreeFunc) valce_chapter_free);
		g_once_init_leave (&valce_chapter_type_id__volatile, valce_chapter_type_id);
	}
	return valce_chapter_type_id__volatile;
}

void valce_media_info_copy (const ValceMediaInfo* self, ValceMediaInfo* dest) {

  (*dest).track_number = (*self).track_number;
	(*dest).track_total = (*self).track_total;
  (*dest).disk_number = (*self).track_number;
	(*dest).disk_total = (*self).disk_total;
	_g_free0 ( (*dest).title );
	(*dest).title = g_strdup ( (*self).title );
	_g_free0 ( (*dest).artist );
	(*dest).artist = g_strdup ( (*self).artist );
	_g_free0 ( (*dest).album_artist );
	(*dest).album_artist = g_strdup ( (*self).album_artist );
	_g_free0 ( (*dest).album );
	(*dest).album = g_strdup ( (*self).album );
	_g_free0 ( (*dest).genre );
	(*dest).genre = g_strdup ( (*self).genre );
	_g_free0 ( (*dest).date );
	(*dest).date = g_strdup ( (*self).date );
}

static void valce_media_info_destroy (ValceMediaInfo* self) {

	_g_free0 ((*self).title);
	_g_free0 ( (*self).artist );
	_g_free0 ( (*self).album_artist );
	_g_free0 ( (*self).album );
	_g_free0 ( (*self).genre );
	_g_free0 ( (*self).date );
}

static ValceMediaInfo* valce_media_info_dup (const ValceMediaInfo* self) {

        ValceMediaInfo* dup;
	dup = g_new0 (ValceMediaInfo, 1);
	valce_media_info_copy (self, dup);
	return dup;
}

void valce_media_info_free (ValceMediaInfo* self) {

	valce_media_info_destroy (self);
	g_free (self);
}

GType valce_media_info_get_type (void) {
	static volatile gsize valce_media_info_type_id__volatile = 0;
	if (g_once_init_enter (&valce_media_info_type_id__volatile)) {
		GType valce_media_info_type_id;
		valce_media_info_type_id = g_boxed_type_register_static ("ValceMediaInfo", (GBoxedCopyFunc) valce_media_info_dup,
                                                                       (GBoxedFreeFunc) valce_media_info_free);
		g_once_init_leave (&valce_media_info_type_id__volatile, valce_media_info_type_id);
	}
	return valce_media_info_type_id__volatile;
}

GType valce_player_state_get_type (void) {

	static volatile gsize valce_player_state_type_id__volatile = 0;
	if (g_once_init_enter (&valce_player_state_type_id__volatile)) {
		            static const GEnumValue values[] = {{VALCE_PLAYER_STATE_STOPPED, "VALCE_PLAYER_STATE_STOPPED", "stopped"},
                                                    {VALCE_PLAYER_STATE_PAUSED,  "VALCE_PLAYER_STATE_PAUSED",  "paused" },
                                                    {VALCE_PLAYER_STATE_PLAYING, "VALCE_PLAYER_STATE_PLAYING", "playing"},
                                                    {VALCE_PLAYER_STATE_OTHER,   "VALCE_PLAYER_STATE_OTHER",   "other"  },
                                                    {0, NULL, NULL}};
		            GType valce_player_state_type_id;
		            valce_player_state_type_id = g_enum_register_static ("ValcePlayerState", values);
		            g_once_init_leave (&valce_player_state_type_id__volatile, valce_player_state_type_id);
	}
	return valce_player_state_type_id__volatile;
}

static ValcePlayer* valce_player_construct (GType object_type) {

        ValcePlayer* self = NULL;
	      self = (ValcePlayer*) g_object_new (object_type, NULL);
        libvlc_set_user_agent (self->priv->inst, "libvalce (VLC)", "libvalce (VLC)");
        libvlc_set_app_id (self->priv->inst, "com.github.nvlgit.libvalce", "0.1.0", "system-run" );

	      return self;
}

ValcePlayer* valce_player_new (void) {
	      return valce_player_construct (VALCE_TYPE_PLAYER);
}

gboolean valce_player_set_uri (ValcePlayer* self, const gchar* uri) {

        g_return_val_if_fail (self != NULL, FALSE);
	      g_return_val_if_fail (uri != NULL, FALSE);

        self->priv->md = libvlc_media_new_location(self->priv->inst,
                                                   (const char*) uri);
        if (self->priv->md == NULL)
                return FALSE;

        if ( g_str_has_prefix (uri, "file:///") ) {
                libvlc_media_parse_with_options (self->priv->md,
                                 libvlc_media_parse_local, 100);
        } else {
                libvlc_media_parse_with_options (self->priv->md,
                                 libvlc_media_parse_network, 200);
        }

        libvlc_media_player_set_media (self->priv->mp,
                                       self->priv->md);

        libvlc_media_release(self->priv->md);
        return TRUE;
}

void valce_player_play (ValcePlayer* self) {

	      g_return_if_fail (self != NULL);
        libvlc_media_player_play (self->priv->mp);
}

void valce_player_pause (ValcePlayer* self) {

	      g_return_if_fail (self != NULL);
        libvlc_media_player_pause (self->priv->mp);
}

void valce_player_stop (ValcePlayer* self) {

	      g_return_if_fail (self != NULL);
	      libvlc_media_player_stop (self->priv->mp);
}

static ValcePlayerState valce_player_get_state  (ValcePlayer* self) {

        libvlc_state_t s = libvlc_NothingSpecial;
        ValcePlayerState state = VALCE_PLAYER_STATE_OTHER;
        s = libvlc_media_player_get_state (self->priv->mp);
        if (s == libvlc_Playing) {
                state = VALCE_PLAYER_STATE_PLAYING;
        }
        if (s  == libvlc_Paused) {
                state = VALCE_PLAYER_STATE_PAUSED;
        }
        if (s == libvlc_Stopped) {
                state = VALCE_PLAYER_STATE_STOPPED;
        }
        return state;
}

static void valce_player_set_state (ValcePlayer* self, ValcePlayerState state) {

        g_return_if_fail (self != NULL);
	      if (state == VALCE_PLAYER_STATE_PLAYING)
		            libvlc_media_player_play (self->priv->mp);
	      if (state == VALCE_PLAYER_STATE_PAUSED)
		            libvlc_media_player_pause (self->priv->mp);
	      if (state == VALCE_PLAYER_STATE_STOPPED)
		            libvlc_media_player_stop (self->priv->mp);
	      if (state == VALCE_PLAYER_STATE_OTHER) {
                // FIXME: Do nothing
        }
        self->priv->state = state;
	      g_object_notify_by_pspec ((GObject *) self, valce_player_properties[VALCE_PLAYER_STATE_PROPERTY]);

}

static gboolean valce_player_get_muted (ValcePlayer* self) {

	      g_return_val_if_fail (self != NULL, FALSE);
	      gboolean result = (gboolean) libvlc_audio_get_mute (self->priv->mp);
	      self->priv->muted = result;
	      return result;
}

static void valce_player_set_muted (ValcePlayer* self, gboolean status) {

	      g_return_if_fail (self != NULL);
	      libvlc_audio_set_mute (self->priv->mp, (gint) status );
	      if (valce_player_get_muted (self) != status) {
          		self->priv->muted = status;
		          g_object_notify_by_pspec ((GObject *) self, valce_player_properties[VALCE_PLAYER_MUTE_PROPERTY]);
	      }
}

void valce_player_set_previous_chapter (ValcePlayer* self) {

        g_return_if_fail (self != NULL);
        libvlc_media_player_previous_chapter (self->priv->mp);
}

void valce_player_set_next_chapter (ValcePlayer* self) {

        g_return_if_fail (self != NULL);
        libvlc_media_player_next_chapter (self->priv->mp);
}

void valce_player_set_current_chapter_index (ValcePlayer* self, gint chapter) {

        g_return_if_fail (self != NULL);
        libvlc_media_player_set_chapter (self->priv->mp, chapter);
       	g_object_notify_by_pspec ((GObject *) self, valce_player_properties[VALCE_PLAYER_CURRENT_CHAPTER_INDEX_PROPERTY]);

}

gint valce_player_get_current_chapter_index (ValcePlayer* self) {

        g_return_val_if_fail (self != NULL, 0LL);
        self->priv->curent_chapter = libvlc_media_player_get_chapter (self->priv->mp);
        return self->priv->curent_chapter;
}

gint valce_player_get_chapter_count (ValcePlayer* self) {

        g_return_val_if_fail (self != NULL, 0LL);
        return libvlc_media_player_get_chapter_count (self->priv->mp);
}

GList* valce_player_get_chapter_list (ValcePlayer* self) {

	      g_return_val_if_fail (self != NULL, NULL);

        gint a = 0;
        gint i = 0;
        libvlc_chapter_description_t** pp = NULL;
        ValceChapter chapter = { 0, 0, g_strdup ("") };
       	chapter.offset = (gint64) 0;
	      chapter.duration = (gint64) 0;
	      _g_free0 (chapter.title);
	      chapter.title = g_strdup ("");
        GList* list = NULL;
        // https://trac.videolan.org/vlc/ticket/19381
        while (TRUE) {
                a = libvlc_media_player_get_full_chapter_descriptions (self->priv->mp,
                                                                       -1, &pp);
                if ( !(a > 0) )
                        return NULL;
		            if ( !(i < a) )
                        break;
                chapter.offset   = (gint64) (*pp[i]).i_time_offset;
                chapter.duration = (gint64) (*pp[i]).i_duration;
		            _g_free0 (chapter.title);
                chapter.title = g_strdup ( (gchar*) (*pp[i]).psz_name );
                libvlc_chapter_descriptions_release (pp, i);
                list = g_list_append (list, valce_chapter_dup (&chapter) );
                i++;
        }
        valce_chapter_destroy (&chapter);
	      return list;
}

gint64 valce_player_get_position (ValcePlayer* self) {

        gint64 result = 0LL;
	      g_return_val_if_fail (self != NULL, 0LL);
	      result = (gint64) libvlc_media_player_get_time (self->priv->mp);
	      return result;
}

gint64 valce_player_get_duration (ValcePlayer* self) {

        gint64 result = 0LL;
	      g_return_val_if_fail (self != NULL, 0LL);
	      result = (gint64) libvlc_media_player_get_length (self->priv->mp);
	      return result;
}

gdouble valce_player_get_volume (ValcePlayer* self) {

	      g_return_val_if_fail (self != NULL, 0.0);
        return (gdouble) ( (gdouble) ( libvlc_audio_get_volume (self->priv->mp) ) / 100 );
}

void valce_player_set_volume (ValcePlayer* self, gdouble volume) {

        g_return_if_fail (self != NULL);
        libvlc_audio_set_volume (self->priv->mp, (gint) (volume * 100) );
	      g_object_notify_by_pspec ((GObject *) self, valce_player_properties[VALCE_PLAYER_VOLUME_PROPERTY]);
}

static void valce_player_get_media_info (ValcePlayer* self, ValceMediaInfo* result) {

        g_return_if_fail (self != NULL);
        self->priv->media_info.title = libvlc_media_get_meta(self->priv->md, libvlc_meta_Title);
        self->priv->media_info.artist = libvlc_media_get_meta(self->priv->md, libvlc_meta_Artist);
        self->priv->media_info.artist = libvlc_media_get_meta(self->priv->md, libvlc_meta_Album);
        self->priv->media_info.album_artist = libvlc_media_get_meta(self->priv->md, libvlc_meta_AlbumArtist);
        self->priv->media_info.date = libvlc_media_get_meta(self->priv->md, libvlc_meta_Date);
        self->priv->media_info.genre = libvlc_media_get_meta(self->priv->md, libvlc_meta_Genre);
        self->priv->media_info.track_number = atoi( libvlc_media_get_meta(self->priv->md, libvlc_meta_TrackNumber) );
        self->priv->media_info.track_total = atoi( libvlc_media_get_meta(self->priv->md, libvlc_meta_TrackTotal) );
        self->priv->media_info.disk_number = atoi( libvlc_media_get_meta(self->priv->md, libvlc_meta_DiscNumber) );
        self->priv->media_info.disk_total = atoi( libvlc_media_get_meta(self->priv->md, libvlc_meta_DiscTotal) );
	      *result = self->priv->media_info;
	      return;
}

void valce_player_seek (ValcePlayer* self, gint64 position) {

        g_return_if_fail (self != NULL);
        libvlc_media_player_set_time (self->priv->mp, position);
}

void valce_player_set_xwindow (ValcePlayer* self, guint32 xid) {

        libvlc_media_player_set_xwindow (self->priv->mp, xid);
}

static void g_cclosure_user_marshal_VOID__INT64 (GClosure* closure, GValue* return_value,
                                                  guint n_param_values, const GValue* param_values,
                                                  gpointer invocation_hint, gpointer marshal_data) {

        typedef void (*GMarshalFunc_VOID__INT64) (gpointer data1, gint64 arg_1, gpointer data2);
	register GMarshalFunc_VOID__INT64 callback;
	register GCClosure* cc;
	register gpointer data1;
	register gpointer data2;
	cc = (GCClosure*) closure;
	g_return_if_fail (n_param_values == 2);
	if (G_CCLOSURE_SWAP_DATA (closure)) {
		data1 = closure->data;
		data2 = param_values->data[0].v_pointer;
	} else {
		data1 = param_values->data[0].v_pointer;
		data2 = closure->data;
	}
	callback = (GMarshalFunc_VOID__INT64) (marshal_data ? marshal_data : cc->callback);
	callback (data1, g_value_get_int64 (param_values + 1), data2);
}

static void g_cclosure_user_marshal_VOID__INT (GClosure* closure, GValue* return_value,
                                                  guint n_param_values, const GValue* param_values,
                                                  gpointer invocation_hint, gpointer marshal_data) {

  typedef void (*GMarshalFunc_VOID__INT) (gpointer data1, gint arg_1, gpointer data2);
	register GMarshalFunc_VOID__INT callback;
	register GCClosure* cc;
	register gpointer data1;
	register gpointer data2;
	cc = (GCClosure*) closure;
	g_return_if_fail (n_param_values == 2);
	if (G_CCLOSURE_SWAP_DATA (closure)) {
		data1 = closure->data;
		data2 = param_values->data[0].v_pointer;
	} else {
		data1 = param_values->data[0].v_pointer;
		data2 = closure->data;
	}
	callback = (GMarshalFunc_VOID__INT) (marshal_data ? marshal_data : cc->callback);
	callback (data1, g_value_get_int (param_values + 1), data2);
}


static void mp_length_changed_cb (const struct libvlc_event_t* event, void* self) {

        gint64 d = 0;
        g_return_if_fail ( (ValcePlayer*) self != NULL);
        if (event->type == libvlc_MediaPlayerLengthChanged) {
                d = (gint64) event->u.media_player_length_changed.new_length;
                if (d > 0)
                        ( (ValcePlayer*) self)->priv->dur = d;
	              g_signal_emit ( (ValcePlayer*) self, valce_player_signals[VALCE_PLAYER_DURATION_CHANGED_SIGNAL], 0, d);
        }
}

static void mp_time_changed_cb (const struct libvlc_event_t* event, void* self) {

        gint64 p = 0;
        g_return_if_fail ( (ValcePlayer*) self != NULL);
        if (event->type == libvlc_MediaPlayerTimeChanged) {
                p = (gint64) event->u.media_player_time_changed.new_time;
	              g_signal_emit ( (ValcePlayer*) self, valce_player_signals[VALCE_PLAYER_POSITION_UPDATED_SIGNAL], 0, p);
                if (p > 0 ) {
                        if (p == ( (ValcePlayer*) self)->priv->dur )
	                        g_signal_emit ( (ValcePlayer*) self, valce_player_signals[VALCE_PLAYER_END_OF_STREAM_SIGNAL], 0);
                }
        }
}

static void mp_chapter_changed_cb (const struct libvlc_event_t* event, void* self) {

        gint c = 0;
        g_return_if_fail ( (ValcePlayer*) self != NULL);
        if (event->type == libvlc_MediaPlayerChapterChanged) {
                c = (gint) event->u.media_player_chapter_changed.new_chapter;
	              g_signal_emit ( (ValcePlayer*) self, valce_player_signals[VALCE_PLAYER_CHAPTER_CHANGED_SIGNAL], 0, c);
        }
}

static void mp_meta_changed_cb (const struct libvlc_event_t* event, void* self) {

        g_return_if_fail ( (ValcePlayer*) self != NULL);
        if (event->type == libvlc_MediaMetaChanged) {
	              g_signal_emit ( (ValcePlayer*) self, valce_player_signals[VALCE_PLAYER_MEDIA_INFO_UPDATED_SIGNAL], 0);
        }
}

static void mp_volume_changed_cb (const struct libvlc_event_t* event, void* self) {

        g_return_if_fail ( (ValcePlayer*) self != NULL);
        if (event->type == libvlc_MediaPlayerAudioVolume) {
        	      g_signal_emit ( (ValcePlayer*) self, valce_player_signals[VALCE_PLAYER_VOLUME_CHANGED_SIGNAL], 0);
        }
}


static void mp_muted_cb (const struct libvlc_event_t* event, void* self) {

        g_return_if_fail ( (ValcePlayer*) self != NULL);
        if (event->type == libvlc_MediaPlayerMuted) {
        	       g_signal_emit ( (ValcePlayer*) self, valce_player_signals[VALCE_PLAYER_MUTED_SIGNAL], 0);
        }

}

static void mp_unmuted_cb (const struct libvlc_event_t* event, void* self) {

        g_return_if_fail ( (ValcePlayer*) self != NULL);
        if (event->type == libvlc_MediaPlayerUnmuted) {
        	g_signal_emit ( (ValcePlayer*) self, valce_player_signals[VALCE_PLAYER_UNMUTED_SIGNAL], 0);
        }

}

static void mp_media_changed_cb (const struct libvlc_event_t* event, void* self) {

        gchar* mrl = NULL;
        g_return_if_fail ( (ValcePlayer*) self != NULL);
        if (event->type == libvlc_MediaPlayerMediaChanged) {
                mrl = (char*) libvlc_media_get_mrl( (libvlc_media_t*) event->u.media_player_media_changed.new_media );
        	      g_signal_emit ( (ValcePlayer*) self, valce_player_signals[VALCE_PLAYER_URI_LOADED_SIGNAL], 0, mrl);
        }
}

static void mp_state_changed_cb (const struct libvlc_event_t* event, void* self) {

        ValcePlayerState state = VALCE_PLAYER_STATE_OTHER;
        g_return_if_fail ( (ValcePlayer*) self != NULL);
        if (event->type == libvlc_MediaStateChanged) {
                libvlc_state_t s = (libvlc_state_t) event->u.media_state_changed.new_state;
                if (s == libvlc_Playing) {
		        state = VALCE_PLAYER_STATE_PLAYING;
	        }
	        if (s  == libvlc_Paused) {
		        state = VALCE_PLAYER_STATE_PAUSED;
	        }
	        if (s == libvlc_Stopped) {
		        state = VALCE_PLAYER_STATE_STOPPED;
	        }
	        g_signal_emit ( (ValcePlayer*) self, valce_player_signals[VALCE_PLAYER_STATE_CHANGED_SIGNAL], 0, state);
        }
}

static libvlc_instance_t* vlc_instance_new (void) {

        gchar	**vlc_argv;
        gint vlc_argc = 1;

        vlc_argv = g_malloc_n (2, sizeof(*vlc_argv) );
        vlc_argv[vlc_argc++] = g_strdup( g_get_prgname () );
        vlc_argv[vlc_argc++] = g_strdup("--no-xlib");
	      vlc_argv[vlc_argc] = NULL;

        return (libvlc_instance_t*) libvlc_new ( (gint) vlc_argc, (const char* const*) vlc_argv);
}


static GObject* valce_player_constructor (GType type, guint n_construct_properties, GObjectConstructParam* construct_properties) {
	
        GObject* obj;
	      GObjectClass* parent_class;
	      ValcePlayer * self;
      	parent_class = G_OBJECT_CLASS (valce_player_parent_class);
	      obj = parent_class->constructor (type, n_construct_properties, construct_properties);
	      self = G_TYPE_CHECK_INSTANCE_CAST (obj, VALCE_TYPE_PLAYER, ValcePlayer);
        self->priv->inst = vlc_instance_new ();
      	self->priv->mp = libvlc_media_player_new (self->priv->inst);
        self->priv->ev = libvlc_media_player_event_manager (self->priv->mp);
        self->priv->dur = 0;

        libvlc_event_attach (self->priv->ev, libvlc_MediaPlayerTimeChanged,    mp_time_changed_cb,    self);
        libvlc_event_attach (self->priv->ev, libvlc_MediaPlayerChapterChanged, mp_chapter_changed_cb, self);
        libvlc_event_attach (self->priv->ev, libvlc_MediaMetaChanged,          mp_meta_changed_cb,    self);
	      libvlc_event_attach (self->priv->ev, libvlc_MediaPlayerLengthChanged,  mp_length_changed_cb,  self);
        libvlc_event_attach (self->priv->ev, libvlc_MediaPlayerAudioVolume,    mp_volume_changed_cb,  self);
        libvlc_event_attach (self->priv->ev, libvlc_MediaPlayerMuted,          mp_muted_cb,           self);
        libvlc_event_attach (self->priv->ev, libvlc_MediaPlayerUnmuted,        mp_unmuted_cb,         self);
	      libvlc_event_attach (self->priv->ev, libvlc_MediaPlayerMediaChanged,   mp_media_changed_cb,   self);
        libvlc_event_attach (self->priv->ev, libvlc_MediaStateChanged,         mp_state_changed_cb,   self);

        return obj;
}


static void valce_player_class_init (ValcePlayerClass * klass) {

        valce_player_parent_class = g_type_class_peek_parent (klass);
	      g_type_class_add_private (klass, sizeof (ValcePlayerPrivate));
	      G_OBJECT_CLASS (klass)->constructor = valce_player_constructor;
	      G_OBJECT_CLASS (klass)->get_property = _vala_valce_player_get_property;
	      G_OBJECT_CLASS (klass)->set_property = _vala_valce_player_set_property;
	      G_OBJECT_CLASS (klass)->finalize = valce_player_finalize;
        g_object_class_install_property (G_OBJECT_CLASS (klass),
                                         VALCE_PLAYER_VOLUME_PROPERTY,
                                         valce_player_properties[VALCE_PLAYER_VOLUME_PROPERTY] =
                                         g_param_spec_double ("volume", "volume", "volume",
                                                              -G_MAXDOUBLE, G_MAXDOUBLE, 0.0,
                                                              G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK |
                                                              G_PARAM_STATIC_BLURB | G_PARAM_READABLE |
                                                              G_PARAM_WRITABLE));
        g_object_class_install_property (G_OBJECT_CLASS (klass),
                                         VALCE_PLAYER_CURRENT_CHAPTER_INDEX_PROPERTY,
                                         valce_player_properties[VALCE_PLAYER_CURRENT_CHAPTER_INDEX_PROPERTY] =
                                         g_param_spec_int ("current-chapter-index", "current-chapter-index", "current-chapter-index",
                                                           G_MININT, G_MAXINT, 0,
                                                           G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK |
                                                           G_PARAM_STATIC_BLURB | G_PARAM_READABLE |
                                                           G_PARAM_WRITABLE));

        g_object_class_install_property (G_OBJECT_CLASS (klass),
                                         VALCE_PLAYER_MEDIA_INFO_PROPERTY,
                                         valce_player_properties[VALCE_PLAYER_MEDIA_INFO_PROPERTY] =
                                         g_param_spec_boxed ("media-info", "media-info", "media-info",
                                                             VALCE_TYPE_MEDIA_INFO,
                                                             G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK |
                                                             G_PARAM_STATIC_BLURB | G_PARAM_READABLE));

	      g_object_class_install_property (G_OBJECT_CLASS (klass),
                                         VALCE_PLAYER_STATE_PROPERTY,
                                         valce_player_properties[VALCE_PLAYER_STATE_PROPERTY] =
                                         g_param_spec_enum ("state", "state", "state",
                                                            VALCE_TYPE_PLAYER_STATE, 0,
                                                            G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK |
                                                            G_PARAM_STATIC_BLURB | G_PARAM_READABLE |
                                                            G_PARAM_WRITABLE));

        g_object_class_install_property (G_OBJECT_CLASS (klass),
                                         VALCE_PLAYER_MUTE_PROPERTY,
                                         valce_player_properties[VALCE_PLAYER_MUTE_PROPERTY] =
                                         g_param_spec_boolean ("mute", "mute", "mute",
                                                               FALSE, G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK |
                                                               G_PARAM_STATIC_BLURB | G_PARAM_READABLE |
                                                               G_PARAM_WRITABLE));

        valce_player_signals[VALCE_PLAYER_END_OF_STREAM_SIGNAL] =
                g_signal_new ("end-of-stream", VALCE_TYPE_PLAYER, G_SIGNAL_RUN_LAST, 0, NULL, NULL,
                              g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);

        valce_player_signals[VALCE_PLAYER_DURATION_CHANGED_SIGNAL] =
                g_signal_new ("duration-changed", VALCE_TYPE_PLAYER, G_SIGNAL_RUN_LAST, 0, NULL, NULL,
                              g_cclosure_user_marshal_VOID__INT64, G_TYPE_NONE, 1, G_TYPE_INT64);

        valce_player_signals[VALCE_PLAYER_POSITION_UPDATED_SIGNAL] =
                g_signal_new ("position-updated", VALCE_TYPE_PLAYER, G_SIGNAL_RUN_LAST, 0, NULL, NULL,
                              g_cclosure_user_marshal_VOID__INT64, G_TYPE_NONE, 1, G_TYPE_INT64);

        valce_player_signals[VALCE_PLAYER_VOLUME_CHANGED_SIGNAL] =
                g_signal_new ("volume-changed", VALCE_TYPE_PLAYER, G_SIGNAL_RUN_LAST, 0, NULL, NULL,
                              g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);

        valce_player_signals[VALCE_PLAYER_MUTED_SIGNAL] =
                g_signal_new ("muted", VALCE_TYPE_PLAYER, G_SIGNAL_RUN_LAST, 0, NULL, NULL,
                              g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);

        valce_player_signals[VALCE_PLAYER_UNMUTED_SIGNAL] =
                g_signal_new ("unmuted", VALCE_TYPE_PLAYER, G_SIGNAL_RUN_LAST, 0, NULL, NULL,
                              g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);

        valce_player_signals[VALCE_PLAYER_URI_LOADED_SIGNAL] =
                g_signal_new ("uri-loaded", VALCE_TYPE_PLAYER, G_SIGNAL_RUN_LAST, 0, NULL, NULL,
                              g_cclosure_marshal_VOID__STRING, G_TYPE_NONE, 1, G_TYPE_STRING);

        valce_player_signals[VALCE_PLAYER_STATE_CHANGED_SIGNAL] =
                g_signal_new ("state-changed", VALCE_TYPE_PLAYER, G_SIGNAL_RUN_LAST, 0, NULL, NULL,
                              g_cclosure_marshal_VOID__ENUM, G_TYPE_NONE, 1, VALCE_TYPE_PLAYER_STATE);

        valce_player_signals[VALCE_PLAYER_MEDIA_INFO_UPDATED_SIGNAL] =
                g_signal_new ("media-info-updated", VALCE_TYPE_PLAYER, G_SIGNAL_RUN_LAST, 0, NULL, NULL,
                              g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);

        valce_player_signals[VALCE_PLAYER_CHAPTER_CHANGED_SIGNAL] =
                g_signal_new ("chapter-changed", VALCE_TYPE_PLAYER, G_SIGNAL_RUN_LAST, 0, NULL, NULL,
                              g_cclosure_user_marshal_VOID__INT, G_TYPE_NONE, 1, G_TYPE_INT);

}


static void valce_player_instance_init (ValcePlayer * self) {

        self->priv = VALCE_PLAYER_GET_PRIVATE (self);
}


static void valce_player_finalize (GObject * obj) {

        ValcePlayer * self;
	      self = G_TYPE_CHECK_INSTANCE_CAST (obj, VALCE_TYPE_PLAYER, ValcePlayer);
        valce_media_info_destroy (&self->priv->media_info);
        libvlc_media_player_release (self->priv->mp);
        libvlc_release (self->priv->inst);

	      G_OBJECT_CLASS (valce_player_parent_class)->finalize (obj);
}


GType valce_player_get_type (void) {

  static volatile gsize valce_player_type_id__volatile = 0;
	if (g_once_init_enter (&valce_player_type_id__volatile)) {
		static const GTypeInfo g_define_type_info = { sizeof (ValcePlayerClass),
                                                              (GBaseInitFunc) NULL,
                                                              (GBaseFinalizeFunc) NULL,
                                                              (GClassInitFunc) valce_player_class_init,
                                                              (GClassFinalizeFunc) NULL, NULL,
                                                              sizeof (ValcePlayer), 0,
                                                              (GInstanceInitFunc) valce_player_instance_init,
                                                              NULL };
		GType valce_player_type_id;
		valce_player_type_id = g_type_register_static (G_TYPE_OBJECT, "ValcePlayer", &g_define_type_info, 0);
		g_once_init_leave (&valce_player_type_id__volatile, valce_player_type_id);
	}
	return valce_player_type_id__volatile;
}

static void _vala_valce_player_get_property (GObject * object, guint property_id, GValue * value, GParamSpec * pspec) {
	ValcePlayer * self;
	self = G_TYPE_CHECK_INSTANCE_CAST (object, VALCE_TYPE_PLAYER, ValcePlayer);
	switch (property_id) {
		case VALCE_PLAYER_VOLUME_PROPERTY:
		        g_value_set_double (value, valce_player_get_volume (self));
		        break;
		case VALCE_PLAYER_CURRENT_CHAPTER_INDEX_PROPERTY:
		        g_value_set_int (value, valce_player_get_current_chapter_index (self));
		        break;
		case VALCE_PLAYER_MEDIA_INFO_PROPERTY:
		        {
			ValceMediaInfo boxed;
			valce_player_get_media_info (self, &boxed);
			g_value_set_boxed (value, &boxed);
		        }
		        break;
		case VALCE_PLAYER_STATE_PROPERTY:
		        g_value_set_enum (value, valce_player_get_state (self));
		        break;
		case VALCE_PLAYER_MUTE_PROPERTY:
		        g_value_set_boolean (value, valce_player_get_muted (self));
		        break;
		default:
		        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
		        break;
	}
}


static void _vala_valce_player_set_property (GObject * object, guint property_id, const GValue * value, GParamSpec * pspec) {
	ValcePlayer * self;
	self = G_TYPE_CHECK_INSTANCE_CAST (object, VALCE_TYPE_PLAYER, ValcePlayer);
	switch (property_id) {
		case VALCE_PLAYER_VOLUME_PROPERTY:
		        valce_player_set_volume (self, g_value_get_double (value));
		        break;
		case VALCE_PLAYER_CURRENT_CHAPTER_INDEX_PROPERTY:
		        valce_player_set_current_chapter_index (self, g_value_get_int (value));
		        break;
		case VALCE_PLAYER_STATE_PROPERTY:
		        valce_player_set_state (self, g_value_get_enum (value));
		        break;
		case VALCE_PLAYER_MUTE_PROPERTY:
		        valce_player_set_muted (self, g_value_get_boolean (value));
		        break;
		default:
		        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
		        break;
	}
}

