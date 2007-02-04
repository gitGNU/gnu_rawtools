/*** oalist.h --- ?  -*- C -*- */
/** $Id: oalist.h,v 1.1 2007/02/04 15:03:56 ivan Exp $ */

/*** Copyright (C) 2006 Ivan Shmakov */

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
/** ispell-local-dictionary: "english" */
/** mode: outline-minor */
/** outline-regexp: "[/]\\*\\*\\*" */
/** End: */
/** LocalWords:   */
/*** oalist.h ends here */
