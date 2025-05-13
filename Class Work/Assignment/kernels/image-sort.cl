typedef struct RGB{
    uint Red;
    uint Green;
    uint Blue;
} RGB;

void swap_rgb(__global RGB* arr, int a, int b)
{
    RGB tmp = arr[a];
    arr[a] = arr[b];
    arr[b] = tmp;
}

__kernel void vertical_sort(__global RGB* buffer, int width, int height, int ascending)
{
    int x = get_global_id(0);
    if (x >= width) return;
    int swapped = 1;
    while (swapped)
    {
        swapped = 0;
        for (int y = 1; y < height; y++)
        {
            int idx1 = x + (y - 1) * width;
            int idx2 = x + y * width;

            RGB rgb1 = buffer[idx1];
            RGB rgb2 = buffer[idx2];

            uint sum1 = rgb1.Red + rgb1.Green + rgb1.Blue;
            uint sum2 = rgb2.Red + rgb2.Green + rgb2.Blue;

            int do_swap = ascending ? (sum1 > sum2) : (sum1 < sum2);
            if (do_swap)
            {
                buffer[idx1] = rgb2;
                buffer[idx2] = rgb1;
                swapped = 1;
            }
        }
    }
}

