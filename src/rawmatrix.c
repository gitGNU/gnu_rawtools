/*** rawmatrix.c --- Multiply file data by a matrix  -*- C -*- */
#define _GNU_SOURCE
#include "config.h"
#include "gettext.h"
#define _(string) gettext (string)
#define N_(string) gettext_noop (string)
static const char doc[] = N_("Multiply input vectors by a matrix");
static const char args_doc[]
= N_("MATRIX [FILE]...\n"
     "-m MATRIX [FILE]...");

/*** Copyright (C) 2007 Ivan Shmakov */

const char *
program_copyright (void)
{
  return _("Copyright (C) 2007 Ivan Shmakov\n"
           "This is free software; see the source for copying"
           " conditions.  There is NO\n"
           "warranty; not even for MERCHANTABILITY or FITNESS FOR A"
           " PARTICULAR PURPOSE.\n");
}

/*** Code: */
#include <assert.h>
#include <argp.h>
#include <errno.h>
#include <error.h>
#include <locale.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>             /* for memset () */

#include "p_arg.h"
#include "parselts.h"
#include "usemacro.h"
#include "useutil.h"

/*** Handling formats */

enum raw_format {
  FORMAT_DOUBLE = 0,
  FORMAT_FLOAT
};

static int
all_double_p (size_t size, const enum raw_format *array)
{
  size_t rest;
  const enum raw_format *p;
  for (rest = size, p = array; rest > 0; rest--, p++) {
    if (*p != 0) {
      /* . */
      return 0;
    }
  }

  /* . */
  return 1;
}

static int
fread_doubles (double *buf, size_t count, FILE *fp)
{
  size_t read;
  if ((read = fread (buf, sizeof (*buf), count, fp)) < count) {
    /* . */
    return (read == 0 && feof (fp)) ? 0 : -1;
  }
  return 1;
}

static int
fwrite_doubles (const double *buf, size_t count, FILE *fp)
{
  size_t wrtn;
  if ((wrtn = fwrite (buf, sizeof (*buf), count, fp)) < count) {
    /* . */
    return -1;
  }
  return 1;
}

static int
fread_doubles_fmt (double *buf, size_t count,
                   const enum raw_format *fmts,
                   FILE *inp)
{
  size_t rest;
  double *bp;
  const enum raw_format *fp;
  for (rest = count, bp = buf, fp = fmts;
       rest > 0;
       rest--, bp++, fp++) {
    float bf;
    void  *p1 = (*fp == FORMAT_DOUBLE ? (void *)bp : (void *)&bf);
    size_t s1 = (*fp == FORMAT_DOUBLE
                 ? sizeof (double) : sizeof (float));
    size_t read;
    if ((read = fread (p1, s1, 1, inp)) != 1) {
      /* . */
      return (bp == buf && read == 0 && feof (inp)) ? 0 : -1;
    }
    if (bp != p1) {
      *bp = bf;
    }
  }
  return 1;
}

static int
fwrite_doubles_fmt (const double *buf, size_t count,
                    const enum raw_format *fmts,
                    FILE *outp)
{
  size_t rest;
  const double *bp;
  const enum raw_format *fp;
  for (rest = count, bp = buf, fp = fmts;
       rest > 0;
       rest--, bp++, fp++) {
    float bf;
    void  *p1 = (*fp == FORMAT_DOUBLE ? (void *)bp : (void *)&bf);
    size_t s1 = (*fp == FORMAT_DOUBLE
                 ? sizeof (double) : sizeof (float));
    size_t wrtn;
    if (bp != p1) {
      bf = *bp;
    }
    if ((wrtn = fwrite (p1, s1, 1, outp)) != 1) {
      /* . */
      return -1;
    }
  }
  return 1;
}

/*** Vectors and matrices */

struct sim_matrix {
  size_t alloc;
  size_t rows, columns;
  /* NB: values[ROW * columns + COLUMN] */
  double *values;
};

static double
scalar_product (size_t size, const double *a, const double *b)
{
  /* FIXME: qsort () an intermediate array for a better precision? */
  size_t rest;
  double acc;
  const double *ap, *bp;
  for (rest = size, acc = 0, ap = a, bp = b;
       rest > 0;
       rest--, acc += *(ap++) * *(bp++))
    ;
  /* . */
  return acc;
}

static void
matrix_by_vector (double *result,
                  const struct sim_matrix *matrix,
                  const double *vector)
{
  size_t vector_sz = matrix->columns;
  size_t rest;
  double *rp;
  const double *vp;
  for (rest = matrix->rows, rp = result, vp = matrix->values;
       rest > 0;
       rest--, rp++, vp += vector_sz) {
    *rp = scalar_product (vector_sz, vp, vector);
  }
}

static int
load_matrix_csv (struct sim_matrix *m, const char *s,
                 int trailing_1_p)
{
  const int delim = ',';
  struct parse_elt_number_param param = { 0, 0 };
  size_t elts;
  char *tail;
  size_t root;

  m->alloc = 0;
  m->values = 0;
  elts = 0;

  parse_elts_delim (s, (void *)&(m->values), sizeof (*(m->values)),
                    &(m->alloc), &elts, delim, &tail,
                    parse_elt_double, (const void *)&param);
  if (tail == s || *tail != '\0') {
    errno = EINVAL;
    /* . */
    return -1;
  }
  assert (elts != 0);

  /* NB: assuming it's a row, temporarily */
  m->columns = elts;
  m->rows    = 1;

  /* . */
  return 0;
}

/*** Applying matrix to a stream */

static int
apply_matrix (FILE *out, const struct sim_matrix *matrix,
              const enum raw_format *in_fmts,
              const enum raw_format *out_fmts,
              int trailing_1_p,
              FILE *in)
{
  const size_t
    inb_sz = matrix->columns,
    in_sz  = inb_sz + (trailing_1_p ? -1 : 0),
    out_sz = matrix->rows;
  const int
    i_direct = all_double_p  (in_sz,  in_fmts),
    o_direct = all_double_p (out_sz, out_fmts);
  double
    i_buf[inb_sz],
    o_buf[out_sz];

  while ((i_direct ? fread_doubles (i_buf, in_sz, in)
          : fread_doubles_fmt (i_buf, in_sz, in_fmts, in)) == 1) {
    int written;
    if (inb_sz > in_sz) {
      i_buf[in_sz] = 1.;
    }
    matrix_by_vector (o_buf, matrix, i_buf);
    if ((written = (o_direct
                    ? fwrite_doubles (o_buf, out_sz, out)
                    : fwrite_doubles_fmt (o_buf, out_sz,
                                          out_fmts, out)))
        != 1) {
      /* . */
      return -1;
    }
  }

  if (! feof (in)) {
    /* . */
    return -1;
  }

  /* . */
  return 0;
}

/*** Parsing the Command Line */

const char *
program_version (void)
{
  return
    "rawmatrix " PACKAGE_NAME " " PACKAGE_VERSION;
}

static void
p_vers (FILE *fp, struct argp_state *unused)
{
  fprintf (fp,
           "%s\n\n%s\n",
           program_version (),
           program_copyright ());
}

const char *argp_program_bug_address = PACKAGE_BUGREPORT;
void (*argp_program_version_hook)(FILE *, struct argp_state *) = p_vers;

enum opts {
  opt_trailing_1 = 256,
  opt_max
};

const char *format_opts[] = {
  [FORMAT_DOUBLE] = "double",
  [FORMAT_FLOAT]  = "float"
};

static struct argp_option p_opts[] = {
  { "trailing-1",       opt_trailing_1, 0, 0,
    N_("append a value of 1.0 to each of the vectors read") },
#if 0
  /* FIXME: implement it! */
  { "format",           't', "TYPE", 0,
    N_("select input format, which may be"
       " `float' or `double' (default)") },
  { "output-format",    'T', "TYPE", 0,
    N_("select output format, which may be"
       " `float' or `double' (default)") },
#endif
  { "matrix",           'm', "MATRIX", 0,
    N_("specify the matrix elements") },
  { "vector-size",      's', "ELTS", 0,
    N_("read vectors of this number of elements") },
  { "output",           'o', "FILE", 0,
    N_("output the result to this file instead of stdout") },
  { "verbose",          'v', 0, 0,
    N_("explain what is being done") },
  { 0 }
};

struct p_args {
  int verbose_p;
  int trailing_1_p;
  enum raw_format *in_fmts;
  enum raw_format *out_fmts;
  long vector_size;
  int matrix_read_p;
  struct sim_matrix matrix;
  const char *output_file;
  struct strings input_files;
};

static error_t
p_opt (int key, char *arg, struct argp_state *state)
{
  struct p_args *args = state->input;

  /* FIXME: looks ugly! */

  switch (key) {
  case ARGP_KEY_INIT:
    args->matrix.columns = 0;
    args->matrix.rows = 0;
    break;
  case opt_trailing_1:
    args->trailing_1_p = 1;
    break;
  case 't':
    argp_failure (state, 0, 0, N_("-%c: not implemented yet"), key);
    /* FIXME: implement it! */
    break;
  case 'T':
    argp_failure (state, 0, 0, N_("-%c: not implemented yet"), key);
    /* FIXME: implemented it! */
    break;
  case 's':
    {
      long vs;
      if (p_arg_long (arg, &vs) < 0 || vs < 1) {
        argp_error (state,
                    N_("%s: not a valid size,"
                       " should be a positive number"),
                    arg);
        /* . */
        return EINVAL;
      }
      args->vector_size = vs;
    }
    break;
  case 'o':
    args->output_file = arg;
    break;
  case 'v':
    args->verbose_p = 1;
    break;
  case ARGP_KEY_ARG:
    /* check if matrix was already read */
    if (args->matrix_read_p) {
      /* . */
      return ARGP_ERR_UNKNOWN;
    }
    /* NB: no `break' here */
  case 'm':
    /* parse matrix */
    if (load_matrix_csv (&(args->matrix), arg, args->trailing_1_p)
        >= 0) {
      /* do nothing */
    } else if (errno == EINVAL) {
      argp_error (state,
                  N_("%s: not a valid matrix,"
                     " should be `NUMBER[, NUMBER]...'"),
                  arg);
      /* . */
      return EINVAL;
    } else {
      argp_failure (state, 0, errno,
                    N_("%s: couldn't store matrix"), arg);
      /* . */
      return errno;
    }

    /* obtain matrix size */
    {
      struct sim_matrix *m = &(args->matrix);
      const int trailing_1_p = args->trailing_1_p;
      const size_t elts = m->columns;
      const size_t cols1
        = (args->vector_size > 0
           ? args->vector_size + (trailing_1_p ? 1 : 0)
           : 0);
      const size_t cols
        = (cols1 > 0
           ? cols1
           : ((size_t)floor (sqrt ((double)elts))
              + (trailing_1_p ? 1 : 0)));

      assert (m->columns > 0 && m->rows == 1);

      /* check if the number of columns is given */
      if (cols1 > 0 && elts % cols1 != 0) {
        argp_error (state,
                    N_("%s: integral number of rows expected"),
                    arg);
        /* . */
        return EINVAL;
      } else if (elts == (trailing_1_p ? cols - 1 : cols) * cols) {
        /* do nothing, M x M or M x (M + 1) matrix is given */
      } else if (trailing_1_p) {
        argp_error (state, N_("%s: M x (M + 1) matrix expected"), arg);
        /* . */
        return EINVAL;
      } else {
        argp_error (state, N_("%s: square matrix expected"), arg);
        /* . */
        return EINVAL;
      }
      m->columns = cols;
      m->rows    = elts / cols;
    }

    /* raise the flag */
    args->matrix_read_p = 1;
    break;
  case ARGP_KEY_ARGS:
    if (strings_append (&(args->input_files),
                        state->argv + state->next,
                        state->argc - state->next) < 0) {
      argp_failure (state, 0, errno, 
                    N_("couldn't store non-option arguments"));
      /* . */
      return errno;
    }
    break;
  case ARGP_KEY_END:
    if (! args->matrix_read_p) {
      argp_error (state, N_("no matrix specified"));
      /* . */
      return EINVAL;
    }
    if (args->input_files.size <= 0) {
      const char *s[1] = { "-" };
      if (strings_append (&(args->input_files), s, 1) < 0) {
        argp_failure (state, 0, errno, 
                      N_("couldn't set stdin as the input file"));
        /* . */
        return errno;
      }
    }
    if (args->output_file == 0) {
      args->output_file = "-";
    }
    assert (args->input_files.size > 0);
    /* FIXME: replace this with an actual code */
    {
      const size_t
        in_sz  = args->matrix.columns,
        out_sz = args->matrix.rows;
      MALLOC_ARY (args->in_fmts,  in_sz);
      MALLOC_ARY (args->out_fmts, out_sz);
      assert (args->in_fmts != 0 && args->out_fmts != 0);
      /* initialize to FORMAT_DOUBLE, which is 0 */
      memset (args->in_fmts,  0,  in_sz * sizeof  (*(args->in_fmts)));
      memset (args->out_fmts, 0, out_sz * sizeof (*(args->out_fmts)));
    }
    break;
  default:
    /* . */
    return ARGP_ERR_UNKNOWN;
    break;
  }

  /* . */
  return 0;
}

/*** main () */

int
main (int argc, char **argv)
{
  struct p_args args = {
    .verbose_p = 0,
    .trailing_1_p = 0,
    .in_fmts = 0,
    .out_fmts = 0,
    .vector_size = 0,
    .matrix_read_p = 0,
    .matrix = { 0, 0, 0, 0 },
    .output_file = 0,
    .input_files = { 0, 0, 0 }
  };
  FILE *output;

  /* set the locale */
  setlocale (LC_ALL, "");

#if ENABLE_NLS
  /* set the default domain for translations */
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);
#endif

  /* parse the command line */
  {
    static struct argp argp = { p_opts, p_opt, args_doc, doc };
    argp_parse (&argp, argc, argv, 0, 0, &args);
  }

  /* open the output file */
  if ((output = open_file (args.output_file, 0)) == 0) {
    error (1, errno, "%s", args.output_file);
  }

  /* process the input files */
  {
    const struct strings *names = &(args.input_files);
    const struct sim_matrix *matrix = &(args.matrix);
    size_t rest;
    const char **np;
    for (rest = names->size, np = names->s;
         rest > 0;
         rest--, np++) {
      FILE *fp;

      if ((fp = open_file (*np, 1)) == 0) {
        error (1, errno, "%s", *np);
      }
      if (args.verbose_p) {
        if (fp == stdin)
          fputs (_("processing standard input...\n"), stderr);
        else
          fprintf (stderr, _("processing `%s'...\n"), *np);
      }
      if (apply_matrix (output, matrix,
                        args.in_fmts, args.out_fmts,
                        args.trailing_1_p,
                        fp) < 0) {
        error (1, errno, "%s", *np);
      }
      close_file (fp);
    }
  }

  /* close output */
  close_file (output);

  /* . */
  return 0;
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
/*** rawmatrix.c ends here */
