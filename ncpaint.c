#include <curses.h>
#include <stdlib.h>

enum {
	ERR_INIT = 32,
	ERR_QUIT,
	ERR_MALLOC,
	ERR_MOUSE,
};

enum {
	BTN_LEFT = 1,
	BTN_MIDDLE = 2,
	BTN_RIGHT = 4,
};

// State struct
struct State {
	MEVENT* mevent;
	unsigned int loops_n;
	unsigned int chars_n;
	unsigned int mouse_n;
	int x;
	int y;
	int brush_ch;
	char btns_down;
	bool is_running;
};

void state_init(struct State* s, MEVENT* mevent);

int init(struct State*);
void loop(struct State*);
int main(void);
int quit(struct State*);

void state_init(struct State* s, MEVENT* mevent)
{
	s->mevent = mevent;
	s->loops_n = 0;
	s->chars_n = 0;
	s->mouse_n = 0;
	s->x = 0;
	s->y = 0;
	s->brush_ch = '#';
	s->btns_down = 0;
	s->is_running = TRUE;
}

int init()
{
	initscr();
	cbreak(); // Do not buffer characters but send them immediately
	noecho(); // Do not echo characters automatically
	keypad(stdscr, TRUE); // Enable special keys like cursor arrows, F-keys
	mouseinterval(0); // Do not wait for double/tripple clicks

	// Request all mouse events
	const mmask_t mouse_support = mousemask(
			ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, NULL);
	if (!mouse_support)
		return ERR_MOUSE;

	// Enable motion reporting (xterm extension)
	printf("\033[?1003h\n");

	// TODO first render

	return OK;
}

void loop(struct State* state)
{
	++state->loops_n;

	bool has_moved = FALSE;
	char btns_changed = 0;

	// Handle input and modify state accordingly
	int ch = getch();
	if (ch == KEY_MOUSE) {
		// Handle all queued mouse events
		while (getmouse(state->mevent) != ERR) {
			++state->mouse_n;

			if (!has_moved) {
				state->x = state->mevent->x;
				state->y = state->mevent->y;
				has_moved = TRUE;
			}

			if (!(btns_changed & BTN_LEFT)) {
				if (state->mevent->bstate & BUTTON1_PRESSED) {
					state->btns_down |= BTN_LEFT;
					btns_changed |= BTN_LEFT;
				} else if (state->mevent->bstate & BUTTON1_RELEASED) {
					state->btns_down &= ~BTN_LEFT;
					btns_changed |= BTN_LEFT;
				}
			}

			if (!(btns_changed & BTN_MIDDLE)) {
				if (state->mevent->bstate & BUTTON2_PRESSED) {
					state->btns_down |= BTN_MIDDLE;
					btns_changed |= BTN_MIDDLE;
				} else if (state->mevent->bstate & BUTTON2_RELEASED) {
					state->btns_down &= ~BTN_MIDDLE;
					btns_changed |= BTN_MIDDLE;
				}
			}

			if (!(btns_changed & BTN_RIGHT)) {
				if (state->mevent->bstate & BUTTON3_PRESSED) {
					state->btns_down |= BTN_RIGHT;
					btns_changed |= BTN_RIGHT;
				} else if (state->mevent->bstate & BUTTON3_RELEASED) {
					state->btns_down &= ~BTN_RIGHT;
					btns_changed |= BTN_RIGHT;
				}
			}
		}
	} else if (ch != ERR) { // Handle key press
		++state->chars_n;

		// Quit if 'q' is pressed
		if (ch == 'q')
			state->is_running = FALSE;

		state->brush_ch = ch;
	}

	// React to state changes
	if (state->btns_down & BTN_LEFT)
		mvaddch(state->y, state->x, state->brush_ch);
	else if (state->btns_down & BTN_RIGHT)
		mvaddch(state->y, state->x, ' ');

	mvprintw(LINES - 1, 0, "[%dx%d]:%d,%d | L: %d Ch: %d M: %d [0x%02x]",
			COLS, LINES, state->x, state->y, state->loops_n,
			state->chars_n, state->mouse_n, state->btns_down);
	clrtoeol();
	move(state->y, state->x);

	refresh();
}

int quit(struct State* state)
{
	int ret = endwin();
	if (ret != OK)
		return ERR_QUIT;

	printf("Ran for %u loops, processed %u keys and %u mouse events\n",
			state->loops_n, state->chars_n, state->mouse_n);

	return ret;
}

int main(void)
{
	MEVENT mevent;
	struct State state;
	state_init(&state, &mevent);
	init(&state);

	while(state.is_running)
		loop(&state);

	quit(&state);

	return 0;
}
