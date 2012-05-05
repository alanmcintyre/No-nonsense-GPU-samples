#include <stdio.h>
#include <CL/opencl.h>

int main(int const /*argc*/, char const** /*argv*/)
{
    // This example operates on buffers of size 32k elements, processing
    // blocks of 512 elements at a time.

    // TODO: Depending on your hardware and the amount of work the kernel
    // does for each element, you may want to alter the blocksize.  Of
    // course, if you aren't processing 32k elements, you need to adjust
    // the number of blocks and block size accordingly.
    unsigned int const blockSize = 512;
    unsigned int const blocks = 64;
    size_t const dimension = blocks*blockSize;

    // Get the list of platforms.
    int const maxPlatformCount = 8;
    cl_platform_id platforms[maxPlatformCount];
    cl_uint numPlatforms = 0;
    cl_int r = clGetPlatformIDs(maxPlatformCount, &platforms[0], &numPlatforms);
    if (r != CL_SUCCESS)
    {
        printf("clGetPlatformIDs failed with return code %d\n", r);
        return r;
    }

    // TODO: You may want to look at the list of platforms that are
    // returned, and choose the most appropriate one for your needs.
    int const platformToUse = 0;

    // Get the devices available for the chosen platform.
    cl_uint deviceCount = 0;
    size_t const maxDeviceCount = 8;
    cl_device_id devices[maxDeviceCount];
    r = clGetDeviceIDs(platforms[platformToUse], CL_DEVICE_TYPE_GPU,
                       maxDeviceCount, &devices[0], &deviceCount);
    if (r != CL_SUCCESS)
    {
        printf("clGetDeviceIDs failed with return code %d\n", r);
        return r;
    }

    // Create the context, using all devices.
    cl_context context = clCreateContext(0, deviceCount, &devices[0], NULL,
                                         NULL, &r);
    if (0 == context || CL_SUCCESS != r)
    {
        printf("clCreateContext failed with return value %p and code %d\n",
               context, r);
        return r;
    }

    // TODO: If you have multiple devices, you may specify which one
    // you'd like to use by changing this variable.
    int const deviceToUse = 0;

    // Create a command queue for the selected device.
    cl_command_queue commandQueue = clCreateCommandQueue(context,
                                    devices[deviceToUse], 0, &r);
    if (0 == commandQueue || CL_SUCCESS != r)
    {
        printf("clCreateCommandQueue failed with return value %p and code %d\n",
               commandQueue, r);
        return r;
    }

    // Read the kernel source.
    // TODO: If you want to open a different
    // kernel, you need to change the filename here.  It might also be
    // convenient to factor the kernel loading and compilation into a
    // separate function.
    FILE* kernelFile = fopen("kernel.cl", "rb");
    if (NULL == kernelFile)
    {
        printf("Unable to open kernel source file\n");
        return 1;
    }
    r = fseek(kernelFile, 0, SEEK_END);
    if (0 != r)
    {
        printf("Unable to seek to end of kernel source file\n");
        return 2;
    }
    long kernelSize = ftell(kernelFile);
    r = fseek(kernelFile, 0, SEEK_SET);
    if (0 != r)
    {
        printf("Unable to seek to beginning of kernel source file\n");
        return 3;
    }
    char* kernelSource = new char[kernelSize+1];
    r = fread(kernelSource, 1, kernelSize, kernelFile);
    // NOTE: if the file is not opened in binary mode, the value
    // returned by ftell is not guaranteed to be the exact number
    // of bytes from the beginning of the file.
    if (kernelSize != r)
    {
        delete[] kernelSource;
        printf("Unable to read kernel source (%d items read instead of %ld)\n",
               r, kernelSize);
        return 4;
    }
    // Null-terminate the source string.
    kernelSource[kernelSize] = 0;

    // Create a program from kernel source text.
    const char* sourceLines[1] = {kernelSource};
    cl_program program = clCreateProgramWithSource(context, 1,
                         &sourceLines[0], NULL, &r);

    // Dispose of the kernel source we loaded from file.
    delete[] kernelSource;
    kernelSource = NULL;

    if (0 == program || CL_SUCCESS != r)
    {
        printf("clCreateProgramWithSource failed with return value %p and code %d\n",
               program, r);
        return r;
    }

    // Build the program
    r = clBuildProgram(program, 0, 0, 0, 0, 0);
    if (CL_SUCCESS != r)
    {
        printf("clBuildProgram failed with return value %d; error log:\n", r);

        // Get the build log and write to stdout
        char buildLog[1024*16];
        cl_int rlog = clGetProgramBuildInfo(program, devices[0], CL_PROGRAM_BUILD_LOG,
                                            sizeof(buildLog), buildLog, NULL );
        if (CL_SUCCESS == rlog)
        {
            printf("%s\n", buildLog);
        }
        else
        {
            printf("Unable to retrieve error log; clGetProgramBuildInfo failed with code %d.\n", rlog);
        }

        return r;
    }

    // Create the kernel
    cl_kernel kernel = clCreateKernel(program, "saxpy", &r);
    if (0 == kernel || CL_SUCCESS != r)
    {
        printf("clCreateKernel failed with return value %p and code %d\n",
               kernel, r);
        return r;
    }

    // Allocate host memory for input and output vectors
    float* x = new float[dimension];
    float* y = new float[dimension];
    float* z = new float[dimension];

    // Set values to something easy to verify
    for (unsigned int i = 0; i < dimension; ++ i)
    {
        x[i] = static_cast<float>(i);
        y[i] = 100 - static_cast<float>(i);
        z[i] = -static_cast<float>(i);
    }

    // Allocate memory on the device for vector x (read-only to the kernel)
    cl_mem devXmem = clCreateBuffer(context,
                                    CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, dimension*sizeof(cl_float),
                                    x, &r);
    if (0 == devXmem || CL_SUCCESS != r)
    {
        printf("clCreateBuffer failed with return value %p and code %d\n",
               devXmem, r);
        return r;
    }

    // Allocate memory on the device for vector y (read-only to the kernel)
    cl_mem devYmem = clCreateBuffer(context,
                                    CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, dimension*sizeof(cl_float),
                                    y, &r);
    if (0 == devYmem || CL_SUCCESS != r)
    {
        printf("clCreateBuffer failed with return value %p and code %d\n",
               devXmem, r);
        return r;
    }

    // Allocate memory on the device for vector z (write-only to the kernel)
    cl_mem devZmem = clCreateBuffer(context,
                                    CL_MEM_WRITE_ONLY, dimension*sizeof(cl_float),
                                    y, &r);
    if (0 == devZmem || CL_SUCCESS != r)
    {
        printf("clCreateBuffer failed with return value %p and code %d\n",
               devXmem, r);
        return r;
    }

    // Set kernel parameters

    // x vector
    r = clSetKernelArg(kernel, 0, sizeof(cl_mem), &devXmem);
    if (CL_SUCCESS != r)
    {
        printf("clSetKernelArg for x failed with return code %d\n", r);
        return r;
    }
    // y vector
    r = clSetKernelArg(kernel, 1, sizeof(cl_mem), &devYmem);
    if (CL_SUCCESS != r)
    {
        printf("clSetKernelArg for y failed with return code %d\n", r);
        return r;
    }
    // z vector
    r = clSetKernelArg(kernel, 2, sizeof(cl_mem), &devZmem);
    if (CL_SUCCESS != r)
    {
        printf("clSetKernelArg for z failed with return code %d\n", r);
        return r;
    }
    // The constant 'a'
    float a = 2.0f;
    r = clSetKernelArg(kernel, 3, sizeof(cl_float), &a);
    if (CL_SUCCESS != r)
    {
        printf("clSetKernelArg for a failed with return code %d\n", r);
        return r;
    }

    // Execute the kernel
    r = clEnqueueNDRangeKernel(commandQueue, kernel, 1, 0, &dimension, 0, 0, 0, 0);
    if (CL_SUCCESS != r)
    {
        printf("clEnqueueNDRangeKernel failed with return code %d\n", r);
        return r;
    }

    // Copy the results back to host memory
    r = clEnqueueReadBuffer(commandQueue, devZmem, CL_TRUE, 0, dimension*sizeof(cl_float), z, 0, 0, 0);
    if (CL_SUCCESS != r)
    {
        printf("clEnqueueReadBuffer failed with return code %d\n", r);
        return r;
    }

    // Check that results are correct.  Note that the code below
    // depends on the computation being exact, which may not be the
    // case for more complicated computations.
    for (unsigned int i = 0; i < dimension; ++ i)
    {
        if (x[i]*a + y[i] != z[i])
        {
            printf("Unexpected result at element %d:\n", (int)i);
            printf(" x[i]*a + y[i] = %f * %f + %f != z[i] = %f\n",
                   x[i], a, y[i], z[i]);
            return 100;
        }
    }
    printf("Computation appears to have completed successfully.\n");

    // Free memory
    delete[] x;
    x = NULL;
    delete[] y;
    y = NULL;
    delete[] z;
    z = NULL;

    // Free device memory
    r = clReleaseMemObject(devXmem);
    if (CL_SUCCESS != r)
    {
        printf("clReleaseMemObject failed with return code %d\n", r);
        return r;
    }
    r = clReleaseMemObject(devYmem);
    if (CL_SUCCESS != r)
    {
        printf("clReleaseMemObject failed with return code %d\n", r);
        return r;
    }
    r = clReleaseMemObject(devZmem);
    if (CL_SUCCESS != r)
    {
        printf("clReleaseMemObject failed with return code %d\n", r);
        return r;
    }

    // Release kernel, program, command queue, and context.
    r = clReleaseKernel(kernel);
    if (CL_SUCCESS != r)
    {
        printf("clReleaseKernel failed with return code %d\n", r);
        return r;
    }

    r = clReleaseProgram(program);
    if (CL_SUCCESS != r)
    {
        printf("clReleaseProgram failed with return code %d\n", r);
        return r;
    }

    r = clReleaseCommandQueue(commandQueue);
    if (CL_SUCCESS != r)
    {
        printf("clReleaseCommandQueue failed with return code %d\n", r);
        return r;
    }

    r = clReleaseContext(context);
    if (CL_SUCCESS != r)
    {
        printf("clReleaseContext failed with return code %d\n", r);
        return r;
    }

    return 0;
}
