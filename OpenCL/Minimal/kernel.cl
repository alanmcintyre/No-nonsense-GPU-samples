// This sample kernel computes z = a*x + y.
// It is assumed that z, x, and y are all vectors of the same size.

__kernel void saxpy(__global float const* x, __global float const* y, 
    __global float* z, float a)
{
    // Get element index n.
    int n = get_global_id(0);

    z[n] = a*x[n] + y[n];
}

