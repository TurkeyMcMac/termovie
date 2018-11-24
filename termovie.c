#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static struct movie {
	FILE *frames;
	char *delim;
	size_t frames_begin;
	int speed;
	bool looping;
} movie;

#define ERROR_ARGUMENT 1
#define ERROR_SYSTEM 2

bool print_frame_line(char **line, size_t *cap)
{
	if (getline(line, cap, movie.frames) > 0 && strcmp(*line, movie.delim))
	{
		printf("\e[K%s", *line);
		return true;
	} else {
		return false;
	}
}

bool print_next_frame(char **line, size_t *cap)
{
	int n_lines = 1;
	if (!print_frame_line(line, cap)) {
		return false;
	}
	while (print_frame_line(line, cap)) {
		++n_lines;
	}
	usleep(movie.speed);
	printf("\e[%dA\r", n_lines);
	return true;
}

void load_movie(int argc, char *argv[])
{
	char *prog_name = argv[0];
	char *movie_name = argv[1];
	if (!movie_name) {
		movie.frames = stdin;
	} else if (!(movie.frames = fopen(argv[1], "r"))) {
		fprintf(stderr, "%s: No movie named '%s' found.\n",
			prog_name, movie_name);
		exit(ERROR_ARGUMENT);
	}
	movie.delim = NULL;
	size_t cap = 0;
	if (getline(&movie.delim, &cap, movie.frames) < 0) {
		fprintf(stderr, "%s: %s\n", prog_name, strerror(errno));
		exit(ERROR_SYSTEM);
	}
	movie.frames_begin = ftell(movie.frames);
	movie.speed = 100000;
	movie.looping = true;
}

void play_movie(void)
{
	char *line = NULL;
	size_t cap = 0;
	do {
		while (print_next_frame(&line, &cap));
		fseek(movie.frames, movie.frames_begin, SEEK_SET);
	} while (movie.looping);
}

int main(int argc, char *argv[])
{
	load_movie(argc, argv);
	play_movie();
}
