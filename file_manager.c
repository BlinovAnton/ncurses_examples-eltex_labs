#include <termios.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <stdlib.h>
#include <curses.h>
#include <fcntl.h>
#include <dirent.h>

#define MAX_FILE_NAME 256

void sig_winch (int signo) {
    struct winsize size;
    ioctl (fileno(stdout), TIOCGWINSZ, (char *) &size);
    resizeterm (size.ws_row, size.ws_col);
    nodelay (stdscr, 1);
    while (wgetch(stdscr) != ERR);
    nodelay (stdscr, 0);
}

int getscrsize (int *rows, int *cols) {
    int termfile;
    struct winsize scrsize;
    termfile = open ("/dev/tty", O_RDWR);
    if (termfile == -1) {
	printf ("Can't open terminal file");
	close (termfile);
	return -1;
    } else {
	if (!ioctl(termfile, TIOCGWINSZ, &scrsize)) {
    	    *rows = scrsize.ws_row;
    	    *cols = scrsize.ws_col;
    	    close (termfile);
    	    return 0;
	} else {
	    printf ("Can't get screensize\n");
	    close (termfile);
	    return -1;
	}
    }
}

int main () {
    FILE* fp;
    int num_of_spaces, i, num_of_files = 0, j;
    int header_height, header_width, header_start_x, header_start_y;
    int list_height, list_width, left_list_start_x, left_list_start_y, right_list_start_x, right_list_start_y;
    int border_list_height, border_list_width, border_left_list_start_x, border_left_list_start_y, border_right_list_start_y, border_right_list_start_x;
    char **files;
    char symbol;
    char* current_file;
    char empty[] = "cd ";
    const char* file_list = ".!file_list";
    char system_command_1[] = "ls -la | awk '{print $9}' > ";
    char system_command_2[] = "rm ";
    char system_command_3[] = "cd ";

    strcat(system_command_1, file_list);
    strcat(system_command_2, file_list);

    WINDOW *header;
    WINDOW *left_list, *border_left_list, *right_list, *border_right_list;
    initscr();
    signal (SIGWINCH, sig_winch);
    keypad (stdscr, 1);
    mousemask (BUTTON1_CLICKED, NULL);
    curs_set (0);
    start_color();
    refresh();
    init_pair (1, COLOR_YELLOW, COLOR_YELLOW);
    init_pair (2, COLOR_WHITE, COLOR_BLUE);
    init_pair (3, COLOR_BLACK, COLOR_CYAN);
    init_pair (4, COLOR_BLACK, COLOR_BLACK);
    init_pair (5, COLOR_BLACK, COLOR_GREEN);
    init_pair (6, COLOR_BLACK, COLOR_RED);

    int cols, rows;
    getscrsize (&rows, &cols);
    if (cols < 12) cols = 12;
    cols += (cols % 2 == 0);
    if (cols > 94) cols = 94;

    num_of_spaces = (cols - 12) / 2;
    header_height = 1;
    header_start_x = 0;
    header_start_y = 0;
    header_width = cols - header_start_x;
    header_width -= (header_width % 2 == 1);
    header = newwin(header_height, header_width, header_start_y, header_start_x);
    wbkgd(header, COLOR_PAIR(3));
    for (i = 0; i < num_of_spaces; i++)
	wprintw(header, "%c", 32);
    wprintw (header, "FILE MANAGER");
    wrefresh (header);

    border_list_height = (rows - 1) - header_height - header_start_y;
    border_list_width = header_width / 2;
    border_left_list_start_y = header_height + header_start_y;
    border_right_list_start_y = border_left_list_start_y;
    border_left_list_start_x = header_start_x;
    border_right_list_start_x = border_left_list_start_x + border_list_width;
    list_height = border_list_height - 2;
    list_width = border_list_width - 2;
    left_list_start_x = 1;
    left_list_start_y = 1;
    right_list_start_x = 1;
    right_list_start_y = 1;

    border_left_list = newwin (border_list_height, border_list_width, border_left_list_start_y, border_left_list_start_x);
    wattron (border_left_list, COLOR_PAIR(2));
    box (border_left_list, 0, 0);
    left_list = derwin (border_left_list, list_height, list_width, left_list_start_y, left_list_start_x);
    wbkgd (left_list, COLOR_PAIR(2));
    wrefresh (left_list);
    wrefresh (border_left_list);

    border_right_list = newwin (border_list_height, border_list_width, border_right_list_start_y, border_right_list_start_x);
    wattron (border_right_list, COLOR_PAIR(2));
    box (border_right_list, 0, 0);
    right_list = derwin (border_right_list, list_height, list_width, right_list_start_y, right_list_start_x);
    wbkgd (right_list, COLOR_PAIR(2));
    wrefresh (right_list);
    wrefresh (border_right_list);

    system(system_command_1);

    fp = fopen(file_list, "r");
    if (fp == NULL) {
	printf("can't open list of files\n");
	fclose(fp);
	return 0;
    }
    while(!feof(fp)) {
	fscanf(fp, "%c", &symbol);
	if (symbol == 10) {
	    num_of_files++;
	}
    }
    fseek(fp, SEEK_SET, 0);
    num_of_files -= 3;
    files = malloc(num_of_files * sizeof(char*));
    for (i = 0; i < num_of_files;) {
	files[i] = malloc(255);
	fscanf(fp, "%s", files[i]);
	if (strcmp(files[i], ".") != 0 && strcmp(files[i], file_list) != 0) {
	    i++;
	}
    }
    fclose(fp);
    system(system_command_2);

    for (i = 0; i <num_of_files; i++) {
	wprintw(left_list, " %s\n", files[i]);
    }
    wrefresh (left_list);

    ///////experiments with mouse
    while (wgetch(stdscr) == KEY_MOUSE) {
	MEVENT event;
	getmouse (&event);
	if (event.y >= header_start_y + 2 && event.y < header_start_y + 1 + num_of_files && event.x >= header_start_x + 1 && event.x < header_start_x + 1 + list_width) {
	    current_file = files[event.y - (header_start_y + 2)];
	    strcat(system_command_3, current_file);
	    //system(system_command_3);
	    ///////////////////////
	    num_of_files = 0;


	    //system(system_command_1);
	    //system("cd ..; ls -la | awk '{print $9}' > .!file_list");
	    system("cd ..; ls");
	    system("sleep 1");
    fp = fopen(file_list, "r");
    if (fp == NULL) {
	printf("can't open list of files\n");
	fclose(fp);
	return 0;
    }
    while(!feof(fp)) {
	fscanf(fp, "%c", &symbol);
	if (symbol == 10) {
	    num_of_files++;
	}
    }
    fseek(fp, SEEK_SET, 0);
    num_of_files -= 3;
    free(files);
    files = malloc(num_of_files * sizeof(char*));
    for (i = 0; i < num_of_files;) {
	files[i] = malloc(255);
	fscanf(fp, "%s", files[i]);
	if (strcmp(files[i], ".") != 0 && strcmp(files[i], file_list) != 0) {
	    system("sleep 1");
	    printf("%s\n", files[i]);
	    i++;
	}
    }
    fclose(fp);
    system(system_command_2);

    for (i = 0; i <num_of_files; i++) {
	wprintw(left_list, " %s\n", files[i]);
    }
    wrefresh (left_list);



	    //////////////////////
	    move (rows - 1, 0);
	    printw ("%s\n", current_file);
	    strcpy(system_command_3, empty);
	} else {
	    move (rows - 1, cols / 2);
	    printw ("Clicked at %dx%d\n", event.y, event.x);
	}
	refresh ();
    }
    //////

    delwin (header);
    delwin (border_left_list);
    delwin (border_right_list);
    delwin (right_list);
    delwin (left_list);
    move (rows - 1, 1);
    printw ("Press any key to continue...");
    refresh();
    getch();
    endwin();
    exit (EXIT_SUCCESS);

    return 0;
}