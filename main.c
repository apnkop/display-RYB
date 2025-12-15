#define _GNU_SOURCE
#include <libpynq.h>
#include <unistd.h>   // fork, usleep, _Exit
#include <display.c>
#define HEIGHT 240 //y coordinates
#define WIDTH 240 //x coordinates
#define PIXEL(x, y) framebuffer[(x) * HEIGHT + (y)] // this allows setting a single pixel in the frame buffer, and as a memory of the indexing.
#define HEIGHT_GRAPH 100
#define WIDTH_GRAPH 150
#define NUMBER_OF_MOVES_PER_FRAME 100 //drawn in the framebuffer, the display is still a lot slower.
#define NUMBER_OF_CHILDS 1
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
void displayDrawMultiPixelsv2(display_t *display, uint16_t x, uint16_t y, uint16_t sizex, uint16_t sizey, uint16_t *colors) {
	uint16_t _x1 = x + display->_offsetx;
	uint16_t _x2 = _x1 + sizex;
	uint16_t _y1 = y + display->_offsety;
	uint16_t _y2 = _y1 + sizey;
	uint16_t size = sizey*sizex;
	spi_master_write_command(display, 0x2A); // set column(x) address
	spi_master_write_addr(display, _x1, _x2);
	spi_master_write_command(display, 0x2B); // set Page(y) address
	spi_master_write_addr(display, _y1, _y2);
	spi_master_write_command(display, 0x2C); // memory write
	spi_master_write_colors(display, colors, size); //write to the display, I cant figure out how to increase spead more:-(
} //this takes about 70 ms
void display_flush(display_t *d, uint16_t *fb){
	displayDrawMultiPixelsv2(d, 0, 0, WIDTH, HEIGHT, fb);
}
int main (void){
	uint16_t framebuffer[HEIGHT][WIDTH];
	for(int y = 0; y < HEIGHT; y++){ //set framebuffer black
		for(int x =0; x < WIDTH; x++){
			framebuffer[y][x] = 0;
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
				if (y >= 229 && y < HEIGHT) {
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
	display_t display;
	display_init(&display);
	uint16_t *framebuffer_on_display = NULL;
	while (framebuffer_on_display == NULL) {
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
		display_flush(&display, framebuffer_on_display);

	}
	free(framebuffer_on_display);
}
int main (void) {
	pynq_init();
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
		case 0:
			master();
			break;
		case 1:
			display();
			break;
		default:
			printf("To many forks");
			_Exit(1);
	}
}
