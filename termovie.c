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
	FILE *movie = fopen(argv[1], "r");
	char *delim = NULL;
	size_t cap = 0;
	getline(&delim, &cap, movie);
	long frames_begin = ftell(movie);
	prepare_terminator();
	initscr();
	char *line = NULL;
	cap = 0;
	while (!terminated) {
		while (!terminated
			&& print_next_frame(&line, &cap, delim, movie));
		fseek(movie, frames_begin, SEEK_SET);
	}
	endwin();
	return 0;
}
