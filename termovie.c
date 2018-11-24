#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define ERROR_ARGUMENT 1
#define ERROR_SYSTEM 2

bool print_frame_line(char **line, size_t *cap, char *delim, FILE *movie)
{
	if (getline(line, cap, movie) > 0 && strcmp(*line, delim)) {
		printf("\e[K%s", *line);
		return true;
	} else {
		return false;
	}
}

bool print_next_frame(char **line, size_t *cap, char *delim, FILE *movie)
{
	int n_lines = 1;
	if (!print_frame_line(line, cap, delim, movie)) {
		return false;
	}
	while (print_frame_line(line, cap, delim, movie)) {
		++n_lines;
	}
	usleep(100000);
	printf("\e[%dA\r", n_lines);
	return true;
}

int main(int argc, char *argv[])
{
	FILE *movie;
	char *prog_name, *movie_name;
	char *delim, *line;
	size_t cap;
	long frames_begin;

	prog_name = argv[0];
	movie_name = argv[1];
	if (!movie_name) {
		fprintf(stderr, "%s: No movie name provided.\n", prog_name);
		exit(ERROR_ARGUMENT);
	}
	movie = fopen(argv[1], "r");
	if (!movie) {
		fprintf(stderr, "%s: No movie named '%s' found.\n",
			prog_name, movie_name);
		exit(ERROR_ARGUMENT);
	}
	delim = NULL;
	cap = 0;
	if (getline(&delim, &cap, movie) < 0) {
		fprintf(stderr, "%s: %s\n", prog_name, strerror(errno));
		exit(ERROR_SYSTEM);
	}
	frames_begin = ftell(movie);

	line = NULL;
	cap = 0;
	while (1) {
		while (print_next_frame(&line, &cap, delim, movie));
		fseek(movie, frames_begin, SEEK_SET);
	}
}
