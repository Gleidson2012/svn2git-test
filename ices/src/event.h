/* event.h
 * - Generic interface for passing events to modules.
 *
 * $Id: event.h,v 1.2.2.1 2002/02/07 09:11:11 msmith Exp $
 *
 * Copyright (c) 2001-2002 Michael Smith <msmith@labyrinth.net.au>
 *
 * This program is distributed under the terms of the GNU General
 * Public License, version 2. You may use, modify, and redistribute
 * it under the terms of this license. A copy should be included
 * with this source.
 */

#ifndef __EVENT_H__
#define __EVENT_H__

typedef enum {
	EVENT_SHUTDOWN, /* Full/final shutdown. MUST NOT ignore */
	EVENT_PAUSE, /* temporary shutdown. Can be ignored */
	EVENT_NEXTTRACK, /* Start a new track in some way */
	EVENT_RECONF, /* Reconfigure self, if possible */
	EVENT_METADATAUPDATE, /* Incoming new metadata */
} event_type;

#endif /* __EVENT_H__ */

