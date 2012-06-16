// GROUP_SIZE_X specifies the number of threads in a group; this
// must be the same number as the constant value groupSize defined
// in the CPU code.
#define GROUP_SIZE_X 512

cbuffer Constants
{
    float a;
};

StructuredBuffer<float> x;
StructuredBuffer<float> y;
RWStructuredBuffer<float> z;

[numthreads(GROUP_SIZE_X, 1, 1)]
void saxpy(uint3 threadIDInGroup : SV_GroupThreadID, 
    uint3 groupID : SV_GroupID, 
    uint groupIndex : SV_GroupIndex, 
    uint3 dispatchThreadID : SV_DispatchThreadID)
{
    // Compute the index of the element to be processed by this thread.
    int n = groupID.x*GROUP_SIZE_X + threadIDInGroup.x;
 
    // Compute the output value z from input buffers x, y and the constant
    // value a (which is defined up above in the "Constants" buffer).
    z[n] = a*x[n] + y[n];
}
