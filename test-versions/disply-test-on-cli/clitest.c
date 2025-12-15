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
void init_fond(uint16_t fontarray[128][WIDTH_CHAR][HEIGHT_CHAR]) {
	char filename[1024] = "font.uint16"; //replace with the actual location of the font
	FILE *file = fopen(filename, "r"); //read uint16_t file
	for (uint8_t c = 0; c < 128; c++) {
		for (int x = 0; x < WIDTH_CHAR; x++) {
			for (int y = 0; y < HEIGHT_CHAR; y++) {
				fscanf(file, " %hu", &fontarray[c][x][y]);
			}
		}
	}
	fclose(file);
}
void flip_font(uint16_t fontarray[128][WIDTH_CHAR][HEIGHT_CHAR]) {
	uint16_t fontarray2[128][WIDTH_CHAR][HEIGHT_CHAR];
	for (uint8_t c = 0; c < 128; c++) {
		for (int x = 0; x < WIDTH_CHAR; x++) {
			for (int y = 0; y < HEIGHT_CHAR; y++) {
				fontarray2[c][x][y] = fontarray[c][x][y];
			}
		}
	}
	for (uint8_t c = 0; c < 128; c++) {
		for (uint8_t x = 0; x < WIDTH_CHAR; x++) {
			for (uint8_t y = 0; y < HEIGHT_CHAR; y++) {
				fontarray[c][y][WIDTH_CHAR - 1 - x] = fontarray2[c][x][y];
			}
		}
	}
}
void write_font(uint16_t fontarray[128][WIDTH_CHAR][HEIGHT_CHAR], const char string[1024], uint8_t x, uint8_t y, uint16_t string_buffer[x][y]){ //curently string length can't be longer than x_length/8
	printf("string started\n");
	uint8_t max_char_numbers = x / WIDTH_CHAR;
	for (uint8_t c = 0; c < max_char_numbers; c++) {
		printf("%i\n", c);
		if (string[c] != 0) {
			for (uint8_t x_space = 0; x_space < WIDTH_CHAR; x_space++) {
				memmove(&string_buffer[(c*WIDTH_CHAR)+x_space][1], &fontarray[(string[c])][x_space][0], HEIGHT_CHAR * sizeof(uint16_t));
				printf("%i%-12i\n", c, x_space);
			}
		}
		else {
			c = max_char_numbers;
			printf("%i%-12i\n", c, max_char_numbers);
		}
	}
}
void draw_cli(const uint16_t framebuffer[WIDTH * HEIGHT], int framenum){
	/*int framebuffertmp[WIDTH][HEIGHT];
	for (int x = 0; x < WIDTH; x++) {
		for (int y = 0; y < HEIGHT; y++) {
			framebuffertmp[x][y] = framebuffer[x][y];
		}
	}
	for (int x = 0; x < WIDTH; x++) {
		for (int y = 0; y < HEIGHT; y++) {
			framebuffer[y][x] = framebuffertmp[x][y];
		}
	}*/
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
int main (void) {
	int direction = 1;
	uint16_t fontarray[128][WIDTH_CHAR][HEIGHT_CHAR];
	init_fond(fontarray);
	flip_font(fontarray);
	uint16_t *framebuffer = nullptr;
	while (framebuffer == nullptr) {
		printf("intilazing memory\n");
		framebuffer = (uint16_t *) calloc(WIDTH * HEIGHT, sizeof(uint16_t));
		if (framebuffer == NULL) {
			printf("Failed to allocate framebuffer\n");
		}
		else {
			break;
		}
	}
	const char string_1[] = {"3.3V"};
	uint16_t string_1_buffer[32][10] = {0};
	uint16_t graphbuffer[WIDTH_GRAPH][HEIGHT_GRAPH];
	for (int x = 0; x < WIDTH_GRAPH; x++) {
		for (int y = 0; y < HEIGHT_GRAPH; y++) {
			graphbuffer[x][y] = 0; //black
		}
	}
	for (int x = 0; x < WIDTH; x++) {
		for (int y = 0; y < HEIGHT_GRAPH; y++) {
			if (x == 90 || y == 90) {
				PIXEL(x, y) = 0x0fff;
			}
		}
	}
	int b0 = 0;
	int fork_num = 0;
	int fork_state = 1;
	write_files_multi(framebuffer); // ensures that if the display program starts to fast, it does not segfault
	usleep(1000*10); //prevent a segfault
	for (int i =0; i <NUMBER_OF_CHILDS; i ++){ //needed for fork
		if (fork_state != 0){
			fork_state = fork(); //the fork returns a number to the parent (or master, as later defined), an 0 to a child.
			fork_num = fork_num + 1;
			if (fork_state != 0){
				printf("%s%i%s\n", "fork ", fork_num, " started");
			}
		}
	}
	if (fork_state != 0){ //sets master branch(or parent) to forknum 0, to prevent two forks running the same code.
		fork_num = 0;
	}
	switch (fork_num) {
		case 1:
			auto framebuffer_drawn = (uint16_t *) calloc(WIDTH * HEIGHT, sizeof(uint16_t));
			for (int i = 0; i < 1; i++) {
				for (int k = 0; k <NUMBER_OF_MOVES_PER_FRAME; k++){
					if (b0 <99 && direction == 1) {
						b0 = b0+1;
					}
					else if (b0 >0 && direction == 2) {
						b0 = b0-1;
					}
					else if (b0 == 99 && direction == 1) {
						direction = 2;
					}
					else if (b0 == 0 && direction == 2) {
						direction = 1;
					}
					for (int x = WIDTH_GRAPH; x > 0; x--) {
						memmove(&graphbuffer[x][0], &graphbuffer[x-1][0], (sizeof(uint16_t)*HEIGHT_GRAPH));
					}
					memset(graphbuffer[0], 0, sizeof(uint16_t)*HEIGHT_GRAPH);
					graphbuffer[0][b0] = b0;
				}
				write_font(fontarray, string_1, 32, 10, string_1_buffer);
				for (int x = 0; x < WIDTH; x++) {
					if (x < 88 && x > 56) {
						memmove(&PIXEL(x, 229), &string_1_buffer[x-56][0], HEIGHT_CHAR*sizeof(uint16_t));
					}
					if (x > 89) {
						memmove(&PIXEL(x, 139), &graphbuffer[x-90][0], HEIGHT_GRAPH*sizeof(uint16_t));
					}
				}
				memmove(framebuffer_drawn, framebuffer, sizeof(&framebuffer_drawn));
				write_files_multi(framebuffer_drawn);
			}
			free(framebuffer);
			free(framebuffer_drawn);
			break;
		case 0:
			for (int i = 0; i <1; i++) {
				usleep(700*1000);
				//read_files_multi(framebuffer, FRAMEBUFFER);
				draw_cli(framebuffer, i);

			}

			break;
		default:
			printf("To many forks");
			_Exit(1);
			break;
	}
}