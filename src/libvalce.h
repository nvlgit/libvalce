/* libvalce.h *
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

#pragma once

#include <glib.h>
#include <glib-object.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <math.h>
#include <vlc/vlc.h>

G_BEGIN_DECLS

#define LIBVALCE_INSIDE
# include "libvalce-version.h"
#undef LIBVALCE_INSIDE

#define VALCE_TYPE_CHAPTER (valce_chapter_get_type ())
typedef struct _ValceChapter ValceChapter;

#define VALCE_TYPE_MEDIA_INFO (valce_media_info_get_type ())
typedef struct _ValceMediaInfo ValceMediaInfo;

#define VALCE_TYPE_PLAYER_STATE (valce_player_state_get_type ())

#define VALCE_TYPE_PLAYER (valce_player_get_type ())
#define VALCE_PLAYER(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), VALCE_TYPE_PLAYER, ValcePlayer))
#define VALCE_PLAYER_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), VALCE_TYPE_PLAYER, ValcePlayerClass))
#define VALCE_IS_PLAYER(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), VALCE_TYPE_PLAYER))
#define VALCE_IS_PLAYER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), VALCE_TYPE_PLAYER))
#define VALCE_PLAYER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), VALCE_TYPE_PLAYER, ValcePlayerClass))

typedef struct _ValcePlayer ValcePlayer;
typedef struct _ValcePlayerClass ValcePlayerClass;
typedef struct _ValcePlayerPrivate ValcePlayerPrivate;

typedef enum  {

        VALCE_PLAYER_STATE_STOPPED,
	VALCE_PLAYER_STATE_PLAYING,
	VALCE_PLAYER_STATE_PAUSED,
	VALCE_PLAYER_STATE_OTHER

} ValcePlayerState;

enum  {
	VALCE_PLAYER_END_OF_STREAM_SIGNAL,
	VALCE_PLAYER_DURATION_CHANGED_SIGNAL,
	VALCE_PLAYER_POSITION_UPDATED_SIGNAL,
	VALCE_PLAYER_VOLUME_CHANGED_SIGNAL,
	VALCE_PLAYER_MUTED_SIGNAL,
	VALCE_PLAYER_UNMUTED_SIGNAL,
	VALCE_PLAYER_URI_LOADED_SIGNAL,
	VALCE_PLAYER_STATE_CHANGED_SIGNAL,
	VALCE_PLAYER_MEDIA_INFO_UPDATED_SIGNAL,
	VALCE_PLAYER_CHAPTER_CHANGED_SIGNAL,
	VALCE_PLAYER_NUM_SIGNALS
};

struct _ValceChapter {

        gint64 offset;
	      gint64 duration;
	      gchar* title;
};

struct _ValceMediaInfo {

        gchar* title;
        gchar* artist;
        gchar* album;
        gchar* album_artist;
        gchar* genre;
        gint track_number;
        gint track_total;
        gint disk_number;
        gint disk_total;
        gchar* date;
};

struct _ValcePlayer {

        GObject parent_instance;
	      ValcePlayerPrivate * priv;
};

struct _ValcePlayerClass {

        GObjectClass parent_class;
};

GType          valce_chapter_get_type (void) G_GNUC_CONST;
void           valce_chapter_free     (ValceChapter* self);
void           valce_chapter_copy     (const ValceChapter* self, ValceChapter* dest);

GType           valce_media_info_get_type (void) G_GNUC_CONST;
void            valce_media_info_free (ValceMediaInfo* self);
void            valce_media_info_copy (const ValceMediaInfo* self, ValceMediaInfo* dest);

GType          valce_player_state_get_type       (void) G_GNUC_CONST;
GType          valce_player_get_type             (void) G_GNUC_CONST;
/**
 * valce_player_new:
 * Returns: (not nullable) (type Valce.Player) (transfer full): create new ValcePlayer
 */
ValcePlayer*   valce_player_new (void);

void           valce_player_set_xwindow          (ValcePlayer* self, guint32 xid);

gboolean       valce_player_set_uri (ValcePlayer* self, const gchar* uri);
/**
 * valce_player_set_volume:
 * value  0.0 ... 1.0
 */
void           valce_player_set_volume (ValcePlayer* self, gdouble volume);
gdouble        valce_player_get_volume (ValcePlayer* self);

void           valce_player_set_previous_chapter       (ValcePlayer* self);
void           valce_player_set_next_chapter           (ValcePlayer* self);
void           valce_player_set_current_chapter_index  (ValcePlayer* self, gint chapter);
gint           valce_player_get_current_chapter_index  (ValcePlayer* self);
gint           valce_player_get_chapter_count          (ValcePlayer* self);
/**
 * incorrect durations https://trac.videolan.org/vlc/ticket/19381
 *
 * valce_player_get_chapter_list:
 * Returns: (nullable) (element-type Valce.Chapter) (transfer full): GList of #ValceChapter
 */
GList*         valce_player_get_chapter_list    (ValcePlayer* self);

void           valce_player_play    (ValcePlayer* self);
void           valce_player_pause   (ValcePlayer* self);
void           valce_player_stop    (ValcePlayer* self);

gint64         valce_player_get_position (ValcePlayer* self);
gint64         valce_player_get_duration (ValcePlayer* self);
void           valce_player_seek         (ValcePlayer* self, gint64 position);


G_END_DECLS
