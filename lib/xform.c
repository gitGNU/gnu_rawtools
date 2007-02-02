/*** xform.c --- Perform some simple data transformations  -*- C -*- */
/** $Id: xform.c,v 1.2 2006/07/16 17:12:12 ivan Exp $ */

/*** Copyright (C) 2006 Ivan Shmakov */

/*** Code: */
#define _GNU_SOURCE
#include <assert.h>
#include <errno.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>             /* for memcpy */

#include "xform.h"
#include "usemacro.h"           /* for MALLOC_VAR () and COPY_VAR () */

/* the structure */
struct xform_table {
  /* the size of the table and the size of the allocated space */
  size_t size, alloc;
  /* table entries */
  struct xform_table_entry *ents;
#ifdef NAN
  /* not-a-number (NaN) maps to... */
  double nan_value;
#endif
  /* out-of-range (less than the least bound) maps to... */
  double range_value;
  /* pre-computed linear interpolation coefficients, or 0 */
  double *l_coefs;
  /* was this table sorted after the last append? */
  int sorted_p;
};

struct xform_table *
xform_table_alloc (size_t allocate)
{
  struct xform_table *t;

  /* allocate the structure */
  if (MALLOC_VAR (t) == 0) {
    /* . */
    return 0;
  }

  /* allocate entries if requested */
  if (allocate == 0) {
    t->ents = 0;
  } else if (MALLOC_ARY (t->ents, allocate) == 0) {
    free (t);
    /* . */
    return 0;
  }

  /* initialize other fields */
  t->size  = 0;
  t->alloc = allocate;
#ifdef NAN
  /* map NaN and out-of-range to NaN by default */
  t->nan_value   = NAN;
  t->range_value = NAN;
#else
  t->range_value = - INFINITY;
#endif
  t->l_coefs = 0;
  /* initial (empty) table is, indeed, sorted */
  t->sorted_p = 1;

  /* . */
  return t;
}

void
xform_table_free (struct xform_table *table)
{
  if (table->ents != 0) free (table->ents);
  if (table->l_coefs != 0) free (table->l_coefs);
  free (table);
}

#ifdef NAN
double
xform_table_nan (const struct xform_table *table)
{
  return table->nan_value;
}

void
xform_table_nan_set (struct xform_table *table, double value)
{
  table->nan_value = value;
}
#endif

double
xform_table_oor (const struct xform_table *table)
{
  return table->range_value;
}

void
xform_table_oor_set (struct xform_table *table, double value)
{
  table->range_value = value;
}

static int
xform_table_ents_cmp (const void *a, const void *b)
{
  const double
    da = ((struct xform_table_entry *)a)->bound,
    db = ((struct xform_table_entry *)b)->bound;

  /* . */
  return (da > db) - (da < db);
}

static void
xform_table_ensure_ordered (struct xform_table *table)
{
  qsort (table->ents, table->size,
         sizeof (*(table->ents)), xform_table_ents_cmp);
}

void
xform_table_no_interp (struct xform_table *table)
{
  if (table->l_coefs != 0) {
    free (table->l_coefs);
    table->l_coefs = 0;
  }
}

int
xform_table_linear_interp (struct xform_table *table)
{
  size_t rest;
  double *dst;
  const struct xform_table_entry *src, *prev;

  /* NB: return if the coefficients are already computed */
  if (table->l_coefs != 0)
    return 0;                   /* . */
  /* NB: need at least two points to compute the coefficients */
  if (table->size < 2) {
    errno = EINVAL;
    return -1;
  }
  if (MALLOC_ARY (table->l_coefs, table->size - 1) == 0)
    return -1;                  /* . */

  /* sort the table if needed */
  xform_table_ensure_ordered (table);

  /* iterate over the boundaries */
  for (rest = table->size - 1,
         dst = table->l_coefs, src = (prev = table->ents) + 1;
       rest > 0;
       rest--, dst++, prev = (src++)) {
    *dst = ((isfinite (src->bound) && isfinite (prev->bound))
            ? (src->value - prev->value) / (src->bound - prev->bound)
            : (double)0);
  }

  /* . */
  return 0;
}

static int
xform_table_realloc (struct xform_table *t, size_t s)
{
  struct xform_table_entry *new;

  /* NB: zero new size request is ignored */
  if (s == 0 || s == t->alloc)
    return 0;                   /* . */
  if ((new = realloc (t->ents, s * sizeof (*(t->ents)))) == 0)
    return -1;                  /* . */
  t->ents  = new;
  t->alloc = s;

  /* NB: disabling interpolation */
  xform_table_no_interp (t);

  /* . */
  return 0;
}

static int
xform_table_grow_by (struct xform_table *t, size_t by)
{
  /* FIXME: magic number */
  const size_t min_grow = 5;
  const size_t new = t->size + by;
  /* . */
  return
    (new > t->alloc)
    && xform_table_realloc (t, MAX (new, t->alloc + min_grow));
}

void
xform_table_clear (struct xform_table *table)
{
  table->size = 0;
}

int
xform_table_append (struct xform_table *table,
                    const struct xform_table_entry *ents,
                    size_t size)
{
  size_t rest;
  int empty_p;
  struct xform_table_entry *dst;
  const struct xform_table_entry *src;
  double last;

  /* allocate more entries if needed */
  /* NB: might spend less entries, if there'll be some NaN's */
  if (xform_table_grow_by (table, size) != 0) {
    /* . */
    return -1;
  }

  /* copy the data */
  /* NB: `last's initial value is ignored if `empty_p' */
  for (rest = size, empty_p = (table->size == 0),
         dst = table->ents + table->size,
         src = ents, last = (empty_p ? 0 : (dst - 1)->bound);
       rest > 0;
       rest--) {
    const double b = src->bound;
#ifdef NAN
    if (isnan (b)) {
      table->nan_value = src->value;
      src++;
      continue;
    }
#endif
    if (empty_p) {
      empty_p = 1;
    } else if (b < last) {
      table->sorted_p = 0;
    }
    last = b;
    COPY_VAR (dst, src);
    table->size++;
    dst++, src++;
  }

  /* . */
  return 0;
}

static const struct xform_table_entry *
xform_table_search (const struct xform_table *t,
                    double value)
{
  const struct xform_table_entry *l, *r;
  assert (! isnan (value));

  if (t->size == 0)
    return 0;                   /* . */

  l = t->ents;
  r = t->ents + t->size - 1;
  /* check if `value' is within the range */
  if (value < l->bound)
    return 0;                   /* . */
  if (value >= r->bound)
    return r;                   /* . */
  while ((r - l) > 1) {
    const struct xform_table_entry *c
      = l + ((r - l) >> 1);
    if (value > c->bound) {
      l = c;
    } else {
      r = c;
    }
  }

  /* . */
  return l;
}

void
xform_table_apply (const struct xform_table *table,
                   double *to, const double *from,
                   size_t size)
{
  size_t rest;
  double *dst;
  const double *src;
  const double
    *l_c    = table->l_coefs,
    range_v = table->range_value,
    nan_v   = table->nan_value;

  for (rest = size, dst = to, src = from;
       rest > 0;
       rest--, dst++, src++) {
    double v = *src;
#ifdef NAN
    if (isnan (v)) {
      /* the value is NaN */
      *dst = nan_v;
      continue;
    }
#endif
    {
      const struct xform_table_entry *left
        = xform_table_search (table, v);
      double m;
      if (left == 0) {
        /* the value is out of range */
        *dst = range_v;
        continue;
      }
      *dst = (left->value
              + ((l_c != 0
                  && (m = l_c[left - table->ents]) != (double)0)
                 ? m * (v - left->bound)
                 : (double)0));
    }
  }
}

/*** Emacs stuff */
/** Local variables: */
/** fill-column: 72 */
/** indent-tabs-mode: nil */
/** ispell-local-dictionary: "english" */
/** mode: outline-minor */
/** outline-regexp: "[/]\\*\\*\\*" */
/** End: */
/** LocalWords:   */
/*** xform.c ends here */
