build:
	gcc main.c src/kernel_loader.c src/BMPFILE.c -o main.exe -Iinclude -lOpenCL -w -lpthread

build_and_run:
	gcc main.c src/kernel_loader.c src/BMPFILE.c -o main.exe -Iinclude -lOpenCL -w -g -lpthread
	cls
	main.exe