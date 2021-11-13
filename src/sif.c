// TODO:
// 	 -1 free allocations and check for NULL allocations
// 	 -2 more flags arguments
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

typedef struct linked_list {
    int list_size;
    int line_size;
    char *line;
    struct linked_list *next;
} list;


void printres(int matches_flag, int nfiles_flag, int nlines_flag)
{
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
    exit(0);
    return;
}

int lappend(list *l, char *line)
{
    list *tmp = l;
    if (tmp->list_size == 0) {
	tmp->line = strdup(line);
	tmp->line_size = strlen(line);
	tmp->next = NULL;
	tmp->list_size = 1;
	return 1;
    }

    while (tmp->next) {
	tmp = tmp->next;
    }

    tmp->next = (list *) malloc(sizeof(list));
    if (!tmp->next) {
	fprintf(stderr, "Could not allocate more memory.\n");
	return 0;
    }

    tmp->next->line = strdup(line);
    tmp->next->line_size = strlen(line);
    tmp->next->next = NULL;
    l->list_size += 1;
    return 1;
}

list *fgetlines(FILE *file)
{
    int line_size = 512;
    char ch;
    char *line = (char *) malloc(sizeof(char) * line_size);
    list *lines = (list *) malloc(sizeof(list));
    lines->list_size = 0;
    int i = 0;
    while(1) {
	ch = fgetc(file);
	if (ch == EOF) {
	    line[i] = '\0';
	    lappend(lines, line);
	    free(line);
	    return lines;
	}
	if (i >= line_size) {
	    line_size *= 2;
	    line = realloc(line, line_size);
       }

	line[i++] = ch;
	if (ch == '\n') {
	    line[i] = '\0';
	    lappend(lines, line);
	    i = 0;
	}
   }
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
	    fprintf(stderr, "%s.\n", strerror(errno));
	    exit(1);
	}
	file = fopen(path, "r");
	nfiles++;
	list *lines = fgetlines(file);
	int i = 0;
	while (lines != NULL && !sigint_flag) {
	    searchstat = regexec(&regex, lines->line, 0, NULL, 0);
	    nlines++;
	    if (searchstat == 0) {
		printf("FILE: %s\n", path);
		printf("(%d) %s\n", ++i, lines->line);
		matches ++;
	    }
	    lines = lines->next;
	}
	

	free(lines);
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
