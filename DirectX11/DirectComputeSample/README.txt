This is a minimal Direct3D11 example (in C++) that fills two input buffers
with data, copies this data to the GPU, performs a "saxpy" (z=a*x+y)
operation on two buffers of single-precision floating point numbers,
reads the results back and compares them to results calculated on the
CPU.

There are TODO comments in places where you might want to consider
making changes if you'll be using this code as a starting point for
something more complicated.

To run: Open the Visual Studio 2010 project DirectComputeSample.vcxproj and
hit F5.  You may first want to set a breakpoint at the last statement
in main(), as all output will be written to the console, not to the
output debug window in Visual Studio.