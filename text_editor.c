#include <termios.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <stdlib.h>
#include <curses.h>
#include <fcntl.h>

#define MAX_FILE_NAME 256

void sig_winch (int signo) {
    struct winsize size;
    ioctl (fileno(stdout), TIOCGWINSZ, (char *) &size);
    resizeterm(size.ws_row, size.ws_col);
    nodelay(stdscr, 1);
    while (wgetch(stdscr) != ERR);
    nodelay(stdscr, 0);
}

int getscrsize (int *rows, int *cols) {
    int termfile;
    struct winsize scrsize;
    termfile = open("/dev/tty", O_RDWR);
    if (termfile == -1) {
	printf("Can't open terminal file");
	close(termfile);
	return -1;
    } else {
	if (!ioctl(termfile, TIOCGWINSZ, &scrsize)) {
    	    *rows = scrsize.ws_row;
    	    *cols = scrsize.ws_col;
    	    close(termfile);
    	    return 0;
	} else {
	    printf("Can't get screensize\n");
	    close(termfile);
	    return -1;
	}
    }
}

int main () {
    int i;
    char symbol;

    WINDOW *text, *border;
    initscr();
    signal(SIGWINCH, sig_winch);
    keypad(stdscr, 1);
    mousemask(BUTTON1_CLICKED, NULL);
    curs_set(0);
    start_color();
    refresh();
    init_pair(1, COLOR_CYAN, COLOR_CYAN);
    init_pair(2, COLOR_WHITE, COLOR_BLUE);
    init_pair(3, COLOR_BLACK, COLOR_CYAN);
    init_pair(4, COLOR_BLACK, COLOR_BLACK);
    init_pair(5, COLOR_BLACK, COLOR_GREEN);
    init_pair(6, COLOR_BLACK, COLOR_RED);
    int cols, rows;
    getscrsize (&rows, &cols);

    int border_start_y = 0, border_start_x = 0;
    int border_height = rows - border_start_y - 1, border_width = cols - border_start_x;
    int save_y = border_start_y, save_x = border_start_x + border_width - 6, close_x = border_start_x + border_width - 3, close_y = save_y;
    int currentfile_fd, file_size;
    FILE * currentfile_fp;
    char *new_file_text;
    char *currentfile = NULL;

    currentfile = "file2";
    border = newwin(border_height, border_width, border_start_y, border_start_x);
    wattron(border, COLOR_PAIR(3));
    box(border, 32, 32);
    wprintw(border, " %s", currentfile);

    mvwprintw(border, border_start_y, border_start_x + border_width - 7, "%c", '[');
    mvwprintw(border, close_y, save_x, "%c", '*');
    mvwprintw(border, border_start_y, border_start_x + border_width - 5, "%c", ']');
    mvwprintw(border, border_start_y, border_start_x + border_width - 4, "%c", '[');
    mvwprintw(border, close_y, close_x, "%c", 'X');
    mvwprintw(border, border_start_y, border_start_x + border_width - 2, "%c", ']');
    mvwprintw(border, border_start_y, border_start_x + border_width - 1, "%c", ' ');

    int text_height = border_height - 2, text_width = border_width - 2;
    int text_start_y = border_start_y + 1, text_start_x = border_start_x + 1;
    text = derwin(border, text_height, text_width, text_start_y, text_start_x);
    wbkgd(text, COLOR_PAIR(2));
    mvwprintw(border, border_start_y + border_height - 1, border_start_x, "%c", 32);
    mvwprintw(border, border_start_y + border_height - 1, border_start_x + border_width - 1, "%c", 32);
    currentfile_fd = open(currentfile, O_RDWR);
    currentfile_fp = fdopen(currentfile_fd, "rw");
    fseek(currentfile_fp, 0, SEEK_END);
    file_size = ftell(currentfile_fp);
    fseek(currentfile_fp, 0, SEEK_SET);
    new_file_text = malloc(file_size * sizeof(char));
    if (currentfile_fd == -1) {
	wprintw(text, "\n\n\t\"%s\" can't be opened\n", currentfile);
	close(currentfile_fd);
    } else {
	i = 0;
	while (!feof(currentfile_fp)) {
	    fscanf(currentfile_fp, "%c", &symbol);
	    wprintw(text, "%c", symbol);
	    new_file_text[i] = symbol;
	    i++;
	}
    }
    wrefresh(text);
    wrefresh(border); 

    //init for edit
    int text_current_x = text_start_x, text_current_y = text_start_y, pos_in_new_file = 0;
    bool stop_write_flag;

    while (1) {
    ///////experiments with mouse
    while (wgetch(stdscr) == KEY_MOUSE) {
	MEVENT event;
	getmouse (&event);
	if (event.y == close_y) {
	    move (rows - 1, 0);
	    printw ("in filename %s\n", currentfile);
	    if (event.x == close_x) { // click on X at [X]
		delwin (border);
	        delwin (text); 
		close (currentfile_fd);
		currentfile = NULL;
		refresh ();
		endwin();
	        exit(EXIT_SUCCESS);
		return 0;
	    }
	    if (event.x == save_x) { // click on * at [*]
		//save: green - saved, red - not saved
		close(currentfile_fd);
		currentfile_fd = open(currentfile, O_RDWR | O_TRUNC);
		currentfile_fp = fdopen (currentfile_fd, "rw");
		if (currentfile_fd == -1) {
		    mvaddch(close_y, save_x, '*' | COLOR_PAIR(6));
		    refresh();
		    system("sleep 1");
		    mvaddch(close_y, save_x, '*' | COLOR_PAIR(3));
		    refresh();
		} else {
		    write(currentfile_fd, new_file_text, file_size);
		    mvaddch(close_y, save_x, '*' | COLOR_PAIR(5));
		    refresh();
		    system("sleep 1");
		    mvaddch(close_y, save_x, '*' | COLOR_PAIR(3));
		    refresh();
		}
	    }
	}
	if (event.x >= (border_start_x + 1) && event.x < (border_start_x + border_width - 1) && event.y >= (border_start_y + 1) && event.y < (border_start_y + border_height - 1)) {
	    move(rows - 1, 0);
	    stop_write_flag = 0;
	    printw("in file %s\n", currentfile);
	    noecho();
	    do {
		symbol = getch();
		switch (symbol) {
		    case 27:
			stop_write_flag = 1;
			break;
		    case 7:
			//backspace
			break;
		    case 10:
			//enter
			new_file_text[pos_in_new_file] = 10;
			pos_in_new_file++;
			text_current_y++;
			text_current_x = text_start_x;
			break;
		    case -103:
			//mouse - mean exit
			stop_write_flag = 1;
			break;
		    default:
			mvaddch(text_current_y, text_current_x, symbol | COLOR_PAIR(2));
			text_current_x++;
			new_file_text[pos_in_new_file] = symbol;
			pos_in_new_file++;
			break;
		}
		refresh();
		//printw("%d ", symbol);
	    } while (!stop_write_flag);
	}
	move(rows - 1, cols / 4);
	printw("Mouse clicked at %i-%i\n", event.x, event.y); 
	refresh();
	move(event.y, event.x);
    }
    }
    ///////
    //return 0;
}