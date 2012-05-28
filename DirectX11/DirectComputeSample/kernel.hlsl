struct BufferStruct
{
    uint4 color;
};

cbuffer consts {
    uint4 const_color;
};
cbuffer veryconsts {
    uint4 very_const_color;
};

// group size
#define thread_group_size_x 4
#define thread_group_size_y 4
RWStructuredBuffer<BufferStruct> g_OutBuff;

/* This is the number of threads in a thread group, 4x4x1 in this example case */
// e.g.: [numthreads( 4, 4, 1 )]
[numthreads( thread_group_size_x, thread_group_size_y, 1 )]

void main( uint3 threadIDInGroup : SV_GroupThreadID, uint3 groupID : SV_GroupID, 
         uint groupIndex : SV_GroupIndex, 
         uint3 dispatchThreadID : SV_DispatchThreadID )
{
    int N_THREAD_GROUPS_X = 16;  // assumed equal to 16 in dispatch(16,16,1)

    int stride = thread_group_size_x * N_THREAD_GROUPS_X;  
                // buffer stide, assumes data stride = data width (i.e. no padding)

    int idx = dispatchThreadID.y * stride + dispatchThreadID.x;

    uint4 color = uint4( groupID.x, groupID.y , const_color.x, very_const_color.x );

    g_OutBuff[ idx ].color = color;
}
