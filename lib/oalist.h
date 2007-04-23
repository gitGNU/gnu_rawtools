/*** oalist.h --- ?  -*- C -*- */

/*** Copyright (C) 2006 Ivan Shmakov */

/** This program is free software; you can redistribute it and/or modify
 ** it under the terms of the GNU General Public License as published by
 ** the Free Software Foundation; either version 2 of the License, or
 ** (at your option) any later version.
 **
 ** This program is distributed in the hope that it will be useful, but
 ** WITHOUT ANY WARRANTY; without even the implied warranty of
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 ** General Public License for more details.
 **
 ** You should have received a copy of the GNU General Public License
 ** along with this program; if not, write to the Free Software
 ** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 ** 02110-1301 USA
 */

/*** Code: */
#ifndef OALIST_H
#define OALIST_H

struct oalist;

/** allocation and deallocation */
extern struct oalist *oalist_alloc  (size_t size,
				     comparison_fn_t compare);
extern struct oalist *oalist_alloc3 (size_t size,
				     comparison_fn_t compare,
				     size_t allocate);
extern void oalist_free (struct oalist *list);

/** clearing and filling the list */
extern void oalist_clear  (struct oalist *list);
extern int  oalist_append (struct oalist *list, const void *ents,
			   size_t count);

/** searching */
extern const void *oalist_search (struct oalist *list,
				  const void *value);

#endif
/*** Emacs stuff */
/** Local variables: */
/** fill-column: 72 */
/** indent-tabs-mode: nil */
/** ispell-local-dictionary: "british" */
/** mode: outline-minor */
/** outline-regexp: "/[*][*][*]" */
/** End: */
/** LocalWords:   */
/*** oalist.h ends here */
