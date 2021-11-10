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

#include "recdir.h"

typedef struct linked_list {
    int list_size;
    int line_size;
    char *line;
    struct linked_list *next;
} list;

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

    char *dir_path;
    char regex_ptrn[512];
    int matches = 0;
    regex_t regex;
    int compstat, searchstat;

    switch (argc) {
	case 2:
	    dir_path = (char *) malloc(sizeof(char) * 2);
	    strcpy(dir_path, ".\0");
	    if (strlen(argv[1]) > 512) {
		fprintf(stderr, "Don't use more than 512 regex character.\n");
		exit(1);
	    } 
	    strcpy(regex_ptrn, argv[1]);
	    break;
	case 3:
	    if (strlen(argv[1]) > 512) {
		fprintf(stderr, "Don't use more than 512 regex character.\n");
		exit(1);
	    } 
	    strcpy(regex_ptrn, argv[1]);
	    dir_path = strdup(argv[2]);
	    break;
	default:
	    fprintf(stderr, "Usage: sif <regex pattern> <path>\n");
	    exit(1);
    }

    compstat = regcomp(&regex, regex_ptrn, 0);
    if (compstat != 0) {
	fprintf(stderr, "REGEX pattern can't be compiled.\n");
	exit(1);
    }

    RECDIR *recdir = recdir_open(dir_path);

    errno = 0;
    struct dirent *ent = recdir_read(recdir);
    FILE *file;
    while (ent) {
	char *path = join_path(recdir_top(recdir)->path, ent->d_name);
	if (access(path, W_OK) != 0) {
	    fprintf(stderr, "%s.\n", strerror(errno));
	    exit(1);
	}
	file = fopen(path, "r");
	list *lines = fgetlines(file);
	while (lines != NULL) {
	    searchstat = regexec(&regex, lines->line, 0, NULL, 0);
	    if (searchstat == 0) {
		printf("FILE: %s\n", path);
		printf("%s\n", lines->line);
		matches ++;
	    }
	    lines = lines->next;
	}
	

	free(lines);
	fclose(file);
	ent = recdir_read(recdir);
    }

    if (errno != 0) {
	fprintf(stderr, "ERROR: could not read the directory: %s, %s\n", recdir_top(recdir)->path, strerror(errno));
	exit(1);
    }

    recdir_close(recdir);
    printf("Matches %d\n", matches);

    return EXIT_SUCCESS;
}
