include ../opencl-config.mk

OpenCLMinimal: OpenCLMinimal.c
	$(CC) OpenCLMinimal.c -g -Wall -I$(OPENCL_INCLUDE) -o OpenCLMinimal -lOpenCL -std=c99

clean:
	rm -f OpenCLMinimal