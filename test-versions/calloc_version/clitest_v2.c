#define _GNU_SOURCE
#include <unistd.h>   // fork, usleep, _Exit
#include <stdio.h>    // FILE, fopen, fclose, fwrite, fread, printf
#include <stdint.h>   // uint16_t, uint8_t
#include <string.h>   // memcpy, strcpy
#include <stdlib.h>   // _Exit, general utilities
#define HEIGHT 240 //y coordinates
#define WIDTH 240 //x coordinates
#define PIXEL(x, y) framebuffer[(x) * HEIGHT + (y)] // this allows setting a single pixel in the frame buffer, and as a memory of the indexing.
#define HEIGHT_GRAPH 100
#define WIDTH_GRAPH 150
#define NUMBER_OF_MOVES_PER_FRAME 100 //drawn in the framebuffer, the display is still a lot slower.
#define NUMBER_OF_CHILDS 1
#define FRAME_BYTES (HEIGHT * WIDTH * sizeof(uint16_t))
#define WIDTH_CHAR 8
#define HEIGHT_CHAR 8
#define MEMORY_ALLOCATIONS 9 //number of memory allocations in main
enum read_file {
	FRAMEBUFFER, FRAMEBACKGROUND
};

void write_files_multi(uint16_t store_array[WIDTH*HEIGHT]){
	char filename[] = "/dev/shm/framebuffer.tmp";
	FILE *file = fopen(filename, "wb");
	fwrite(store_array, sizeof(uint16_t), WIDTH*HEIGHT, file);//dumps the binary to the file, decreasing the time it takes by a factor 60.
	fclose(file);
	rename("/dev/shm/framebuffer.tmp", "/dev/shm/framebuffer");
}
void read_files_multi(uint16_t store_array[WIDTH * HEIGHT], uint8_t mode) {
	char filename[2048];
	switch (mode) {
		case 0:
			rename("/dev/shm/framebuffer", "/dev/shm/framebuffer.reading"); //to hopefully prevent race conditions, by decreasing the change of it happening.
			strcpy(filename, "/dev/shm/framebuffer.reading");
			break;
		case 1:
			strcpy(filename, "RawPictures/Background.raw");
			break;
		default:
			rename("/dev/shm/framebuffer", "/dev/shm/framebuffer.reading"); //to hopefully prevent race conditions, by decreasing the change of it happening.
			strcpy(filename, "/dev/shm/framebuffer.reading");
			break;
	}
	FILE *file = fopen(filename, "rb"); //reads the binary data.
	fread(store_array, sizeof(uint16_t), WIDTH*HEIGHT, file);
	fclose(file);
}
void init_fond(uint16_t fontarray000[128 * WIDTH_CHAR * HEIGHT_CHAR], uint16_t fontarray090[128 * WIDTH_CHAR * HEIGHT_CHAR * WIDTH_CHAR], uint16_t fontarray180[128 * WIDTH_CHAR * HEIGHT_CHAR * WIDTH_CHAR], uint16_t fontarray270[128 * WIDTH_CHAR * HEIGHT_CHAR * WIDTH_CHAR]) {
	//this works but can be made better
	char filename[1024] = "font.uint16"; //replace with the actual location of the font
	FILE *file = fopen(filename, "r"); //read uint16_t file
	for (uint8_t c = 0; c < 128; c++) {
		for (int x = 0; x < WIDTH_CHAR; x++) {
			for (int y = 0; y < HEIGHT_CHAR; y++) {
				fscanf(file, " %hu", &fontarray000[(c * (WIDTH_CHAR * HEIGHT_CHAR) + x * HEIGHT_CHAR + y)]);
			}
		}
	}
	fclose(file);
	//flip 90 degrees
	for (uint8_t c = 0; c < 128; c++) {
		for (int x = 0; x < WIDTH_CHAR; x++) {
			for (int y = 0; y < HEIGHT_CHAR; y++) {
				fontarray090[c * (WIDTH_CHAR * HEIGHT_CHAR) + y * WIDTH_CHAR + (WIDTH_CHAR - 1 - x)] = fontarray000[(c * (WIDTH_CHAR * HEIGHT_CHAR) + x * HEIGHT_CHAR + y)];
			}
		}
	}
	//flip 180 degrees
	for (uint8_t c = 0; c < 128; c++) {
		for (int x = 0; x < WIDTH_CHAR; x++) {
			for (int y = 0; y < HEIGHT_CHAR; y++) {
				fontarray180[c * (WIDTH_CHAR * HEIGHT_CHAR) + (WIDTH_CHAR - 1 - x) * HEIGHT_CHAR + (HEIGHT_CHAR - 1 - y)] = fontarray000[(c * (WIDTH_CHAR * HEIGHT_CHAR) + x * HEIGHT_CHAR + y)];
			}
		}
	}
	//flip 270 degrees
	for (uint8_t c = 0; c < 128; c++) {
		for (int x = 0; x < WIDTH_CHAR; x++) {
			for (int y = 0; y < HEIGHT_CHAR; y++) {
				fontarray270[c * (WIDTH_CHAR * HEIGHT_CHAR) +(HEIGHT_CHAR - 1 - y) * WIDTH_CHAR + x] = fontarray000[(c * (WIDTH_CHAR * HEIGHT_CHAR) + x * HEIGHT_CHAR + y)];
			}
		}
	}
}
void draw_cli(const uint16_t framebuffer[WIDTH * HEIGHT], int framenum){
	printf("%s%i\n", "Frame number: ", framenum);
	for (int y = 0; y < HEIGHT; y++) {
		for (int x = 0; x < WIDTH; x++) {
			switch (PIXEL(x, y)) {
				case 0:
					printf("0");
					break;
				default:
					printf("1");
					break;
			};
		}
		printf("\n");
	}
}
void write_font(uint16_t fontarray[128*(WIDTH_CHAR*HEIGHT_CHAR)], const char string[1024], uint8_t x, uint8_t y, uint16_t text_buffer[x*y]){ //curently string length can't be longer than x_length/8
	printf("string started\n");
	uint8_t max_char_numbers = x / WIDTH_CHAR;
	for (uint8_t c = 0; c < max_char_numbers; c++) {
		printf("%i\n", c);
		uint8_t x_cooridinate = 0;
		if (string[c] != 0) {
			for (uint8_t x_space = 0; x_space < WIDTH_CHAR; x_space++) {
				x_cooridinate = (c*WIDTH_CHAR) + x_space;
				memmove(&text_buffer[(x_cooridinate*y) + 2], &fontarray[string[c]* (WIDTH_CHAR * HEIGHT_CHAR) + (x_space * HEIGHT_CHAR) + 0], HEIGHT_CHAR * sizeof(uint16_t));
				printf("%i%-12i\n", c, x_space);
			}
		}
		else {
			c = max_char_numbers;
			printf("%i%-12i\n", c, max_char_numbers);
		}
	}
}
void master(void) {
	uint8_t success_memory_alloc = 0;
	uint8_t memory_allocation_tries = 0;
	uint16_t *framebuffer = NULL;
	uint16_t *framebuffer_drawn = NULL;
	uint16_t *font = NULL;
	uint16_t *font90 = NULL;
	uint16_t *font180 = NULL;
	uint16_t *font270 = NULL;
	uint16_t *text_buffer1 = NULL;
	uint16_t *graphbuffer = NULL;
	uint8_t *graph_data = NULL;
	const char string_1[] = {"3.3V"};
	while (success_memory_alloc != MEMORY_ALLOCATIONS) {
		memory_allocation_tries++;
		success_memory_alloc = 0;
		if (framebuffer == NULL) {
			framebuffer = calloc(WIDTH * HEIGHT, sizeof(uint16_t));
		}
		else {
			success_memory_alloc = success_memory_alloc + 1;
		}
		if (framebuffer_drawn == NULL) {
			framebuffer_drawn = calloc(WIDTH * HEIGHT, sizeof(uint16_t));
		}
		else {
			success_memory_alloc = success_memory_alloc + 1;
		}
		if (font == NULL) {
			font = calloc(128 * (WIDTH_CHAR * HEIGHT_CHAR), sizeof(uint16_t));
		}
		else {
			success_memory_alloc = success_memory_alloc + 1;
		}
		if (font90 == NULL) {
			font90 = calloc(128 * (WIDTH_CHAR * HEIGHT_CHAR), sizeof(uint16_t));
		}
		else {
			success_memory_alloc = success_memory_alloc + 1;
		}
		if (font180 == NULL) {
			font180 = calloc(128 * (WIDTH_CHAR * HEIGHT_CHAR), sizeof(uint16_t));
		}
		else {
			success_memory_alloc = success_memory_alloc + 1;
		}
		if (font270 == NULL) {
			font270 = calloc(128 * (WIDTH_CHAR * HEIGHT_CHAR), sizeof(uint16_t));
		}
		else {
			success_memory_alloc = success_memory_alloc + 1;
		}
		if (text_buffer1 == NULL) {
			text_buffer1 = calloc(32*10, sizeof(uint16_t));
		}
		else {
			success_memory_alloc = success_memory_alloc + 1;
		}
		if (graphbuffer == NULL) {
			graphbuffer = calloc(WIDTH_GRAPH * HEIGHT_GRAPH, sizeof(uint16_t));
		}
		else {
			success_memory_alloc = success_memory_alloc + 1;
		}
		if (graph_data == NULL) {
			graph_data = calloc(WIDTH_GRAPH, sizeof(uint8_t));
		}
		else {
			success_memory_alloc = success_memory_alloc + 1;
		}
		if (success_memory_alloc != MEMORY_ALLOCATIONS) {
			usleep(1000);
		}
		if (success_memory_alloc != MEMORY_ALLOCATIONS && memory_allocation_tries > 4) {
			printf("Memory allocation timed out\n");
			_Exit(1);
		}
	}
	init_fond(font, font90, font180, font270);
	write_font(font90, string_1, 32, 10, text_buffer1);
	for (int x = 0; x < WIDTH; x++) {
		for (int y = 0; y < HEIGHT; y++) {
			if (x == 89 || y == 89) {
				PIXEL(x, y) = 0x0fff;
			}
		}
	}
	int b0 = 0;
	int b1 = 0;
	for (int i =0; i <WIDTH_GRAPH; i++) {
		if (b0 <99 && b1<=b0) {
			b1 = b0;
			b0 = b0 +1;
		}
		else if (b0<99 && b1>b0) {
			b1 = b0;
			b0 = b0 -1;
		}
		else if (b0 == 99) {
			b1 = b0;
			b0 = b0 -1;
		}
		graph_data[i] = b0;
	}
	for (int x = 0; x < WIDTH_GRAPH; x++) {
		graphbuffer[x*HEIGHT_GRAPH+graph_data[x]] = 0xffff;
	}

	//combine frame buffer parts below:
	for (int x = 0; x < WIDTH; x++) {
		for (int y = 0; y < HEIGHT; y++) {
			if (x >=0 && x < 56) {
				//nothing yet
			}
			else if (x >= 56 && x < 88) {
				if (y >= 230 && y < HEIGHT) {
					memcpy(&PIXEL(x, y), &text_buffer1[(x-56)*(HEIGHT_CHAR+2)], HEIGHT_CHAR * sizeof(uint16_t));
					y = y + HEIGHT_CHAR + 2;
				}
			}
			else if (x >= 90) {
				if (y >= 139){
					memcpy(&PIXEL(x, y), &graphbuffer[(x-89)*(HEIGHT_GRAPH)], HEIGHT_GRAPH * sizeof(uint16_t));
					y = y + HEIGHT_GRAPH;
				}
			}
		}
	}
	memmove(framebuffer_drawn, framebuffer, WIDTH * HEIGHT * sizeof(uint16_t));
	write_files_multi(framebuffer_drawn);
	free(framebuffer);
	free(framebuffer_drawn);
	free(font);
	free(font90);
	free(font180);
	free(font270);
	free(text_buffer1);
	free(graphbuffer);
	free(graph_data);
}
void display(void) {
	uint16_t *framebuffer_on_display = NULL;
	while (framebuffer_on_display == NULL) {
		printf("intilazing memory\n");
		framebuffer_on_display = calloc(WIDTH * HEIGHT, sizeof(uint16_t));
		if (framebuffer_on_display == NULL) {
			printf("Failed to allocate framebuffer for display\n");
		}
		else {
			break;
		}
	}
	for (int i = 0; i <1; i++) {
		usleep(700*1000);
		read_files_multi(framebuffer_on_display, FRAMEBUFFER);
		draw_cli(framebuffer_on_display, i);

	}
	free(framebuffer_on_display);
}
int main (void) {
	int fork_num = 0;
	int fork_state = 1;
	usleep(1000 * 10); //prevent a segfault
	for (int i = 0; i < NUMBER_OF_CHILDS; i++) {
		//needed for fork
		if (fork_state != 0) {
			fork_state = fork();
			//the fork returns a number to the parent (or master, as later defined), an 0 to a child.
			fork_num = fork_num + 1;
			if (fork_state != 0) {
				printf("%s%i%s\n", "fork ", fork_num, " started");
			}
		}
	}
	if (fork_state != 0) {
		//sets master branch(or parent) to forknum 0, to prevent two forks running the same code.
		fork_num = 0;
	}
	switch (fork_num) {
		case 1:
			master();
			break;
		case 0:
			display();
			break;
		default:
			printf("To many forks");
			_Exit(1);
	}
}
