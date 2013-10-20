include ../opencl-config.mk

OpenCLMinimal: OpenCLMinimal.cpp
	$(CC) OpenCLMinimal.cpp -g -Wall -I$(OPENCL_INCLUDE) -o OpenCLMinimal -lOpenCL -lstdc++

clean:
	rm -f OpenCLMinimal