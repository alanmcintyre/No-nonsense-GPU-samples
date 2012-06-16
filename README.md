No-nonsense-GPU-samples
=======================

A small collection of GPU computing samples, free of complicated
utility library clutter.

I started writing these samples because I was tired of having to wade
through oceans of DXUT/oclUtils/shrQATest calls, or trace through
mazes of twisty little makefiles, all alike, just to figure out
how to do some particular thing with a GPGPU API. I figured other
people might like to see them (and if they're here I'm less likely to 
lose them).

These samples are all standalone, so that you don't have to go on a
wild goose chase to figure out what's being done.  As much as
possible, everything is done in a straightforward sequence of steps,
all in the main() function.

If you find a bug, can think of a way to make one of the samples
clearer or less cluttered, or if you can think of some other samples
that would be useful, please let me know.

For now, I've only tested these samples on NVIDIA hardware.