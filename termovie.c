#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

static struct movie {
	FILE *frames;
	char *delim;
	long frames_begin;
	int speed;
	bool looping;
} movie;

#define ERROR_ARGUMENT 1
#define ERROR_SYSTEM 2

void print_help(FILE *to)
{
	fprintf(to,
		"Usage: termovie [<options>] [<frames>]\n"
		"Options:\n"
		"  -f <fps>  Print <fps> frames per second.\n"
		"  -l        Loop frames until interrupted.\n"
		"  -L        Stop after the last frame (the default.)\n"
		"  -h        Print this help information.\n"
		"  -v        Print version information.\n"
		"\n"
		"If <frames> is not a file, stdin is used.\n"
	);
}

void print_version(FILE *to)
{
	fprintf(to, "termovie version 0.1.0\n");
}

void print_advice(FILE *to)
{
	fprintf(to, "Use the option -h for help information.\n");
}

bool print_frame_line(char **line, size_t *cap)
{
	if (getline(line, cap, movie.frames) > 0 && strcmp(*line, movie.delim))
	{
		printf("%s", *line);
		return true;
	} else {
		return false;
	}
}

int print_next_frame(char **line, size_t *cap)
{
	int n_lines = 1;
	if (!print_frame_line(line, cap)) {
		return 0;
	}
	while (print_frame_line(line, cap)) {
		++n_lines;
	}
	usleep(movie.speed);
	return n_lines;
}

void clear_last_frame(int n_lines)
{
	printf("\r\e[K");
	while (n_lines--) {
		printf("\e[1A\e[K");
	}
}

void load_movie(int argc, char *argv[])
{
	char *prog_name = argv[0];
	int opt;
	movie.speed = 100000;
	movie.looping = false;
	while ((opt = getopt(argc, argv, "f:lLvh")) != -1) {
		switch (opt) {
		case 'f':
			if (optarg) {
				int fps = atoi(optarg);
				if (fps > 0) {
					movie.speed = 1000000 / atoi(optarg);
				} else {
					fprintf(stderr, "%s: Invalid FPS\n",
						prog_name);
					print_advice(stderr);
					exit(ERROR_ARGUMENT);
				}
			} else {
				fprintf(stderr,
					"%s: No FPS provided with -f option.\n",
					prog_name);
				print_advice(stderr);
				exit(ERROR_ARGUMENT);
			}
			break;
		case 'l':
			movie.looping = true;
			break;
		case 'L':
			movie.looping = false;
			break;
		case 'h':
			print_help(stdout);
			exit(0);
		case 'v':
			print_version(stdout);
			exit(0);
		default:
			exit(ERROR_ARGUMENT);
		}
	}
	char *movie_name = argv[optind];
	if (!movie_name) {
		movie.frames = stdin;
	} else if (!(movie.frames = fopen(movie_name, "r"))) {
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
}

static sig_atomic_t terminated = false;
void notify_terminated(int _)
{
	(void)_;
	terminated = true;
}
void prepare_for_termination(void)
{
	struct sigaction action;
	memset(&action, 0, sizeof(action));
	action.sa_handler = notify_terminated;
	sigaction(SIGTERM, &action, NULL);
	sigaction(SIGINT, &action, NULL);
}

void play_movie(void)
{
	char *line = NULL;
	size_t cap = 0;
	prepare_for_termination();
	do {
		int n_lines;
		do {
			n_lines = print_next_frame(&line, &cap);
			clear_last_frame(n_lines);
		} while (n_lines > 0 && !terminated);
		fseek(movie.frames, movie.frames_begin, SEEK_SET);
	} while (movie.looping && !terminated);
}

int main(int argc, char *argv[])
{
	load_movie(argc, argv);
	play_movie();
}
