#define _GNU_SOURCE
#include <libpynq.h>
#include <unistd.h>
#include <display.c>
#define HEIGHT 240 //y coordinates
#define WIDTH 240

void write_files_multi(uint16_t store_array[HEIGHT][WIDTH]){
    char filename[] = "/dev/shm/framebuffer.tmp";
    FILE *file = fopen(filename, "w");
    fwrite(store_array, sizeof(uint16_t), WIDTH*HEIGHT, file);//dumps the binary to the file, decreasing the time it takes by a factor 60.
    fclose(file);
    rename("/dev/shm/framebuffer.tmp", "/dev/shm/framebuffer");
}
void read_files_multi_raw(uint16_t store_array[HEIGHT][WIDTH]) {
    char filename[2048];
    rename("/dev/shm/framebuffer", "/dev/shm/framebuffer.reading"); //to hopefully prevent race conditions, by decreasing the change of it happening.
    strcpy(filename, "/dev/shm/framebuffer.reading");
    FILE *file = fopen(filename, "r"); //reads the binary data.
    fread(store_array, sizeof(uint16_t), WIDTH*HEIGHT, file);
    fclose(file);
}

void read_files_multi(uint16_t store_array[HEIGHT][WIDTH]) {
    char filename[2048];
    strcpy(filename, "framebuffer");
    FILE *file = fopen(filename, "r");
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            fscanf(file, " %hu", &store_array[y][x]);
        }
    }
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

int main() {
    pynq_init();
    gpio_init();
    switches_init();
    buttons_init();
    display_t display;
    display_init(&display);
    int operation = 0;
    uint16_t store_array[HEIGHT][WIDTH];
    uint16_t framebuffer[HEIGHT][WIDTH];
    while (operation != 1) {
        printf("Enter operation code: ");
        scanf(" %d", &operation);
        switch (operation) {
            case 2:
                read_files_multi(store_array);
                write_files_multi(store_array);
                read_files_multi_raw(framebuffer);
                display_flush(&display, (uint16_t*)framebuffer);
                break;

        }
    }
    display_destroy(&display);
    buttons_destroy();
    switches_destroy();
    pynq_destroy();
}