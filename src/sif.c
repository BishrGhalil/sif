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

char *lineptr;
int line_size;

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
    sigint_flag = 1;
    printres(matches_flag, nfiles_flag, nlines_flag);
    free(lineptr);
    exit(0);
    return;
}

void exit_error(int errnum, int exit_status, char *msg)
{
    if (msg && errnum != 0) {
	fprintf(stderr, "ERROR: %s, %s\n", msg, strerror(errnum));
    }
    else if (errnum != 0){
	fprintf(stderr, "ERROR: %s\n", strerror(errnum));
    } 
    else {
	fprintf(stderr, "ERROR\n");
    }

    printres(matches_flag, nfiles_flag, nlines_flag);
    exit(exit_status);
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
	OPT_STRING('s', "search", &regex_ptrn, "Pattern to search for"),
	OPT_STRING('p', "path", &dir_path, "Directory path"),
	OPT_GROUP("Search options"),
	OPT_BOOLEAN('u', "hidden", &hidden_flag, "Search hidden folders"),
	OPT_GROUP("Output options"),
	OPT_BOOLEAN('m', "matches", &matches_flag, "Matches number"),
	OPT_BOOLEAN('l', "lines", &nlines_flag, "Total searched lines"),
	OPT_BOOLEAN('f', "files", &nfiles_flag, "Total searched files"),
	OPT_GROUP("Info options"),
	OPT_BOOLEAN('v', "version", &version_flag, "Version"),
	OPT_END(),
};

    struct argparse argparse;
    argparse_init(&argparse, options, usage, 2);
    argparse_describe(&argparse,"\nSearch for regex patterns inside files.", "\nBishr Ghalil.");

    argc = argparse_parse(&argparse, argc, argv);

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
	    exit_error(errno, 1, NULL);
        }
        file = fopen(path, "r");
        if (errno != 0) {
	    printf("FILE: %s\n", path);
	    exit_error(errno, 1, "Could not read the file");
        }
        nfiles++;
        int i = 0;
        // FIXME
        lineptr = (char *) malloc(sizeof(char *) * LINE_SIZE);
        line_size = LINE_SIZE;
        int getline_res = 0;
        while (getline_res != -1) {
            getline_res = getline(&lineptr, &line_size, file);
            searchstat = regexec(&regex, lineptr, 0, NULL, 0);
            nlines++;
            if (searchstat == 0) {
                printf("FILE: %s\n", path);
                printf("(%d) %s\n", ++i, lineptr);
                matches ++;
            }
        }


        if (errno != 0) {
            free(lineptr);
	    exit_error(errno, 1, path);
        }
        fclose(file);
        ent = recdir_read(recdir, hidden_flag);
    }

    if (errno != 0) {
        free(lineptr);
	exit_error(errno, 1, NULL);
    }

    recdir_close(recdir);
    printres(matches_flag, nfiles_flag, nlines_flag);
    free(lineptr);
    return EXIT_SUCCESS;
}
