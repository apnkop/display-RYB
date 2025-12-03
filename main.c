#define _GNU_SOURCE
#include <libpynq.h>
#include <unistd.h>
#include <display.c>
#define HEIGHT 240 //y coordinates
#define WIDTH 240 //x coordinates
#define HEIGHT_GRAPH 100
#define WIDTH_GRAPH 150
#define NUMBER_OF_MOVES_PER_FRAME 10 //drawn in the framebuffer, the display is still a lot slower.
#define NUMBER_OF_CHILDS 1
#define FRAME_BYTES (HEIGHT * WIDTH * sizeof(uint16_t))
enum read_file {
	FRAMEBUFFER, FRAMEBACKGROUND
};
/* Warning !!! BECAUSE OF PROGRAM SPEED all arrays, that store pixel colour are y by x. so [HEIGHT]X[WIDTH] */
void write_files_multi(uint16_t store_array[HEIGHT][WIDTH]){
	char filename[] = "/dev/shm/framebuffer.tmp";
	FILE *file = fopen(filename, "w");
	fwrite(store_array, sizeof(uint16_t), WIDTH*HEIGHT, file);//dumps the binary to the file, decreasing the time it takes by a factor 60.
	fclose(file);
	rename("/dev/shm/framebuffer.tmp", "/dev/shm/framebuffer");
}
void read_files_multi(uint16_t store_array[HEIGHT][WIDTH], uint8_t mode) {
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
	FILE *file = fopen(filename, "r"); //reads the binary data.
	fread(store_array, sizeof(uint16_t), WIDTH*HEIGHT, file);
	fclose(file);
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
			framebuffer[y][x] = 0; //black
		}
	}
	uint16_t graphbuffer[HEIGHT_GRAPH][WIDTH_GRAPH];
	for (int y = 0; y < HEIGHT_GRAPH; y++) {
		for (int x = 0; x < WIDTH_GRAPH; x++) {
			graphbuffer[y][x] = 0; //black
		}
	}
	for (int k =0; k<240; k++) {
		framebuffer[139][k] = 0xff00;
	}
	for (int k =0; k <240; k++) {
		framebuffer[k][138] = 0xff00;
	}
	uint16_t framebuffer_drawn[HEIGHT][WIDTH];
	int b0 = 0;
	pynq_init();
	gpio_init();
	switches_init();
	buttons_init();
    int fork_num = 0;
    int fork_state = 1;
	write_files_multi(framebuffer); // ensures that if the display program starts to fast, it does not segfault
	sleep_msec(1000); //prevent a segfault
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
	switch (fork_num){
		case 0:
        	while(1){
				for (int k = 0; k <NUMBER_OF_MOVES_PER_FRAME; k++){
					for (int x = 1; x < WIDTH_GRAPH; x++) {
						//sleep_msec(1);
						b0=get_button_state(0);
						if (b0 == 0) {
							graphbuffer[1][x] = 0xff00;
							graphbuffer[99][x] =0;
						}
						else if (b0 == 1) {
							graphbuffer[99][x] = 0xffff;
							graphbuffer[1][x] = 0;
						}
					}
					//memset(&graphbuffer[99][0], 0xff, WIDTH_GRAPH*sizeof(uint16_t));
                	/*if (b0 == 0){
                		graphbuffer[0][0] = 0xffff;
                	}
                	else if (b0 == 1){
                		graphbuffer[0][99] = 0xff00; // this is just for testing
                	}*/
				}

        		for (int y = 0; y < HEIGHT_GRAPH; y++) {
        			memcpy(&framebuffer[90+y][90], &graphbuffer[y][0], WIDTH_GRAPH*sizeof(uint16_t));
        		}
			memcpy(framebuffer_drawn, framebuffer, sizeof(framebuffer_drawn));
			write_files_multi(framebuffer_drawn);
        	}
		break;
		case 1:
			display_t display;
			display_init(&display);
			while(1){
				read_files_multi(framebuffer_drawn, FRAMEBUFFER);
	                display_flush(&display, (uint16_t*)framebuffer_drawn);
        	}
			display_destroy(&display);
		break;
		default:
			printf("To many forks");
			_Exit(1);
		break;
	}
	buttons_destroy();
	switches_destroy();
	pynq_destroy();
}
