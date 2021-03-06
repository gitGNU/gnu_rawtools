/*** rawxform.c --- Apply table transformation  -*- C -*- */
#define _GNU_SOURCE
#include "config.h"
#include "gettext.h"
#define _(string) gettext (string)
#define N_(string) gettext_noop (string)
static const char doc[] = N_("Apply table transformation");
static const char args_doc[] = "[FILE]...";

/*** Copyright (C) 2006, 2007 Ivan Shmakov */

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

const char *
program_copyright (void)
{
  return _("Copyright (C) 2006, 2007 Ivan Shmakov\n"
           "This is free software; see the source for copying"
           " conditions.  There is NO\n"
           "warranty; not even for MERCHANTABILITY or FITNESS FOR A"
           " PARTICULAR PURPOSE.\n");
}

/*** Code: */
#include <argp.h>
#include <assert.h>
#include <errno.h>
#include <error.h>
#include <locale.h>
#include <math.h>
#include <stdint.h>             /* for uint8_t */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "numconv.h"
#include "p_arg.h"
#include "usemacro.h"
#include "useutil.h"
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
  FORMAT_UINT8 = 0,
  FORMAT_FLOAT,
  FORMAT_DOUBLE
};

static int
apply_table (FILE *out, struct xform_table *table,
             enum flt_format fmt, enum flt_format out_fmt,
             FILE *in)
{
  float   buf_1[BUF_SZ];
  double  buf_inter[BUF_SZ];
  void    *buf_in
    = (fmt == FORMAT_FLOAT) ?  (void *)buf_1  :  (void *)buf_inter;
  size_t  in_elt_sz
    = (fmt == FORMAT_FLOAT) ? sizeof (*buf_1) : sizeof (*buf_inter);
  size_t  count;

  assert (fmt == FORMAT_FLOAT || fmt == FORMAT_DOUBLE);
  assert (out_fmt == FORMAT_UINT8
          || out_fmt == FORMAT_FLOAT || out_fmt == FORMAT_DOUBLE);
  while ((count = fread (buf_in, in_elt_sz, BUF_SZ, in))
         > 0) {
    size_t count_w;
    if (fmt == FORMAT_FLOAT) {
      nconv_double_from_float (buf_inter, buf_1, count);
    }
    xform_table_apply (table, buf_inter, buf_inter, count);
    switch (out_fmt) {
    case FORMAT_UINT8:
      {
        uint8_t obuf[BUF_SZ];
        uint8s_from_doubles (obuf, buf_inter, count);
        count_w = fwrite ((void *)obuf, sizeof (*obuf), count, out);
      }
      break;
    case FORMAT_FLOAT:
      {
        float obuf[BUF_SZ];
        nconv_float_from_double (obuf, buf_inter, count);
        count_w = fwrite ((void *)obuf, sizeof (*obuf), count, out);
      }
      break;
    case FORMAT_DOUBLE:
      count_w = fwrite ((void *)buf_inter, sizeof (*buf_inter), count,
                        out);
      break;
    default:
      /* NB: should not happen */
      error (1, 0, "%s:%d: unhandled output format",
             __FUNCTION__, out_fmt);
      break;
    }
    if (count_w != count) {
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
    "rawxform " PACKAGE_NAME " " PACKAGE_VERSION;
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
  [FORMAT_UINT8]  = "uint8",
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
    N_("select input format, which may be"
       " `float' or `double' (default)") },
  { "output-format",    'T', "TYPE", 0,
    N_("select output format, which may be `uint8',"
       " `float' or `double' (default)") },
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
  int output_format;
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
  int rv;
  int chars;
  struct xform_table_entry ent;

  if (rv = sscanf (arg, " %lf , %lf%n",
                   &ent.bound, &ent.value, &chars),
      (rv != 2 && rv != 3)
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
      if ((i = p_arg_string (arg, format_opts, 0)) < 0
          && i != FORMAT_DOUBLE && i != FORMAT_FLOAT) {
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
  case 'T':
    {
      int i;
      if ((i = p_arg_string (arg, format_opts, 0)) < 0) {
        argp_error (state,
                    N_("invalid argument `%s' for `--format';"
                       " should be `uint8', `float' or `double'"),
                    arg);
        /* . */
        return EINVAL;
      }
      args->output_format = i;
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
    FORMAT_DOUBLE,
    FORMAT_DOUBLE,
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
      if (apply_table (output, table,
                       args.input_format, args.output_format,
                       fp) < 0) {
        error (1, errno, "%s", *np);
      }
      if (fp != stdin)
        fclose (fp);
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
/*** rawxform.c ends here */
