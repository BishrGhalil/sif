// TODOOO: Better argument handling
// TODOO: Better cleanup() when sigint
// TODO: Better syntax
#define _DEFAULT_SOURCE
#include <assert.h>
#include <errno.h>
#include <pcre.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "argparse.h"
#include "recdir.h"

#define VERSION "1.12"
#define PROGRAM "sif"

#define LINE_SIZE 512

#define BOLD "\e[1m"
#define BLUE "\e[34m"
#define RESET "\e[0m"
#define GREEN "\e[32m"

char *lineptr;
char *path;
char *line;
char *dir_path;
char *re_pattern;
pcre *re;

size_t line_size;

int version_flag = 0;
int matches_flag = 0;
int nfiles_flag = 0;
int lines_number_flag = 0;
int hidden_flag = 0;
int sigint_flag = 0;

int matches = 0;
int nfiles = 0;
int lines_number = 0;

static const char *const usage[] = {
    "sif [options] -s <re_pattern> -p <path>",
    NULL,
};

void cleanup() {
  if (re) {
    pcre_free(re);
  }
}

void printres(int matches_flag, int nfiles_flag, int lines_number_flag) {
  if (matches_flag) {
    printf("Matches: %d\t", matches);
  }
  if (nfiles_flag) {
    printf("Files: %d\t", nfiles);
  }
  if (lines_number_flag) {
    printf("Lines: %d\t", lines_number);
  }
  printf("\n");
}

void sighandler(int sig) {
  (void)sig;
  sigint_flag = 1;
  printres(matches_flag, nfiles_flag, lines_number_flag);
  cleanup();
  exit(0);
  return;
}

void exit_error(int errnum, int exit_status, char *msg) {
  if (msg && errnum != 0) {
    fprintf(stderr, "ERROR: %s, %s\n", msg, strerror(errnum));
  } else if (errnum != 0) {
    fprintf(stderr, "ERROR: %s\n", strerror(errnum));
  } else {
    fprintf(stderr, "ERROR\n");
  }

  printres(matches_flag, nfiles_flag, lines_number_flag);
  exit(exit_status);
}

// no white spaces; get rid of white spaces and tabe at the start of a string
const char *nws(const char *str) {
  while (str[0] == ' ' || str[0] == '\t') {
    str++;
  }
  return str;
}

int main(int argc, char **argv) {

  sigaction(SIGINT, &(struct sigaction){.sa_handler = sighandler}, NULL);

  const char *error;
  int erroffset;
  int rc;

  struct argparse_option options[] = {
      OPT_HELP(),
      OPT_GROUP("Arguments"),
      OPT_STRING('s', "search", &re_pattern, "Pattern to search for", NULL, 0,
                 0),
      OPT_STRING('p', "path", &dir_path,
                 "Directory path, Current directory is default", NULL, 0, 0),
      OPT_GROUP("Search options"),
      OPT_BOOLEAN('u', "hidden", &hidden_flag, "Search hidden folders", NULL, 0,
                  0),
      OPT_GROUP("Output options"),
      OPT_BOOLEAN('m', "matches", &matches_flag, "Matches number", NULL, 0, 0),
      OPT_BOOLEAN('l', "lines", &lines_number_flag, "Total searched lines",
                  NULL, 0, 0),
      OPT_BOOLEAN('f', "files", &nfiles_flag, "Total searched files", NULL, 0,
                  0),
      OPT_GROUP("Info options"),
      OPT_BOOLEAN('v', "version", &version_flag, "Version", NULL, 0, 0),
      OPT_END(),
  };

  struct argparse argparse;
  argparse_init(&argparse, options, usage, 2);
  argparse_describe(&argparse, "\nSearch for regex patterns inside files.",
                    "\nBishr Ghalil.");

  argc = argparse_parse(&argparse, argc, (void *)argv);

  if (version_flag) {
    printf("Version: %s\n", VERSION);
    exit(0);
  }

  if (!dir_path) {
    dir_path = strdup(".");
  }

  if (!re_pattern) {
    argparse_usage(&argparse);
    exit(1);
  }

  re = pcre_compile(re_pattern, 0, &error, &erroffset, NULL);

  if (re == NULL) {
    printf("REGEX compilation failed at \"%s\" : %s\n",
           re_pattern + (erroffset - 1), error);
    return 1;
  }

  RECDIR *recdir = recdir_open(dir_path);

  if (!recdir) {
    fprintf(
        stderr,
        "ERROR: Please use a valid path, Directory \"%s\" does not exist.\n",
        dir_path);
    exit(1);
  }

  errno = 0;
  struct dirent *ent = recdir_read(recdir, hidden_flag);
  FILE *file;
  while (ent && !sigint_flag) {
    path = join_path(recdir_top(recdir)->path, ent->d_name);
    if (access(path, W_OK) != 0) {
      exit_error(errno, 1, NULL);
    }
    // FIXME: unvalid read of 4 bytes
    file = fopen(path, "r");
    if (errno != 0) {
      printf("FILE: %s\n", path);
      exit_error(errno, 1, "Could not read the file");
    }
    nfiles++;
    int line_number = 0;
    int getline_res = 0;
    while (1) {
      lineptr = (char *)malloc(sizeof(char *) * LINE_SIZE);
      if (!lineptr) {
        exit_error(0, 1, "Can't allocate memory\n");
      }

      line_size = LINE_SIZE;
      getline_res = getline(&lineptr, &line_size, file);
      if (getline_res == -1) {
        if (lineptr) {
          free(lineptr);
        }
        if (path) {
          free(path);
        }
        break;
      }

      rc = pcre_exec(re, NULL, lineptr, strlen(lineptr), 0, 0, NULL, 0);
      lines_number++;
      line_number++;

      if (rc < 0) {
        free(lineptr);
        continue;
      }

      line = nws(lineptr);
      printf(BOLD BLUE "%s" RESET ":" GREEN "%d" RESET ": %s", path,
             line_number, line);
      matches++;
      if (lineptr) {
        free(lineptr);
      }
    }

    if (errno != 0) {
      cleanup();
      exit_error(errno, 1, path);
    }

    if (file) {
      fclose(file);
    }
    ent = recdir_read(recdir, hidden_flag);
  }

  if (errno != 0) {
    cleanup();
    exit_error(errno, 1, NULL);
  }

  recdir_close(recdir);
  printres(matches_flag, nfiles_flag, lines_number_flag);
  cleanup();
  return EXIT_SUCCESS;
}
