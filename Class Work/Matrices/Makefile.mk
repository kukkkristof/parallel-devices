build:
	gcc main.c src/kernel_loader.c -o main.exe -Iinclude -lOpenCL -w

build_and_run:
	gcc main.c src/kernel_loader.c -o main.exe -Iinclude -lOpenCL -w
	main.exe