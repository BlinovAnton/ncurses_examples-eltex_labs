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
    int number_of_files = 0, i, j, len;
    char symbol;
    system("ls > .fileshere");
    FILE *fileshere;
    fileshere = fopen (".fileshere", "r");
    if (fileshere == NULL) {
	printf(".fileshere wasn't opened\n");
	return 0;
    }
    while (!feof(fileshere)) {
	fscanf(fileshere, "%c", &symbol);
	if (symbol == 10) {
	    number_of_files++;
	}
    }
    fseek(fileshere, SEEK_SET, 0);
    number_of_files--; //one empty string
    char **files = malloc(number_of_files * sizeof(char*));
    int max_filename_size = 0;
    for (i = 0; i < number_of_files; i++) {
	files[i] = malloc(MAX_FILE_NAME * sizeof(char));
	fscanf(fileshere, "%s", files[i]);
	if (max_filename_size < strlen(files[i])) {
	    max_filename_size = strlen(files[i]);
	}
    } 
    system("rm .fileshere");

    WINDOW *blackhole;
    WINDOW *header, *header_border;
    WINDOW *flist, *flist_border;
    WINDOW *text, *text_border, *filename;
    initscr();
    signal(SIGWINCH, sig_winch);
    keypad(stdscr, 1);
    mousemask(BUTTON1_CLICKED, NULL);
    curs_set(0);
    start_color();
    refresh();
    init_pair(1, COLOR_YELLOW, COLOR_YELLOW);
    init_pair(2, COLOR_WHITE, COLOR_BLUE);
    init_pair(3, COLOR_BLACK, COLOR_CYAN);
    init_pair(4, COLOR_BLACK, COLOR_BLACK);
    init_pair(5, COLOR_BLACK, COLOR_GREEN);
    init_pair(6, COLOR_BLACK, COLOR_RED);
    
    int cols, rows;
    getscrsize (&rows, &cols);
    int header_border_height = 3;
    int header_border_width, num_of_spaces; 
    header_border_width = cols - 2;
    header_border = newwin(header_border_height, header_border_width, 1, 1);
    wattron (header_border, COLOR_PAIR(1));
    box (header_border, 32, 32);
    header = derwin(header_border, 1, header_border_width - 2, 1, 1);
    num_of_spaces = (header_border_width - 2 - 11) / 2; //11 = strlen("file editor"), 2 lefter and righter
    wbkgd (header, COLOR_PAIR(2));
    for (i = 0; i < num_of_spaces; i++) {
	wprintw (header, " "); 
    }
    wprintw (header, "FILE EDITOR");
    wrefresh (header);
    wrefresh (header_border);

    //int flist_border_height = 5 + number_of_files;
    int flist_border_height = rows - header_border_height - 1;
    int flist_border_width = max_filename_size + 6;
    flist_border = newwin(flist_border_height, flist_border_width, header_border_height, 1); 
    wattron (flist_border, COLOR_PAIR(1));
    box (flist_border, 32, 32);
    flist = derwin(flist_border, flist_border_height - 2, flist_border_width - 2, 1, 1);
    wbkgd (flist, COLOR_PAIR(2));
    wprintw(flist, "\n/..");
    for (i = 0; i < number_of_files; i++) {
	wprintw(flist, "\n %s", files[i]);
    }
    wrefresh (flist);
    wrefresh (flist_border);
    
    bool flag_text_created = 0;
    int text_border_height = rows - header_border_height - 1, text_border_width = cols - flist_border_width - 1;
    int filename_space_width, currentfile_fd, file_size, pos_for_new_data = 0;
    FILE * currentfile_fp;
    char *new_file_text;
    char *currentfile = NULL;
    ///////experiments with mouse
    while (wgetch(stdscr) == KEY_MOUSE) {
	MEVENT event;
	getmouse (&event);
	if (event.y >= 6 && event.y < 6 + number_of_files && event.x >= 2 && event.x <= 2 + 3 + max_filename_size) {
	    delwin (text_border);
 	    delwin (text); 
	    delwin (filename);
	    currentfile = files[event.y - 6];
	    len = strlen(files[event.y - 6]);
	    text_border = newwin (text_border_height, text_border_width, header_border_height, flist_border_width);
	    wattron (text_border, COLOR_PAIR(1));
	    box (text_border, 32, 32);
	    if (text_border_width - 2 < len + 7) { // 7 - strlen(" [*][X]")
		filename_space_width = len + 7;
	    } else {
		filename_space_width = text_border_width - 2;
	    }
	    filename = derwin (text_border, 1, filename_space_width, 1, 1);
	    wbkgd (filename, COLOR_PAIR(3));
	    wattron (filename, A_DIM);
	    wprintw(filename, "%s", files[event.y - 6]);
	    j = text_border_width - 2 - len - 6; // num_of_spaces for "filename [*][X]"
	    for (i = 0; i < j; i++) {
		wprintw(filename, "%c", 32);
	    }
	    wprintw(filename, "%c", '['); 
	    wprintw(filename, "%c", '*'); 
	    wprintw(filename, "%c", ']');
	    wprintw(filename, "%c", '[');
	    wprintw(filename, "%c", 'X');
	    wprintw(filename, "%c", ']'); 
	    
	    text = derwin (text_border, text_border_height - 3, text_border_width - 2, 2, 1);
	    wbkgd (text, COLOR_PAIR(2));
	    currentfile_fd = open (currentfile, O_RDWR);
	    currentfile_fp = fdopen (currentfile_fd, "rw");
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
		flag_text_created = 1;
	    }
	    wrefresh(filename);
	    wrefresh(text);
	    wrefresh(text_border);
	    
	} else {
	if (flag_text_created && event.y == (header_border_height + 1) && event.x >= (flist_border_width + 1) && event.x < header_border_width) {
	    move (rows - 1, cols / 3);
	    printw ("Mouse clicked at %i-%i\n", event.x, event.y);
	    move (rows - 1, 3 * (cols / 4));
	    printw ("in filename %s\n", currentfile);
	    if (event.x == flist_border_width + filename_space_width - 1) { // click on X at [X]
		delwin (text_border);
	        delwin (text); 
		delwin (filename);
		close (currentfile_fd);
		currentfile = NULL;
		flag_text_created = 0;
		blackhole = newwin(text_border_height - 1, text_border_width - 1, header_border_height + 1, flist_border_width + 1);
		wattron (blackhole, COLOR_PAIR(4));
		wrefresh (blackhole);
		delwin (blackhole);
		refresh ();
	    }
	    if (event.x == flist_border_width + filename_space_width - 4) { // click on * at [*]
		//save: green - saved, red - not saved
		close(currentfile_fd);
		currentfile_fd = open(currentfile, O_RDWR | O_TRUNC);
		currentfile_fp = fdopen (currentfile_fd, "rw");
		if (currentfile_fd == -1) {
		    mvaddch(header_border_height + 1, flist_border_width + filename_space_width - 4, '*' | COLOR_PAIR(6));
		    refresh();
		    system("sleep 1");
		    mvaddch(header_border_height + 1, flist_border_width + filename_space_width - 4, '*' | COLOR_PAIR(3));
		    refresh();
		} else {
		    write(currentfile_fd, new_file_text, file_size);
		    mvaddch(header_border_height + 1, flist_border_width + filename_space_width - 4, '*' | COLOR_PAIR(5));
		    refresh();
		    system("sleep 1");
		    mvaddch(header_border_height + 1, flist_border_width + filename_space_width - 4, '*' | COLOR_PAIR(3));
		    /*event.x = 0; event.y = 0;
		    move (event.y, event.x);*/
		    refresh();
		}
	    }
	} else {
	    if (flag_text_created && event.y >= (header_border_height + 2) && event.y < (header_border_height + text_border_height - 1) && event.x >= (flist_border_width + 1) && event.x < header_border_width) {
		move (rows - 1, cols / 3);
	        printw ("Mouse clicked at %i-%i\n", event.x, event.y);
	        move (rows - 1, 3 * (cols / 4));
	        printw ("in file %s\n", currentfile);
		//next need realise edition
		
		
	    } else {
		move (rows - 1, cols / 3);
	        printw ("Mouse clicked at %i-%i\n", event.x, event.y);	    
		refresh ();
	    }
	}
	}
	refresh ();
	move (event.y, event.x);
    }
    ////// 

    delwin (header_border);
    delwin (header);
    delwin (flist_border);  
    delwin (flist);
    delwin (text_border);
    delwin (text);
    move (rows - 1, 1);
    printw ("Press any key to continue...");
    refresh ();
    getch ();
    endwin ();
    exit (EXIT_SUCCESS);

    return 0;
}