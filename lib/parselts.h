/*** parselts.h --- Parsing arrays and their elements  -*- C -*- */

/*** Copyright (C) 2007 Ivan Shmakov */

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
