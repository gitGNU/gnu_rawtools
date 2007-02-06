/*** rawilv.c --- Interleave parts of the files  -*- C -*- */
#define _GNU_SOURCE
#include "config.h"
#include "gettext.h"
#define _(string) gettext (string)
#define N_(string) gettext_noop (string)
static const char doc[] = N_("Interleave parts of the files");
static const char args_doc[]
= "[INPUT-FILE]...\n-i INPUT-FILE [OUTPUT-FILE]...";

/*** Copyright (C) 2007 Ivan Shmakov */

/*** Code: */

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
#include <stdio.h>
#include <stdlib.h>

#include "p_arg.h"
#include "usemacro.h"
#include "useutil.h"

#define BUF_SZ 4096

/*** Utility */

static const char *
skip_space (const char *s)
{
  while (*s != '\0' && isspace (*s)) {
    s++;
  }

  /* . */
  return s;
}

static int
comma_sep_sizes (const char *s, off_t **buf,
                 size_t *allocptr, size_t *size)
{
  int count;
  const char *p;
  const char delim = ',';

  for (p = s, count = 0; *p != '\0'; count++) {
    char *t;
    long v;
    if ((v = strtol (p, &t, 0)),
        t == s
        || (t = skip_space (t),
            *t != '\0' && *t != delim)
        || v <= 0) {
      errno = EINVAL;
      /* . */
      return -1;
    }
    p = (*t == delim ? t + 1 : t);

    if (ensure_enough_space ((void **)buf, allocptr,
                             sizeof (**buf), (*size) + 1)
        != 0) {
      /* . */
      return -1;
    }
    (*buf)[(*size)++] = v;
  }

  /* . */
  return count;
}

static void
close_files (FILE **files, size_t count)
{
  size_t rest;
  FILE **fp;
  for (rest = count, fp = files; rest > 0; rest--, fp++) {
    close_file (*fp);
  }
}

/*** Parsing the Command Line */

const char *
program_version (void)
{
  return
    "rawilv " PACKAGE_NAME " " PACKAGE_VERSION;
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
  { "block-sizes",      's', "NUMBERS", 0,
    N_("Interleave blocks of given sizes, separated by commas") },
  { "blocks",             0, 0, OPTION_ALIAS },
  { "input",            'i', "FILE", 0,
    N_("select a de-interleave mode on the given input file") },
  { "output",           'o', "FILE", 0,
    N_("output the result to this file instead of stdout") },
  { "verbose",          'v', 0, 0,
    N_("explain what is being done") },
  { 0 }
};

struct p_args {
  int verbose_p;
  off_t *block_sizes;
  size_t block_sizes_size, block_sizes_alloc;
  const char *input_file;
  const char *output_file;
  struct strings noa_files;     /* non-option arguments */
};

static error_t
p_opt (int key, char *arg, struct argp_state *state)
{
  struct p_args *args = state->input;

  /* FIXME: looks ugly! */

  switch (key) {
  case 's':
    if (comma_sep_sizes (arg, &(args->block_sizes),
                         &(args->block_sizes_alloc),
                         &(args->block_sizes_size))
        > 0) {
      /* do nothing */
    } else if (errno == EINVAL) {
      argp_error (state,
                  N_("invalid argument `%s';"
                     " should be `NUMBER[, NUMBER]...'"),
                  arg);
      /* . */
      return EINVAL;
    } else {
      argp_failure (state, 0, errno,
                    N_("couldn't store block sizes `%s'"), arg);
      /* . */
      return errno;
    }
    break;
  case 'o':
    if (args->input_file) {
      argp_error (state,
                  N_("--output cannot be used in de-interleave mode"));
      /* . */
      return errno;
    }
    args->output_file = arg;
    break;
  case 'i':
    if (args->output_file) {
      argp_error (state,
                  N_("--input cannot be used in interleave mode"));
      /* . */
      return errno;
    }
    args->input_file = arg;
    break;
  case 'v':
    args->verbose_p = 1;
    break;
  case ARGP_KEY_ARGS:
    if (strings_append (&(args->noa_files),
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
      if (strings_append (&(args->noa_files), s, 1) < 0) {
        argp_failure (state, 0, errno, 
                      N_("couldn't set stdin as the input file"));
        /* . */
        return errno;
      }
    }
    break;
  case ARGP_KEY_END:
    {
      const size_t
        sizes = args->block_sizes_size,
        files = args->noa_files.size;
      if (sizes == files) {
        /* do nothing */
      } else if (sizes > files) {
        argp_error (state,
                    N_("extra block sizes given in the command line"));
        return EINVAL;
      } else if (ensure_enough_space ((void **)&(args->block_sizes),
                                      &(args->block_sizes_alloc),
                                      sizeof (*(args->block_sizes)),
                                      files) != 0) {
        argp_failure (state, 0, errno, 
                      N_("couldn't store block sizes"));
        /* . */
        return errno;
      } else {
        const off_t value = (sizes < 1 ? 1
                             : args->block_sizes[sizes - 1]);
        size_t rest;
        off_t *p;
        for (rest = files - sizes, p = args->block_sizes + sizes;
             rest > 0;
             rest--, *(p++) = value)
          ;
        args->block_sizes_size = files;
      }
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
    .block_sizes = 0,
    .block_sizes_size  = 0,
    .block_sizes_alloc = 0,
    .input_file  = 0,
    .output_file = 0,
    .noa_files   = { 0, 0, 0 }
  };
  int interleave_p;
  FILE *the_file;

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

  {
    const char *fn;

    /* determine mode */
    interleave_p = (args.input_file == 0);
    fn = (interleave_p ? args.output_file : args.input_file);
    if (fn == 0) { fn = "-"; }

    /* open the [-i|-o] file */
    if ((the_file = open_file (fn, 0)) == 0) {
      error (1, errno, "%s", fn);
    }
  }

  {
    const struct strings *names = &(args.noa_files);
    const size_t noas_count = names->size;
    FILE *noas[noas_count];

    /* open the files */
    {
      const int input_p = interleave_p;
      size_t rest;
      const char **np;
      FILE **fp;
      for (rest = noas_count, np = names->s, fp = noas;
           rest > 0;
           rest--, np++, fp++) {
        if ((*fp = open_file (*np, input_p)) == 0) {
          error (1, errno, "%s", *np);
        }
      }
    }

    /* process the data */
    if (interleave_p) {
      const off_t *sizes = args.block_sizes;
      char buf[BUF_SZ];
      char *bp;
      int done_p, eof_p;
      size_t rest_files;
      off_t rest_bytes;
      FILE **fp;
      const off_t *sp;
      const char **np;

      assert (args.block_sizes_size == noas_count);

      for (done_p = eof_p = 0,
             bp = buf,
             np = names->s,
             fp = noas,  rest_files = noas_count,
             sp = sizes, rest_bytes = *sp;
           ! done_p; ) {
        if (rest_bytes > 0) {
          /* do nothing */
        } else if (fp++, sp++, np++, --rest_files > 0) {
          /* do nothing */
        } else if (eof_p) {
          done_p = 1;
          continue;
        } else {
          np = names->s;
          fp = noas,  rest_files = noas_count;
          sp = sizes, rest_bytes = *sp;
        }
        {
          size_t rv;
          const size_t buf_size = bp - buf;
          size_t to_read = MIN (*sp, sizeof (buf) - buf_size);

          assert (to_read > 0);
          if ((rv = fread (buf + buf_size, 1, to_read, *fp),
               bp += rv, rest_bytes -= rv, rv)
              == to_read) {
            /* do nothing */
          } else if (! feof (*fp)) {
            /* reading error */
            error (1, errno, "%s", *np);
          } else if (rv != 0 && rest_bytes > 0) {
            error (1, 0, _("%s: EOF in the middle of the block"),
                   *np);
          } else {
            eof_p = 1;
            rest_bytes = 0;
            continue;
          }
          if (bp - buf < sizeof (buf)) {
            /* do nothing */
          } else if ((fwrite (buf, 1, sizeof (buf), the_file))
                     != sizeof (buf)) {
            error (1, errno, "%s", args.output_file);
          }
        }
      }

      if (bp <= buf) {
        /* do nothing */
      } else if ((fwrite (buf, 1, (bp - buf), the_file))
                 != (bp - buf)) {
        error (1, errno, "%s", args.output_file);
      }

      /* check if every input file is at EOF */
      {
        size_t rest;
        const char **np;
        FILE **fp;
        int neofs;
        const char *last = 0;
        for (rest = noas_count, np = names->s, fp = noas, neofs = 0;
             rest > 0;
             rest--, np++, fp++) {
          if (! feof (*fp)) {
            error (0, 0, _("Warning: `%s' is not at EOF"), *np);
            neofs++;
          } else {
            last = *np;
          }
        }
        if (neofs == 1) {
          assert (last != 0);
          error (0, 0, _("Warning: `%s' ended prematurely"), last);
        }
        if (neofs) {
          close_files (noas, noas_count);
          error (1, 0, _("premature EOF in some of the inputs"));
        }
      }
    } else {
      char buf[BUF_SZ];

      /* FIXME: write it! */
      error (1, 0, _("de-interleave mode is not implemented"));
    }

    /* close the files */
    close_files (noas, noas_count);
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
/*** rawilv.c ends here */