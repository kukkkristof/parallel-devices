#include <BMPFILE.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>


int Read_BMP_From_File(BMPImage* image, char* path) 
{
    FILE* image_file = fopen(path, "rb");

    if(image_file == NULL)
    {
        // printf("File \"%s\" does no exist! (not ok)\n", path);
        return 1;
    }
    else
    {
        // printf("File \"%s\" does exist! (ok)\n", path);
        
    }

    fread(image->signature, 2, sizeof(char), image_file);
    fread(&(image->header), 1, sizeof(Header), image_file);


    image->signature[2] = '\0';
    
    // printf("File signature: %s (", image->signature);
    if(strcmp(image->signature, "BM") == 0)
    {
        // printf("ok)\n");
    }
    else
    {
        // printf("not ok)\n");
        return 2;
    }

    // print_header(image->header);

    

    fread(&(image->infoHeader), 1, sizeof(InfoHeader), image_file);

    // print_infoHeader(image->infoHeader);

    fseek(image_file, image->header.data_offset, SEEK_SET);

    image->imageData = malloc(image->infoHeader.image_height * image->infoHeader.image_width * (image->infoHeader.bits_per_pixel / 8));
    fread(image->imageData, 1,image->infoHeader.image_height * image->infoHeader.image_width * (image->infoHeader.bits_per_pixel / 8), image_file);

    fclose(image_file);
    return 0;
}

int Write_To_BMP_File(BMPImage* image, char* path)
{
    FILE *file;


    file = fopen(path, "wb");

    fwrite(&(image->signature), sizeof(char), 2, file);

    fwrite(&(image->header), sizeof(Header), 1, file);

    fwrite(&(image->infoHeader), image->infoHeader.size, 1, file);

    

    uint32_t num_data = image->infoHeader.image_height * image->infoHeader.image_width;


    fseek(file, image->header.data_offset, SEEK_SET);
    fwrite(image->imageData, num_data * sizeof(uint8_t) * 3, 1, file);
    

    fclose(file);
}

void print_header(Header header) {
    printf("File size: %lf MB\n", header.file_size / 1024.0 / 1024);
    printf("Data offset: %x\n", header.data_offset);
}

void print_infoHeader(InfoHeader infoHeader)
{
    printf("===========\nInfo Header\n===========\n");
    printf("InfoHeader Size: %u bytes\n", infoHeader.size);
    printf("Image width: %upx\n", infoHeader.image_width);
    printf("Image height: %upx\n", infoHeader.image_height);
    printf("Number of Planes: %u\n", infoHeader.num_of_planes);
    printf("Bits per Pixel: %u\n", infoHeader.bits_per_pixel);
    printf("Compression type: ");
    switch (infoHeader.compression_type)
    {
    case 0:
        printf("BI_RGB - No compression!\n");
        break;
    case 1:
        printf("BI_RLE8 - 8bit RLE Encoding!\n");
        break;
    case 2:
        printf("BI_RLE4 - 4bit RLE encoding!\n");
        break;
    default:
        printf("Unknown compression type!\n");    
        break;
    }
    printf("Image size: %u (Can be 0 if there is no compression)\n", infoHeader.image_size_when_compressed);
    printf("Horizontal Resolution: %u pixels / meter\n", infoHeader.pixels_per_meter_horizontal);
    printf("Vertical Resolution: %u pixels / meter\n", infoHeader.pixels_per_meter_vertical);
    printf("Colors used: %d\n", infoHeader.num_of_colors);
    printf("Important colors: %d\n", infoHeader.num_of_important_colors);
}


void convert_to_RGB(RGB* targetArray, uint8_t* data, uint32_t num_elements_of_target)
{
    for(int i = 0; i < num_elements_of_target; i++)
    {
        targetArray[i].Red = data[i*3+2];
        targetArray[i].Green = data[i*3+1];
        targetArray[i].Blue = data[i*3];
    }
}

void convert_to_binary(uint8_t* targetArray, RGB* data, uint32_t num_elements_of_data)
{
    for(int i = 0; i < num_elements_of_data; i++)
    {
        targetArray[i*3 + 2] = data[i].Red;
        targetArray[i*3 + 1] = data[i].Green;
        targetArray[i*3] = data[i].Blue;
    }
}