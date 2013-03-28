No-nonsense-GPU-samples
=======================

A small collection of GPU computing samples, free of complicated
utility library clutter.

I started writing these samples because I was tired of having to wade
through oceans of DXUT/oclUtils/shrQATest calls, or trace through
mazes of twisty little makefiles, all alike, just to figure out how to
do some particular thing with a GPGPU API. I figured other people
might like to see them (and if they're here I'm less likely to lose
them).

These samples are all standalone, so that you don't have to go on a
wild goose chase to figure out what's being done.  As much as
possible, everything is done in a straightforward sequence of steps,
all in the main() function.

If you find a bug, can think of a way to make one of the samples
clearer, or if you can think of some other samples that would be
useful, please let me know.

I've only run these samples on NVIDIA GTX 680 and Radeon 6870HD, so please
let me know if you encounter any issues on other hardware.

The samples with "Win8" at the end of the folder name were built with Visual
Studio 2012 on Windows 8/DirectX11.1.  They should run with minor modification
on DirectX11 and/or Visual Studio 2010.  