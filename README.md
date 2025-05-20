# Parallel Devices

## Assignment
For my Assignment I made an image pixel sorter which sorts pixels in a vertical line.

### How it works:
For this I made a custom BMP image class, with RGB representation. I store the aforementioned RGB values in an array which has a size of image_width * image_height. From this we can deduce that from 0(inclusive) to image_width(exclusive) we have a single row of pixels, and from image_width(inc.) to 2* image_width(exc.) we have an other row of pixels and so on.

With this we can go throught the vertical lines the following way:
We take a vertical line index (ranging 0 to image_width) and with the selected sorting algorigthm we compare the pixel with one that is image_width away in the array. We compare these pixels and dependent on the sorting direction (ascending or descending) we swap those pictures. We do this iteratively until we reach the top of the image(we start from the bottom row). Once reached we can go to a next line and do the same over again but with the horizonal axis shifted by one. This program can easily be parallelized with checking different rows on a different CPU.

Dependent on the CPUS given the speed with which this process can be done varies drastically. The administrative costs, and thread count should be taken into consideration.



Main idea: https://www.youtube.com/watch?v=HMmmBDRy-jE (with shaders)

### Kristóf Kukk (P2MZHY)