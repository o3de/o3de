__kernel void ComputePartialVolumes(__global short4 * voxels,
                                    const    int      numVoxels,
                                    const    float4   plane,
                                    const    float4   minBB,
                                    const    float4   scale,
                                    __local  uint4 *  localPartialVolumes,
                                    __global uint4 *  partialVolumes)
{
    int localId = get_local_id(0);
    int groupSize = get_local_size(0);
    int i0 = get_global_id(0) << 2;
    float4 voxel;
    uint4  v;

    voxel = convert_float4(voxels[i0]);
    v.s0 = (dot(plane, mad(scale, voxel, minBB)) >= 0.0f) * (i0 < numVoxels);
    voxel = convert_float4(voxels[i0 + 1]);
    v.s1 = (dot(plane, mad(scale, voxel, minBB)) >= 0.0f) * (i0 + 1 < numVoxels);
    voxel = convert_float4(voxels[i0 + 2]);
    v.s2 = (dot(plane, mad(scale, voxel, minBB)) >= 0.0f) * (i0 + 2 < numVoxels);
    voxel = convert_float4(voxels[i0 + 3]);
    v.s3 = (dot(plane, mad(scale, voxel, minBB)) >= 0.0f) * (i0 + 3 < numVoxels);

    localPartialVolumes[localId] = v;
    barrier(CLK_LOCAL_MEM_FENCE);

    for (int i = groupSize >> 1; i > 0; i >>= 1)
    {
        if (localId < i)
        {
            localPartialVolumes[localId] += localPartialVolumes[localId + i];
        }
        barrier(CLK_LOCAL_MEM_FENCE);
    }
    if (localId == 0)
    {
        partialVolumes[get_group_id(0)] = localPartialVolumes[0];
    }
}
__kernel void ComputePartialSums(__global uint4 * data,
                                   const  int     dataSize,
                                 __local  uint4 * partialSums) 
{

    int globalId  = get_global_id(0);
    int localId   = get_local_id(0);
    int groupSize = get_local_size(0);
    int i;
    if (globalId < dataSize)
    {
        partialSums[localId] = data[globalId];
    }
    else
    {
        partialSums[localId] = (0, 0, 0, 0);
    }
    barrier(CLK_LOCAL_MEM_FENCE);
    for (i = groupSize >> 1; i > 0; i >>= 1)
    {
        if (localId < i)
        {
            partialSums[localId] += partialSums[localId + i];
        }
        barrier(CLK_LOCAL_MEM_FENCE);
    }
    if (localId == 0)
    {
        data[get_group_id(0)] = partialSums[0];
    }
}

