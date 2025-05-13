#ifndef BMPFILE_H
#define BMPFILE_H

#include <stdint.h>


typedef struct Header{
    uint32_t file_size;
    uint32_t reserved;
    uint32_t data_offset;
} Header;

typedef struct InfoHeader{
    uint32_t size;
    uint32_t image_width;
    uint32_t image_height;
    uint16_t num_of_planes;
    uint16_t bits_per_pixel;
    uint32_t compression_type;
    uint32_t image_size_when_compressed;
    uint32_t pixels_per_meter_horizontal;
    uint32_t pixels_per_meter_vertical;
    uint32_t num_of_colors;
    uint32_t num_of_important_colors;
} InfoHeader;

typedef struct BMPImage
{
    char signature[2];
    Header header;
    InfoHeader infoHeader;
    uint8_t* imageData;
} BMPImage;

typedef struct RGB{
    unsigned int Red;
    unsigned int Green;
    unsigned int Blue;
} RGB;

int Read_BMP_From_File(BMPImage* image, char* path);
int Write_To_BMP_File(BMPImage* image, char* path);
void print_header(Header header);
void print_infoHeader(InfoHeader infoHeader);
void convert_to_RGB(RGB* targetArray, uint8_t* data, uint32_t num_elements);
void convert_to_binary(uint8_t* targetArray, RGB* data, uint32_t num_elements_of_data);

#endif /* BMPFILE_H */