/* playlist_basic.h
 * - Simple unscripted playlist
 *
 * $Id: playlist_basic.h,v 1.3.2.1 2002/02/07 09:11:12 msmith Exp $
 *
 * Copyright (c) 2001-2002 Michael Smith <msmith@labyrinth.net.au>
 *
 * This program is distributed under the terms of the GNU General
 * Public License, version 2. You may use, modify, and redistribute
 * it under the terms of this license. A copy should be included
 * with this source.
 */

#ifndef __PLAYLIST_BASIC_H__
#define __PLAYLIST_BASIC_H__

typedef struct
{
	char **pl;
	int len;
	int pos;
	char *file; /* Playlist file */
	time_t mtime;
	int random;
	int once;
    int restartafterreread;

} basic_playlist;

void playlist_basic_clear(void *data);
char *playlist_basic_get_next_filename(void *data);
int playlist_basic_initialise(module_param_t *params, playlist_state_t *pl);


#endif  /* __PLAYLIST_BASIC_H__ */

