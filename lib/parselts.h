/*** parselts.h --- Parsing arrays and their elements  -*- C -*- */

/*** Copyright (C) 2007 Ivan Shmakov */

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
#ifndef PARSELTS_H
#define PARSELTS_H

typedef int (*parse_elt_fn) (const char *string, void *buffer,
                             char **tailptr, void *param);

struct parse_elt_number_param {
  int allow_whitespace_after_p;
  int allow_out_of_range_p;
};

struct parse_elt_integer_param {
  struct parse_elt_number_param number_param;
  int base;
};

struct parse_elt_of_strings_param {
  const char *const *tokens;
  int skip_whitespace_after_p;
  int skip_whitespace_before_p;
};

/** Parsing single elements */

int parse_elt_long (const char *string, long *buf, char **tailptr,
                    struct parse_elt_integer_param *param);
int parse_elt_ulong (const char *string, unsigned long *buf,
                     char **tailptr,
                     struct parse_elt_integer_param *param);
int parse_elt_llong (const char *string, long long *buf,
                     char **tailptr,
                     struct parse_elt_integer_param *param);
int parse_elt_ullong (const char *string, unsigned long long *buf,
                      char **tailptr,
                      struct parse_elt_integer_param *param);

int parse_elt_float  (const char *string, float  *buf, char **tailptr,
                      struct parse_elt_number_param *param);
int parse_elt_double (const char *string, double *buf, char **tailptr,
                      struct parse_elt_number_param *param);
int parse_elt_ldouble (const char *string, long double *buf,
                       char **tailptr,
                       struct parse_elt_number_param *param);

/*** Loading arrays from delimited strings */

int parse_elts_delim (const char *string, void **buf, size_t elt_sz,
                      size_t *allocptr, size_t *sizeptr,
                      int delim, char **tailptr,
                      parse_elt_fn parse_elt, void *param);

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
/*** parselts.h ends here */
