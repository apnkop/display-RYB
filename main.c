#define _GNU_SOURCE
#include <libpynq.h>
#include <unistd.h>
#include <display.c>
#define HEIGHT 240
#define WIDTH 240
#define NUMBER_OF_MOVES_PER_FRAME 1000 // this draws a 1000 frames before pushing one to the display. This will be fixed
#define NUMBER_OF_CHILDS 1
#define FRAME_BYTES (HEIGHT * WIDTH * sizeof(uint16_t))

void write_full(int fd, const void *buf, size_t n) {
    const uint8_t *p = (const uint8_t*)buf;
    size_t left = n;
    while (left > 0) {
        ssize_t w = write(fd, p, left);
        p += w;
        left -= (size_t)w;
    }
}

void read_full(int fd, void *buf, size_t n) {
    uint8_t *p = (uint8_t*)buf;
    size_t left = n;
    while (left > 0) {
        ssize_t r = read(fd, p, left);
        p += r;
        left -= (size_t)r;
    }
}

void displayDrawMultiPixelsv2(display_t *display, uint16_t x, uint16_t y, uint16_t sizex, uint16_t sizey, uint16_t *colors) {
	uint16_t _x1 = x + display->_offsetx;
	uint16_t _x2 = _x1 + sizex;
	uint16_t _y1 = y + display->_offsety;
	uint16_t _y2 = _y1 + sizey;
	uint16_t size = sizey*sizex;
	spi_master_write_command(display, 0x2A); // set column(x) address
	spi_master_write_addr(display, _x1, _x2); //it should be possible to update only the changed pixels, I dont feel like fixing it now.
	spi_master_write_command(display, 0x2B); // set Page(y) address
	spi_master_write_addr(display, _y1, _y2);
	spi_master_write_command(display, 0x2C); // memory write
	spi_master_write_colors(display, colors, size); //write to the display, I cant figure out how to increase spead more:-(
}
void display_flush(display_t *d, uint16_t *fb){
	displayDrawMultiPixelsv2(d, 0, 0, WIDTH, HEIGHT, fb);
}
int main (void){
	uint16_t framebuffer[HEIGHT][WIDTH];
	for(int y = 0; y < HEIGHT; y++){ //set framebuffer black
		for(int x =0; x < WIDTH; x++){
			framebuffer[y][x] = 0x0000;
		}
	}
	uint16_t framebuffer_drawn[HEIGHT][WIDTH];
	int b0 = 0;
        pynq_init();
	gpio_init();
	switches_init();
	buttons_init();
        int fork_num = 0;
        int fork_state = 1;
	int pipe_frame_transfer[2];
	pipe(pipe_frame_transfer);
        for (int i =0; i <NUMBER_OF_CHILDS; i ++){
		sleep_msec(100);
                if (fork_state != 0){
                        fork_state = fork();
                        fork_num = fork_num + 1;
                                if (fork_state != 0){
                                	printf("%s%i%s\n", "fork ", fork_num, " started");
                                }
                }
        }
	if (fork_state != 0){ //sets master branch to forknum 0, to prevent two forks running the same code.
		fork_num = 0;
	}
	switch (fork_num){
	case 0:
		close(pipe_frame_transfer[0]);
		nice(-20); //this requirs the program to run with sudo, but it increases the priority on linux, so it gets more cpu time./
        	while(1){
			for (int k = 0; k <NUMBER_OF_MOVES_PER_FRAME; k++){
                		memmove(&framebuffer[1][0], &framebuffer[0][0], (HEIGHT-1)*WIDTH*sizeof(uint16_t)); //move famebuffer from 0 to end, per row.
                		memset(&framebuffer[0][0], 0, WIDTH*sizeof(uint16_t));
                		b0 = get_button_state(0); //selfexplenetary
                		if (b0 == 0){
                		        	framebuffer[0][0] = 0xffff;
                		}
                		else if (b0 == 1){
                	        		framebuffer[0][100] = 0xff00;
                		}
			}
			memcpy(framebuffer_drawn, framebuffer, sizeof(framebuffer_drawn));
			fork_state = fork();
			if (fork_state == 0){
				write_full(pipe_frame_transfer[1], framebuffer_drawn, FRAME_BYTES);
				_Exit(0);
			}
        	}
		close(pipe_frame_transfer[1]);
		break;
	case 1:
		display_t display;
		display_init(&display);
		close(pipe_frame_transfer[1]);
		while(1){
			read_full(pipe_frame_transfer[0], framebuffer_drawn, FRAME_BYTES);
	                display_flush(&display, (uint16_t*)framebuffer_drawn);
        	}
		display_destroy(&display);
		break;
	}
	buttons_destroy();
	switches_destroy();
        pynq_destroy();
}

