/*** xform.h --- Perform some simple data transformations  -*- C -*- */

/*** Copyright (C) 2006 Ivan Shmakov */

/*** Code: */
#ifndef XFORM_H
#define XFORM_H

#include <math.h>               /* for NAN (if supported) */

struct xform_table;
struct xform_table_entry {
  /* lower bound and the value */
  double bound, value;
};

/** allocation and deallocation */
extern struct xform_table *xform_table_alloc (size_t allocate);
extern void xform_table_free (struct xform_table *table);

/** mapping of NaN and out-of-range values */
#ifdef NAN
extern double xform_table_nan (const struct xform_table *table);
extern void xform_table_nan_set (struct xform_table *table,
                                 double value);
#endif
extern double xform_table_oor (const struct xform_table *table);
extern void xform_table_oor_set (struct xform_table *table,
                                 double value);

/** choosing interpolation */
extern void xform_table_no_interp (struct xform_table *table);
extern int  xform_table_linear_interp (struct xform_table *table);

/** clearing and filling the table */
extern void xform_table_clear (struct xform_table *table);
extern int  xform_table_append (struct xform_table *table,
                                const struct xform_table_entry *ents,
                                size_t size);

/** apply the transformation */
extern void xform_table_apply (const struct xform_table *table,
                               double *to, const double *from,
                               size_t size);

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
/*** xform.h ends here */
