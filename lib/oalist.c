/*** oalist.c --- Simple Ordered Association Lists  -*- C -*- */

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
#define _GNU_SOURCE
#include <stdlib.h>
#include <string.h>             /* for memcpy */

#include "oalist.h"
#include "usemacro.h"           /* for MALLOC_VAR () */

/* NB: for `BYTE *' */
#define BYTE char

/* the structure */
struct oalist {
  /* the number of the elements and the size of the allocated space */
  size_t count, alloc;
  /* the size of the element */
  size_t size;
  /* the comparison function */
  comparison_fn_t cmp;
  /* is the data ordered? */
  int ordered_p;
  /* the data */
  void *data;
};

struct oalist *
oalist_alloc (size_t size, comparison_fn_t compare)
{
  /* . */
  return
    oalist_alloc3 (size, compare, 0);
}

struct oalist *
oalist_alloc3 (size_t size, comparison_fn_t compare, size_t allocate)
{
  struct oalist *l;

  /* allocate the structure */
  if (MALLOC_VAR (l) == 0) {
    /* . */
    return 0;
  }

  /* allocate entries if requested */
  if (allocate == 0) {
    l->data = 0;
  } else if ((l->data = malloc (size * allocate)) == 0) {
    free (l);
    /* . */
    return 0;
  }

  /* initialize other fields */
  l->size  = 0;
  l->alloc = allocate;

  /* initial (empty) list is, indeed, ordered */
  l->ordered_p = 1;

  /* . */
  return l;
}

void
oalist_free (struct oalist *list)
{
  if (list->data != 0) free (list->data);
  free (list);
}

static void
oalist_ensure_ordered (struct oalist *list)
{
  if (list->ordered_p) {
    /* . */
    return;
  }
  qsort (list->data, list->count, list->size, list->cmp);
  list->ordered_p = 1;
}

static int
oalist_realloc (struct oalist *l, size_t s)
{
  struct oalist *new;

  /* NB: zero new size request is ignored */
  if (s == 0 || s == l->alloc)
    return 0;                   /* . */
  if ((new = realloc (l->data, s * l->size)) == 0)
    return -1;                  /* . */
  l->data  = new;
  l->alloc = s;

  /* NB: losing elements */
  if (l->count > l->alloc)
    l->count = l->alloc;

  /* . */
  return 0;
}

static int
oalist_grow_by (struct oalist *l, size_t by)
{
  /* FIXME: magic number */
  const size_t min_grow = 5;
  const size_t new = l->size + by;
  /* . */
  return
    (new > l->alloc)
    && oalist_realloc (l, MAX (new, l->alloc + min_grow));
}

void
oalist_clear (struct oalist *list)
{
  list->size = 0;
}

int
oalist_append (struct oalist *list,
               const void *ents,
               size_t count)
{
  comparison_fn_t cmp = list->cmp;
  const size_t size = list->size;
  size_t rest;
  BYTE *dst;
  const BYTE *src;
  const void *prev;

  if (oalist_grow_by (list, count) != 0) {
    /* . */
    return -1;
  }

  /* copy the data */
  for (rest = count,
         dst  = (char *)list->data + list->count * size,
         prev = (list->count > 0 ? dst - size : 0),
         src  = ents;
       rest > 0;
       rest--, dst++, prev = src++) {
    if (prev != 0 && cmp (prev, src) < 0) {
      list->ordered_p = 0;
    }
    memcpy (dst, src, size);
    list->count++;
  }

  /* . */
  return 0;
}

const void *
oalist_search (struct oalist *list, const void *value)
{
  comparison_fn_t cmp = list->cmp;
  const size_t size = list->size;
  const BYTE *l, *r;

  if (list->count == 0)
    return 0;                   /* . */

  oalist_ensure_ordered (list);

  r = (l = list->data) + list->count - 1;
  /* check if `value' is within the range */
  if (cmp (value, l) < 0)
    return 0;                   /* . */
  if (cmp (value, r) >= 0)
    return r;                   /* . */
  while ((r - l) > size) {
    const void *c = l + ((r - l) >> 1);
    if (cmp (value, c) > 0) {
      l = c;
    } else {
      r = c;
    }
  }

  /* . */
  return l;
}

/*** Emacs stuff */
/** Local variables: */
/** fill-column: 72 */
/** indent-tabs-mode: nil */
/** ispell-local-dictionary: "british" */
/** mode: outline-minor */
/** outline-regexp: "/[*][*][*]" */
/** End: */
/** LocalWords:   */
/*** oalist.c ends here */
