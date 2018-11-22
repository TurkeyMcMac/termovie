#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <ncurses.h>

sig_atomic_t terminated = false;
void handle_termination(int _)
{
	(void)_;
	terminated = true;
}
void prepare_terminator(void)
{
	struct sigaction action;
	memset(&action, 0, sizeof(action));
	action.sa_handler = handle_termination;
	sigaction(SIGINT, &action, NULL);
}

bool print_next_frame(char **line, size_t *cap, char *delim, FILE *movie)
{
	if (getline(line, cap, movie) > 0 && strcmp(*line, delim)) {
		erase();
		printw(*line);
	} else {
		return false;
	}
	while (getline(line, cap, movie) > 0 && strcmp(*line, delim)) {
		printw(*line);
	}
	usleep(100000);
	refresh();
	return true;
}

int main(int argc, char *argv[])
{
	FILE *movie;
	char *delim, *line;
	size_t cap;
	long frames_begin;

	prepare_terminator();

	movie = fopen(argv[1], "r");
	delim = NULL;
	cap = 0;
	getline(&delim, &cap, movie);
	frames_begin = ftell(movie);

	initscr();

	line = NULL;
	cap = 0;
	while (!terminated) {
		while (print_next_frame(&line, &cap, delim, movie)
			&& !terminated);
		fseek(movie, frames_begin, SEEK_SET);
	}

	endwin();
}
