/*** rawflt2u1.c --- Convert `float' stream into `uint8_t' one  -*- C -*- */
#define _GNU_SOURCE
#include "config.h"
#include "gettext.h"
#define _(string) gettext (string)
#define N_(string) gettext_noop (string)
static const char doc[] = N_("Convert `float' stream into `uint8_t' one");
static const char args_doc[] = "[FILE]...";

/*** Copyright (C) 2006 Ivan Shmakov */

const char *
program_copyright (void)
{
  return _("Copyright (C) 2006 Ivan Shmakov\n"
           "This is free software; see the source for copying"
           " conditions.  There is NO\n"
           "warranty; not even for MERCHANTABILITY or FITNESS FOR A"
           " PARTICULAR PURPOSE.\n");
}

/*** Code: */
#include <argp.h>
#include <errno.h>
#include <error.h>
#include <locale.h>
#include <math.h>
#include <stdint.h>             /* for uint8_t */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "p_arg.h"
#include "usemacro.h"
#include "xform.h"

#define BUF_SZ  4096

#if 0
#define DFL_OUTPUT_MULT  (255L)
#define DFL_INPUT_MULT   ((double)1)
#endif

#if 0
/* FIXME: unused for now */
#ifdef NAN
#define DFL_NAN_VALUE    NAN
#endif
#define DFL_RANGE_VALUE  (- INFINITY)
#endif

/*** Utility */

static int
ensure_enough_space (void **ptr, size_t *allocptr,
                     size_t size, size_t wanted)
{
  const size_t alloc = (*ptr != 0) ? *allocptr : 0;
  if (wanted <= alloc) {
    /* . */
    return 0;
  }

  {
    /* FIXME: magic formulae */
    const size_t
      sz = MAX (MAX (3, (alloc << 1) + 1), wanted);
    void *new;
    if ((new = realloc (*ptr, size * sz)) == 0) {
      /* . */
      return -1;
    }
    *ptr = new;
    *allocptr = sz;
  }

  /* . */
  return 0;
}

static FILE *
open_file (const char *arg, int input_p)
{
  FILE *fp;

  if (arg == 0) {
    errno = EINVAL;
    /* . */
    return 0;
  } else if (strcmp ("-", arg) == 0) {
    /* . */
    return input_p ? stdin : stdout;
  } else if ((fp = fopen (arg, input_p ? "r" : "w")) == 0) {
    /* . */
    return 0;
  }

  /* . */
  return fp;
}

static void
doubles_from_floats (double *dst, const float *src, size_t count)
{
  size_t rest;
  double *dp;
  const float *sp;
  for (rest = count, dp = dst, sp = src;
       rest > 0;
       rest--, *(dp++) = *(sp++))
    ;
}

static void
uint8s_from_doubles (uint8_t *dst, const double *src, size_t count)
{
  size_t rest;
  uint8_t *dp;
  const double *sp;
  for (rest = count, dp = dst, sp = src;
       rest > 0;
       rest--, *(dp++) = BOUND (*sp, 0, 255), sp++)
    ;
}

static int
load_table (struct xform_table *table, FILE *fp,
            unsigned int *lineno)
{
  unsigned int lno;
  char *line = 0;
  size_t l_sz = 0;
  ssize_t chars;

  for (lno = 1;
       (chars = getline (&line, &l_sz, fp)) >= 0;
       lno++) {
    const char *s = p_arg_skipspace (line);
    struct xform_table_entry ent;
    if (*s == '\0' || *s == '#') {
      /* the line is empty or is a commentary */
      continue;
    }
    if (sscanf (s, "%lf %lf", &(ent.bound), &(ent.value))
        != 2) {
      if (lineno != 0)
        *lineno = lno;
      errno = EINVAL;
      /* . */
      return -1;
    }
    xform_table_append (table, &ent, 1);
  }

  if (! feof (fp)) {
    if (lineno != 0)
      *lineno = lno;
    /* . */
    return -1;
  }

  /* . */
  return lno - 1;
}

enum flt_format {
  FORMAT_FLOAT = 0,
  FORMAT_DOUBLE
};

static int
apply_table (FILE *out, struct xform_table *table,
             enum flt_format fmt, FILE *in)
{
  float   buf_1[BUF_SZ];
  double  buf_inter[BUF_SZ];
  uint8_t buf_out[BUF_SZ];
  void    *buf_in
    = (fmt == FORMAT_FLOAT) ?  (void *)buf_1  :  (void *)buf_inter;
  size_t  in_elt_sz
    = (fmt == FORMAT_FLOAT) ? sizeof (*buf_1) : sizeof (*buf_inter);
  size_t  count;

  while ((count = fread (buf_in, in_elt_sz, BUF_SZ, in))
         > 0) {
    size_t count_w;
    if (fmt == FORMAT_FLOAT) {
      doubles_from_floats (buf_inter, buf_1, count);
    }
    xform_table_apply (table, buf_inter, buf_inter, count);
    uint8s_from_doubles (buf_out, buf_inter, count);
    if ((count_w = fwrite ((void *)buf_out, sizeof (*buf_out),
                           count, out))
        != count) {
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

/*** Vectors of Strings */

struct strings {
  size_t size, alloc;
  const char **s;
};

static int
strings_append (struct strings *vec, const char **s, size_t count)
{
  if (count == 0) {
    /* . */
    return 0;
  }
  if (ensure_enough_space ((void **)&(vec->s), &(vec->alloc),
                           sizeof (*vec->s), count + vec->size)
      != 0) {
    /* . */
    return -1;
  }
  COPY_ARY (vec->s + vec->size, s, count);
  vec->size += count;

  /* . */
  return 0;
}

static void
strings_clear (struct strings *vec)
{
  if (vec->s != 0) free (vec->s);
  vec->size = vec->alloc = 0;
}

/*** Parsing the Command Line */

const char *
program_version (void)
{
  return
    "rawflt2u1 " PACKAGE_NAME " " PACKAGE_VERSION;
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
  opt_offset = 256,
  opt_max
};

enum {
  INTERP_NONE = 0,
  INTERP_LINEAR,
  INTERP_MAX
};

const char *interp_opts[] = {
  [INTERP_NONE]   = "none",
  [INTERP_LINEAR] = "linear",
  [INTERP_MAX]    = 0
};

const char *format_opts[] = {
  [FORMAT_FLOAT]  = "float",
  [FORMAT_DOUBLE] = "double"
};

static struct argp_option p_opts[] = {
  { 0, 0, 0, 0, /***/ N_("specifying the transformation table") },
#if 0
  { "input-multiplier", 'm', "FLOAT", 0,
    N_("pre-multiply the table boundaries"
       " with the inverse of this value") },
  { "output-multiplier", 'M', "FLOAT", 0,
    N_("pre-multiply the table values"
       " with the inverse of this value") },
#endif
  { "table-file",       'f', "FILE", 0,
    N_("append the contents of the file to the transformation table") },
  { "table-entry",      'e', "ENTRY", 0,
    N_("append ENTRY to the transformation table;"
       " ENTRY is a pair of floats separated by a comma") },
  { 0, 0, 0, 0, /***/ N_("miscellaneous") },
  { "format",           't', "TYPE", 0,
    N_("select input format, which may be `float' (default)"
       " or `double'") },
  { "interpolate",      'I', "TYPE", 0,
    N_("use interpolation TYPE, which may be `none' (default)"
       " or `linear'") },
  { "output",           'o', "FILE", 0,
    N_("output the result to this file instead of stdout") },
  { "verbose",          'v', 0, 0,
    N_("explain what is being done") },
  { 0 }
};

struct p_args {
  int verbose_p;
#if 0
  double input_mult, output_mult;
#endif
  int interpolation;
  int input_format;
  const char *output_file;
  struct strings table_files;
  struct xform_table *x_table;
#if 0
  /* FIXME: unused for now */
#ifdef NAN
  double nan_value;
#endif
  double range_value;
#endif
  struct strings input_files;
};

static int
handle_table_entry (struct p_args *args, const char *arg)
{
  int chars;
  struct xform_table_entry ent;

  if (sscanf (arg, " %lf , %lf%n",
              &ent.bound, &ent.value, &chars) != 3
      || arg[chars] != '\0') {
    errno = EINVAL;
    /* . */
    return -1;
  }

  /* . */
  return
    xform_table_append (args->x_table, &ent, 1);
}

static error_t
p_opt (int key, char *arg, struct argp_state *state)
{
  struct p_args *args = state->input;

  /* FIXME: looks ugly! */

  switch (key) {
#if 0
  case 'm':
  case 'M':
    if (p_arg_double (arg, (key == 'm'
                            ? &(args->input_mult)
                            : &(args->output_mult)))
        < 0) {
      argp_error (state,
                  N_("invalid argument `%s' for `%s';"
                     " should be a float"),
                  (key == 'm'
                   ? "--input-multiplier"
                   : "--output-multiplier"));
      /* . */
      return EINVAL;
    }
    break;
#endif
  case 'f':
    if (strings_append (&(args->table_files), &arg, 1) < 0) {
      /* FIXME: is this okay? */
      argp_failure (state, 0, errno,
                    N_("couldn't handle file name `%s'"), arg);
      /* . */
      return errno;
    }
    break;
  case 'e':
    if (handle_table_entry (args, arg) >= 0) {
      /* do nothing */
    } else if (errno == EINVAL) {
      argp_error (state,
                  N_("invalid entry `%s'; should be `FLOAT, FLOAT'"),
                  arg);
      /* . */
      return EINVAL;
    } else {
      /* FIXME: is this okay? */
      argp_failure (state, 0, errno,
                    N_("couldn't handle entry `%s'"), arg);
      /* . */
      return errno;
    }
    break;
  case 'I':
    {
      int i;
      if ((i = p_arg_string (arg, interp_opts, 0)) < 0) {
        argp_error (state,
                    N_("invalid argument `%s' for `--interpolation';"
                       " should be `none' or `linear'"),
                    arg);
        /* . */
        return EINVAL;
      }
      args->interpolation = i;
    }
    break;
  case 't':
    {
      int i;
      if ((i = p_arg_string (arg, format_opts, 0)) < 0) {
        argp_error (state,
                    N_("invalid argument `%s' for `--format';"
                       " should be `float' or `double'"),
                    arg);
        /* . */
        return EINVAL;
      }
      args->input_format = i;
    }
    break;
  case 'o':
    args->output_file = arg;
    break;
  case 'v':
    args->verbose_p = 1;
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
  case ARGP_KEY_NO_ARGS:
    {
      const char *s[1] = { "-" };
      if (strings_append (&(args->input_files), s, 1) < 0) {
        argp_failure (state, 0, errno, 
                      N_("couldn't set stdin as the input file"));
        /* . */
        return errno;
      }
    }
    break;
  case ARGP_KEY_END:
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
    0,
#if 0
    DFL_INPUT_MULT,
    DFL_OUTPUT_MULT,
#endif
    INTERP_NONE,
    FORMAT_FLOAT,
    "-",
    { 0, 0, 0 },
    0,
#if 0
#ifdef NAN
    NAN,
#endif
    - INFINITY,
#endif
    { 0, 0, 0 }
  };
  FILE *output;

  /* set the locale */
  setlocale (LC_ALL, "");

#if ENABLE_NLS
  /* set the default domain for translations */
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);
#endif

  /* allocate a transformation table */
  if ((args.x_table = xform_table_alloc (0)) == 0) {
    error (1, errno, N_("couldn't allocate a transformation table"));
  }

  /* parse the command line */
  {
    static struct argp argp = { p_opts, p_opt, args_doc, doc };
    argp_parse (&argp, argc, argv, 0, 0, &args);
  }

  /* read the tables */
  {
    const struct strings *names = &(args.table_files);
    struct xform_table *table = args.x_table;
    size_t rest;
    const char **np;
    for (rest = names->size, np = names->s;
         rest > 0;
         rest--, np++) {
      FILE *fp;
      unsigned int line;
      if ((fp = open_file (*np, 1)) == 0) {
        error (1, errno, "%s", *np);
      }
      if (load_table (table, fp, &line) >= 0) {
        /* do nothing */
      } else if (line == 0) {
        error (1, errno, "%s", *np);
      } else {
        error_at_line (1, errno, *np, line,
                       N_("error parsing line%s"),
                       errno == EINVAL ? _(": garbage found") : "");
      }
      if (fp != stdin)
        fclose (fp);
    }
  }

  /* NB: these file names aren't needed any longer */
  strings_clear (&(args.table_files));

  /* prepare the coefficients if needed */
  if (args.interpolation == INTERP_LINEAR) {
    xform_table_linear_interp (args.x_table);
  }

  /* open the output file */
  if ((output = open_file (args.output_file, 0)) == 0) {
    error (1, errno, "%s", args.output_file);
  }

  /* process the input files */
  {
    const struct strings *names = &(args.input_files);
    struct xform_table *table = args.x_table;
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
      if (apply_table (output, table, args.input_format, fp) < 0) {
        error (1, errno, "%s", *np);
      }
      if (fp != stdin)
        fclose (fp);
    }
  }

  /* close output */
  if (output != stdout)
    fclose (output);

  /* . */
  return 0;
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
/*** rawflt2u1.c ends here */
