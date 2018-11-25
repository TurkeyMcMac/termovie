#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/time.h>

static struct movie {
	FILE *frames;
	char *tmp_path;
	char *delim;
	long frames_begin;
	int speed;
	bool looping;
} movie;

#define ERROR_ARGUMENT 1
#define ERROR_SYSTEM 2

void notify_alarm(int _)
{
	(void)_;
	// This just interrupts sleep.
}
static struct itimerval frame_alarm_setting;
void prepare_alarm(void)
{
	struct sigaction action;
	memset(&action, 0, sizeof(action));
	action.sa_handler = notify_alarm;
	action.sa_flags |= SA_RESTART;
	sigaction(SIGALRM, &action, NULL);
	div_t time = div(movie.speed, 1e6);
	frame_alarm_setting.it_interval.tv_sec = time.quot;
	frame_alarm_setting.it_interval.tv_usec = time.rem;
	frame_alarm_setting.it_value = frame_alarm_setting.it_interval;
}
void set_alarm(void) {
	setitimer(ITIMER_REAL, &frame_alarm_setting, NULL);
}
void wait_for_next_alarm(void)
{
	pause();
	set_alarm();
}

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
	fprintf(to, "termovie version 0.3.3\n");
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
	wait_for_next_alarm();
	return n_lines;
}

void clear_last_frame(int n_lines)
{
	// Clear the current line completely:
	printf("\r\e[K");
	while (n_lines--) {
		// Go to the line above and clear it:
		printf("\e[1A\e[K");
	}
}

void create_seekable_frames(void)
{
	static const char tmp_template[] = "/tmp/termovie-frames.XXXXXX";
	FILE *source = movie.frames;
	movie.tmp_path = malloc(sizeof(tmp_template));
	memcpy(movie.tmp_path, tmp_template, sizeof(tmp_template));
	movie.frames = fdopen(mkstemp(movie.tmp_path), "w+");
	char buf[BUFSIZ];
	size_t buflen;
	do {
		buflen = fread(buf, 1, sizeof(buf), source);
		fwrite(buf, 1, buflen, movie.frames);
	} while (buflen == sizeof(buf));
	fclose(source);
	rewind(movie.frames);
	movie.frames_begin = 0;
}

void load_movie(int argc, char *argv[])
{
	char *prog_name = argv[0];
	int opt;
	movie.speed = 100000;
	movie.looping = false;
	while ((opt = getopt(argc, argv, "f:lLvh")) != -1) {
		switch (opt) {
		case 'f': {
				double fps = atof(optarg);
				if (fps > 0 && fps <= 1e6) {
					movie.speed = 1e6 / fps;
				} else {
					fprintf(stderr, "%s: Invalid FPS\n",
						prog_name);
					print_advice(stderr);
					exit(ERROR_ARGUMENT);
				}
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
	if ((movie.frames_begin = ftell(movie.frames)) < 0 && movie.looping) {
		create_seekable_frames();
	}
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
	prepare_alarm();
	set_alarm();
	do {
		int n_lines;
		do {
			n_lines = print_next_frame(&line, &cap);
			clear_last_frame(n_lines);
		} while (n_lines > 0 && !terminated);
		fseek(movie.frames, movie.frames_begin, SEEK_SET);
	} while (movie.looping && !terminated);
	free(line);
}

void unload_movie(void)
{
	fclose(movie.frames);
	if (movie.tmp_path) {
		remove(movie.tmp_path);
		free(movie.tmp_path);
	}
	free(movie.delim);
}

int main(int argc, char *argv[])
{
	load_movie(argc, argv);
	play_movie();
	unload_movie();
}
