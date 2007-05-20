/*** rawrange.c --- ?  -*- C -*- */
#define _GNU_SOURCE
#include "config.h"
#include "gettext.h"
#define _(string) gettext (string)
#define N_(string) gettext_noop (string)
static const char doc[] = N_("Interleave parts of the files");
static const char args_doc[]
= N_("[FILE]...\n\v"
     "Supported format names are:"
     " uint8 (default), uint16, uint32, uint64,"
     " int8, int16, int32, int64,"
     " float, double");

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
#include <argp.h>
#include <assert.h>
#include <errno.h>
#include <error.h>
#include <locale.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "numrange.h"
#include "p_arg.h"
#include "usemacro.h"
#include "useutil.h"

#define PROGRAM_NAME "rawrange"

#define BUF_SZ 4096

/*** Utility */

enum num_format {
  FORMAT_UINT8 = 0,
  FORMAT_UINT16,
  FORMAT_UINT32,
  FORMAT_UINT64,
  FORMAT_INT8,
  FORMAT_INT16,
  FORMAT_INT32,
  FORMAT_INT64,
  FORMAT_FLOAT,
  FORMAT_DOUBLE,
  FORMAT_MAX
};

const char *format_opts[] = {
  [FORMAT_UINT8]  = "uint8",
  [FORMAT_UINT16] = "uint16",
  [FORMAT_UINT32] = "uint32",
  [FORMAT_UINT64] = "uint64",
  [FORMAT_INT8]   = "int8",
  [FORMAT_INT16]  = "int16",
  [FORMAT_INT32]  = "int32",
  [FORMAT_INT64]  = "int64",
  [FORMAT_FLOAT]  = "float",
  [FORMAT_DOUBLE] = "double",
  [FORMAT_MAX]    = 0
};

/*** Parsing the Command Line */

const char *
program_version (void)
{
  return
    PROGRAM_NAME " " PACKAGE_NAME " " PACKAGE_VERSION;
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
  opt_max
};

static struct argp_option p_opts[] = {
  { "format",           't', "TYPE", 0,
    N_("select input format") },
  { "verbose",          'v', 0, 0,
    N_("explain what is being done") },
  { 0 }
};

struct p_args {
  int verbose_p;
  int format;
  struct strings files;
};

static error_t
p_opt (int key, char *arg, struct argp_state *state)
{
  struct p_args *args = state->input;

  switch (key) {
  case 't':
    {
      int i;
      if ((i = p_arg_string (arg, format_opts, 0)) < 0) {
        argp_error (state,
                    N_("invalid argument `%s' for `--format'"),
                    arg);
        /* . */
        return EINVAL;
      }
      args->format = i;
    }
    break;
  case 'v':
    args->verbose_p = 1;
    break;
  case ARGP_KEY_ARGS:
    if (strings_append (&(args->files),
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
      if (strings_append (&(args->files), s, 1) < 0) {
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
    .verbose_p  = 0,
    .format     = FORMAT_UINT8,
    .files      = { 0, 0, 0 },
  };
  FILE *output = stdout;

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

  /* process the input files */
  {
    const struct strings *names = &(args.files);
    const int t = args.format;
    /* NB: casting to `void *' */
    void (*extend) (const void *vec, size_t size,
                    void *min, void *max)
      = (t == FORMAT_INT8     ? nrange_extend_int8_tt
         : t == FORMAT_INT16  ? nrange_extend_int16_tt
         : t == FORMAT_INT32  ? nrange_extend_int32_tt
         : t == FORMAT_INT64  ? nrange_extend_int64_tt
         : t == FORMAT_UINT8  ? nrange_extend_uint8_tt
         : t == FORMAT_UINT16 ? nrange_extend_uint16_tt
         : t == FORMAT_UINT32 ? nrange_extend_uint32_tt
         : t == FORMAT_UINT64 ? nrange_extend_uint64_tt
         : t == FORMAT_FLOAT  ? nrange_extend_float
         : t == FORMAT_DOUBLE ? nrange_extend_double
         : 0);
    const size_t elt_sz
      = (t == FORMAT_UINT8 || t == FORMAT_INT8 ? sizeof (int8_t)
         : t == FORMAT_UINT16 || t == FORMAT_INT16 ? sizeof (int16_t)
         : t == FORMAT_UINT32 || t == FORMAT_INT32 ? sizeof (int32_t)
         : t == FORMAT_UINT64 || t == FORMAT_INT64 ? sizeof (int64_t)
         : t == FORMAT_FLOAT  ? sizeof (float)
         : t == FORMAT_DOUBLE ? sizeof (double)
         : 0);
    const size_t buf_elts = BUF_SZ / elt_sz;
    char minp[elt_sz], maxp[elt_sz];
    size_t rest;
    const char **np;
    int has_range_p = 0;

    assert (extend != 0);
    assert (elt_sz != 0);
    assert (buf_elts > 0);

    for (rest = names->size, np = names->s;
         rest > 0;
         rest--, np++) {
      char buf[buf_elts * elt_sz];
      FILE *fp;
      size_t count;

      if ((fp = open_file (*np, 1)) == 0) {
        error (1, errno, "%s", *np);
      }
      if (args.verbose_p) {
        if (fp == stdin)
          fputs (_("processing standard input...\n"), stderr);
        else
          fprintf (stderr, _("processing `%s'...\n"), *np);
      }
      while (! feof (fp)
             && (count = fread (buf, elt_sz, buf_elts, fp)) > 0) {
        void   *bp  = (has_range_p ? 0 : elt_sz) + buf;
        size_t size = (has_range_p ? 0 :     -1) + count;
        if (! has_range_p) {
          has_range_p = 1;
          COPY_ARY ((char *)minp, (char *)buf, elt_sz);
          COPY_ARY ((char *)maxp, (char *)buf, elt_sz);
        }
        /* NB: casting to `void *' */
        (*extend) ((void *)bp, size, (void *)minp, (void *)maxp);
      }
      /* FIXME: check for EOF? */
      close_file (fp);
    }

    /* print the result */
    #define PRINTF_RANGE_CAST(fs, dtype, type) \
        fprintf (output, "%" fs " %" fs "\n", \
                 (dtype)*((type *)minp), \
                 (dtype)*((type *)maxp))
    #define PRINTF_INT_RANGE_CAST(type) \
        PRINTF_RANGE_CAST ("lld", long long int, type)
    #define PRINTF_UINT_RANGE_CAST(type) \
        PRINTF_RANGE_CAST ("llu", unsigned long long int, type)
    #define PRINTF_DOUBLE_RANGE_CAST(type) \
        PRINTF_RANGE_CAST ("g", double, type)
    switch (t) {
    case FORMAT_INT8:   PRINTF_INT_RANGE_CAST   (int8_t);  break;
    case FORMAT_INT16:  PRINTF_INT_RANGE_CAST   (int16_t); break;
    case FORMAT_INT32:  PRINTF_INT_RANGE_CAST   (int32_t); break;
    case FORMAT_INT64:  PRINTF_INT_RANGE_CAST   (int64_t); break;
    case FORMAT_UINT8:  PRINTF_UINT_RANGE_CAST  (uint8_t);  break;
    case FORMAT_UINT16: PRINTF_UINT_RANGE_CAST  (uint16_t); break;
    case FORMAT_UINT32: PRINTF_UINT_RANGE_CAST  (uint32_t); break;
    case FORMAT_UINT64: PRINTF_UINT_RANGE_CAST  (uint64_t); break;
    case FORMAT_FLOAT:  PRINTF_DOUBLE_RANGE_CAST (float);  break;
    case FORMAT_DOUBLE: PRINTF_DOUBLE_RANGE_CAST (double); break;
    default:
      /* NB: shouldn't happen */
      assert (0);
    }
  }

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
/*** rawrange.c ends here */
