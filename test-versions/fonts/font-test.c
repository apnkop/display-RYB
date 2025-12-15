#include <stdint.h>
#include <stdio.h>
#define WIDTH_CHAR 8
#define HEIGHT_CHAR 8
void print_array(uint16_t array[128][WIDTH_CHAR][HEIGHT_CHAR]) {
    for (uint8_t c = 0; c < 128; c++) {
        printf("%c\n", c);
        for (int i = 0; i <24; i++) {
            printf("%s", "-");
        }
        printf("\n");
        for (int x = 0; x < WIDTH_CHAR; x++) {
            for (int y = 0; y < HEIGHT_CHAR; y++) {
                switch (array[c][x][y]) {
                    case 0:
                        printf("%s", " ");
                        break;
                    default:
                        printf("%s", ".");
                }
            }
            printf("\n");
        }
    }
}
void read_fond(uint16_t fontarray[128][WIDTH_CHAR][HEIGHT_CHAR]) {
    char filename[] = "font.uint16";
    FILE *file = fopen(filename, "r"); //reads the binary data.
    for (uint8_t c = 0; c < 128; c++) {
            for (int x = 0; x < WIDTH_CHAR; x++) {
                for (int y = 0; y < HEIGHT_CHAR; y++) {
                 fscanf(file, " %hu", &fontarray[c][x][y]);
                }
            }
        }
    fclose(file);
}

int main() {
    uint16_t fontarray[128][WIDTH_CHAR][HEIGHT_CHAR];
    read_fond(fontarray);
    print_array(fontarray);
}