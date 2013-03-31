This is a Direct3D11.1 example (in C++) that shows how to enumerate
the available adapters on a system and create devices and contexts
for those associated with a hardware device.

There are TODO comments in places where you might want to consider
making changes if you'll be using this code as a starting point for
something more complicated.

To run: Open the Visual Studio 2012 project DirectComputeSample.vcxproj and
hit F5.  You may first want to set a breakpoint at the last statement
in main(), as all output will be written to the console, not to the
output debug window in Visual Studio.