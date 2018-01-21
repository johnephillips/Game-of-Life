/*****************************************************************************
 *	John Phillips                                                            *
 *	Course: ICS 4U                                                           *  
 *	Date: February 23, 2017                                                  * 
 *	                                                                         * 
 *	Purpose:  Simulate a 2D Game of Life from starting configurations.       *
 *	                                                                         *
 *	Usage:  Need allegro-5.2.dll and either allegro_monolith-5.2.dll or each *
 *	        individual font and primitive dll in folder. Before running the  *
 *          file names and desired sizes need to be changed. The font also   *
 *			needs to be in the folder.                                       *
 *																			 *
 *			This program simulates a 2D game of life on a bounded grid that  *
 *			is read in from a file. Using the left and right arrow keys you  *
 *			can switch between the loaded files, and with space bar you can  *
 *			simulate the next generation; press escape to exit.              *
 *                                                                           *
 *	Revision History:                                                        *  
 *	                                                                         *
 *	Known Issues:   Animation set up is a bit jittery.                       *
 *                                                                           *
 *	                                                                         *  
 *****************************************************************************/

#include <iostream>
#include <allegro5/allegro5.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>
#include <apmatrix.h>
#include <fstream>
using namespace std;

//SIZE is size of each square in pixels. 
const int ROWS = 20, COLS = 50, SIZE = 50, TITLESPACE = SIZE*2, NUMGRIDS = 3, WIDTH = COLS*SIZE, HEIGHT = ROWS*SIZE + TITLESPACE, GPS = 5; //Gens per second

//Change these names to match those in the folder. A valid font is required
const char filename[][15] = {"LIFE_HRV.DAT", "LIFE_CAT.DAT", "LIFE_GLI.DAT"}, fontName[] = "Arimo-Regular.ttf";

apmatrix<char> grid[NUMGRIDS];
//Used to efficiently loop through adjacent cells
const int deltaRow[8] = {-1, -1, -1, 0, 0, 1, 1, 1}, deltaCol[8] = {-1, 0, 1, -1, 1, -1, 0, 1};
const char ALIVE = 'x';
const char DEAD = '.';

bool initializeAllegro(ALLEGRO_DISPLAY *&, ALLEGRO_EVENT_QUEUE *&, ALLEGRO_TIMER *&, ALLEGRO_FONT *&);
bool loadGrids();
bool loadGrid(int);
bool inbound(int, int, int);
void displayGrid(int, int [], ALLEGRO_FONT *&);
void nextGen(int);
void checkEvent(ALLEGRO_EVENT_QUEUE *&eventQueue, ALLEGRO_EVENT &ev, bool &done, bool &next, int &curr, bool &tick, bool &first);
int countNeighbours(int, int, int);


int main(){
	ALLEGRO_DISPLAY *display = NULL;
	ALLEGRO_EVENT_QUEUE *eventQueue = NULL;
	ALLEGRO_FONT *font = NULL;
	ALLEGRO_TIMER *timer = NULL;
	
	//Should initialize all the aspects of allegro used in the program and will also read in NUMGRIDS grids
	if (!initializeAllegro(display, eventQueue, timer, font) || !loadGrids())
		return 1;
	
	//done stores whether the user has indicated termination
	//next stores whether the simulation of the next generation was requested
	//first stores whether the first nextGen call is needed
	//changed stores whether the grid to display has just switched
	//tick stores whether a timer event was reached
	bool done = false, next = false, first = false, changed = false, tick = true;
	//Current grid and the generation numbers
	int curr = 0, gen[3] = {0};
	ALLEGRO_EVENT ev;
	displayGrid(curr, gen, font);
	al_start_timer(timer);
	
	while (!done) {
		//Passes the necessary variables by reference to determine current state
		checkEvent(eventQueue, ev, done, next, curr, tick, first);
		
		if (tick) {
			displayGrid(curr, gen, font);
			tick = false;
			if (next) {
	            nextGen(curr);
	            gen[curr]++;
	        }  
		} 
		if (first) {
			nextGen(curr);
            gen[curr]++;
            first = false;
		}
	}
	
	al_destroy_font(font);
	al_destroy_timer(timer);
	al_uninstall_keyboard();
	al_destroy_event_queue(eventQueue);
	al_destroy_display(display);
	
	return 0;
}

//The *& is necessary to ensure that the initialization is maintained after the function is done
bool initializeAllegro(ALLEGRO_DISPLAY *&display, ALLEGRO_EVENT_QUEUE *&eventQueue, ALLEGRO_TIMER *&timer, ALLEGRO_FONT *&font) {
	if (!al_init()){
		cerr << "Error initializing allegro" << endl;
		return false;
	}
	
	display = al_create_display(WIDTH, HEIGHT);
	if (!display){
		cerr << "Error creating display" << endl;
		return false;
	}
	
	if (!al_init_primitives_addon()) {
		cerr << "Error initializing primitives" << endl;
		return false;
	}
	
	if (!al_install_keyboard()) {
		cerr << "Error installing keyboard" << endl;
		return false;
	}
	
	timer = al_create_timer(1./GPS);
	if (!timer) {
		cerr << "Error initializing timer" << endl;
		return false;
	}
	
	eventQueue = al_create_event_queue();
	if (!eventQueue) {
		cerr << "Error creating event queue" << endl;
		return false;
	}
	
	al_register_event_source(eventQueue, al_get_keyboard_event_source());
	al_register_event_source(eventQueue, al_get_timer_event_source(timer));
	
	al_init_font_addon();
	al_init_ttf_addon();
	
	font = al_load_ttf_font(fontName, SIZE, 0);
	
	if (!font) {
		cerr << "Error loading font " << fontName << endl;
		return false;
	}
	
	return true;
}

//Loads all NUMGRIDS grids
bool loadGrids() {
	for (int i = 0; i < NUMGRIDS; i++) {
		grid[i].resize(ROWS, COLS);
		if (!loadGrid(i)) {
			return false;
		}
	}
	return true;
}

//Loads a single grid. Can be used to implement a reset to gen 0 feature
bool loadGrid(int num) {
    fstream fin(filename[num]);
    if (!fin.is_open()) {
        cerr << "Error opening \"" << filename[num] << "\"" << endl;
		return false;
	}
	
	for (int r = 0; r < grid[num].numrows(); r++){
		for (int c = 0; c < grid[num].numcols(); c++){
			fin >> grid[num][r][c];
		}
	}
	
	fin.close();
	
	return true;
}

//Console display of grid
/*void displayGrid(){
	system("cls");
	for (int r = 0; r < grid.numrows(); r++){
		for (int c = 0; c < grid.numcols(); c++){
			cout << grid[r][c];
		}
		cout << endl;
	}
}*/

void displayGrid(int num, int gen[], ALLEGRO_FONT *&font) {
	al_clear_to_color(al_map_rgb(0, 0, 0));
	
	//Display generation title above grid approximately centered
	al_draw_textf(font, al_map_rgb(255, 255, 255), WIDTH/2, TITLESPACE/4, ALLEGRO_ALIGN_CENTER, "Generation %d", gen[num]);
	
	//Show grid lines
	//Vertical
	for (int i = 1; i < grid[num].numcols(); i++){
		al_draw_line(i*SIZE, 0 + TITLESPACE, i*SIZE, HEIGHT, al_map_rgb(255, 255, 255), 1);
	}
	//Horizontal
	for (int i = 0; i < grid[num].numrows(); i++){
		al_draw_line(0, i*SIZE + TITLESPACE, WIDTH, i*SIZE + TITLESPACE, al_map_rgb(255, 255, 255), 1);
	}
	//Show live cells
	for (int r = 0; r < grid[num].numrows(); r++){
		for (int c = 0; c < grid[num].numcols(); c++){
			if (grid[num][r][c] == ALIVE){
				al_draw_filled_rectangle(c*SIZE, r*SIZE + TITLESPACE, (c+1)*SIZE, (r+1)*SIZE + TITLESPACE, al_map_rgb(255, 255, 0));
			}		
	    }
	}
	al_flip_display();
}

void checkEvent(ALLEGRO_EVENT_QUEUE *&eventQueue, ALLEGRO_EVENT &ev, bool &done, bool &next, int &curr, bool &tick, bool &first) {
	al_wait_for_event(eventQueue, &ev);
	if (ev.type == ALLEGRO_EVENT_KEY_DOWN) {
		switch(ev.keyboard.keycode) {
			case ALLEGRO_KEY_ESCAPE:
		        done = true;
		        break;
			case ALLEGRO_KEY_SPACE:
				first = next = true;
				break;
			case ALLEGRO_KEY_RIGHT:
				curr = (curr + 1) % NUMGRIDS;
				break;
			case ALLEGRO_KEY_LEFT:
                curr = (curr - 1 + NUMGRIDS) % NUMGRIDS; //Add NUMGRIDS to ensure that the result will always be positive
                break;
		}
	}
	else if (ev.type == ALLEGRO_EVENT_KEY_UP) {
		switch(ev.keyboard.keycode) {
			case ALLEGRO_KEY_SPACE:
		        next = false;
		        break;
		}
	}
	else if (ev.type == ALLEGRO_EVENT_TIMER) {
		tick = true;
	}
}

void nextGen(int num){
	apmatrix<char> nextGrid(ROWS, COLS);
	int adj; //Number of alive neighbours
	for (int r = 0; r < grid[num].numrows(); r++){
		for (int c = 0; c < grid[num].numcols(); c++){
			adj = countNeighbours(num, r, c);

			if (grid[num][r][c] == ALIVE){
				if (adj == 2 || adj == 3)
					nextGrid[r][c] = ALIVE;
				else
					nextGrid[r][c] = DEAD;
			}
			else{
				if (adj == 3)
					nextGrid[r][c] = ALIVE;
				else
					nextGrid[r][c] = DEAD;
			}
		}
	}
	grid[num] = nextGrid;
}

int countNeighbours(int num, int row, int col){
	int count = 0;
	for (int i = 0; i < 8; i++){
		int neighbourRow = row + deltaRow[i];
		int neighbourCol = col + deltaCol[i];
		
		if (inbound(num, neighbourRow, neighbourCol) && grid[num][neighbourRow][neighbourCol] == ALIVE){
			count++;
		}
	}
	return count;
}

bool inbound(int num, int row, int col){
	return row >= 0 && row < grid[num].numrows() && col >= 0 && col < grid[num].numcols();
}
