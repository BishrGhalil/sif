// TODO: more flags arguments
#define _DEFAULT_SOURCE
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <regex.h>
#include <unistd.h>
#include <signal.h>

#include "recdir.h"
#include "argparse.h"

#define VERSION "1.0"
#define LINE_SIZE 512

int version_flag = 0;
int matches_flag = 0;
int nfiles_flag = 0;
int nlines_flag = 0;
int hidden_flag = 0;
int sigint_flag = 0;

int matches = 0;
int nfiles = 0;
int nlines = 0;

static const char *const usage[] = {
    "sif [options] -s <regex_pattern> -p <path>",
    NULL,
};

void printres(int matches_flag, int nfiles_flag, int nlines_flag)
{
    printf("\n");
    if (matches_flag) {
	printf("Matches: %d\t", matches);
    }
    if (nfiles_flag) {
	printf("Files: %d\t", nfiles);
    }
    if (nlines_flag) {
	printf("Lines: %d\t", nlines);
    }
    printf("\n");
}

void sighandler(int sig) {
    (void) sig;
    sigint_flag = 1;
    printres(matches_flag, nfiles_flag, nlines_flag);
    exit(0);
    return;
}

int main(int argc, char **argv)
{

    sigaction(SIGINT, &(struct sigaction){ .sa_handler = sighandler }, NULL);
    int compstat = 0,
	searchstat = 0;

    const char *dir_path = NULL;
    const char *regex_ptrn = NULL;
    regex_t regex;

    struct argparse_option options[]= {
	OPT_HELP(),
	OPT_GROUP("Arguments"),
	OPT_STRING('s', "search", &regex_ptrn, "Pattern to search for", NULL, 0, 0),
	OPT_STRING('p', "path", &dir_path, "Directory path", NULL, 0, 0),
	OPT_GROUP("Search options"),
	OPT_BOOLEAN('u', "hidden", &hidden_flag, "Search hidden folders", NULL, 0, 0),
	OPT_GROUP("Output options"),
	OPT_BOOLEAN('m', "matches", &matches_flag, "Matches number", NULL, 0, 0),
	OPT_BOOLEAN('l', "lines", &nlines_flag, "Total searched lines", NULL, 0, 0),
	OPT_BOOLEAN('f', "files", &nfiles_flag, "Total searched files", NULL, 0, 0),
	OPT_GROUP("Info options"),
	OPT_BOOLEAN('v', "version", &version_flag, "Version", NULL, 0, 0),
	OPT_END(),
    };

    struct argparse argparse;
    argparse_init(&argparse, options, usage, 2);
    argparse_describe(&argparse,"\nSearch for regex patterns inside files.", "\nBishr Ghalil.");

    argc = argparse_parse(&argparse, argc,(void *) argv);

    if (version_flag) {
	printf("Version: %s\n", VERSION);
	exit(0);
    }

    if (!dir_path) {
	argparse_usage(&argparse);
	exit(1);
    }
    if (!regex_ptrn) {
	argparse_usage(&argparse);
	exit(1);
    }
    compstat = regcomp(&regex, regex_ptrn, 0);
    if (compstat != 0) {
	fprintf(stderr, "REGEX pattern can't be compiled.\n");
	exit(1);
    }

    RECDIR *recdir = recdir_open(dir_path);
    if (!recdir) {
	fprintf(stderr, "ERROR: Please use a valid path, Directory \"%s\" does not exist.\n", dir_path);
	exit(1);
    }

    errno = 0;
    struct dirent *ent = recdir_read(recdir, hidden_flag);
    FILE *file;
    while (ent && !sigint_flag) {
	char *path = join_path(recdir_top(recdir)->path, ent->d_name);
	if (access(path, W_OK) != 0) {
	    fprintf(stderr, "%s.\n", strerror(errno));
	    exit(1);
	}
	file = fopen(path, "r");
	if (errno != 0) {
	    fprintf(stderr, "ERROR: Could not read the file: %s, %s\n", path, strerror(errno));
	    exit(1);
	}
	nfiles++;
	int i = 0;
	int getline_res = 0;
	while (getline_res != -1) {
	    char *lineptr = (char *) malloc(sizeof(char *) * LINE_SIZE);
	    int line_size = LINE_SIZE;
	    getline_res = getline(&lineptr, &line_size, file);
	    searchstat = regexec(&regex, lineptr, 0, NULL, 0);
	    nlines++;
	    if (searchstat == 0) {
		printf("FILE: %s\n", path);
		printf("(%d) %s\n", ++i, lineptr);
		matches ++;
	    }
	    free(lineptr);
	}


	if (errno != 0) {
	    fprintf(stderr, "ERROR: %s\n", strerror(errno));
	    exit(1);
	}
	fclose(file);
	ent = recdir_read(recdir, hidden_flag);
    }

    if (errno != 0) {
	fprintf(stderr, "ERROR: could not read the directory: %s, %s\n", recdir_top(recdir)->path, strerror(errno));
	exit(1);
    }

    recdir_close(recdir);
    printres(matches_flag, nfiles_flag, nlines_flag);
    return EXIT_SUCCESS;
}
