/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <RHI/RayTracingClusterBlas.h>

namespace AZ
{
    namespace DX12
    {
        RHI::Ptr<RayTracingClusterBlas> RayTracingClusterBlas::Create()
        {
            return aznew RayTracingClusterBlas;
        }

        uint64_t RayTracingClusterBlas::GetAccelerationStructureByteSize()
        {
            // TODO(CLAS): Implement this
            return 0;
        }

        RHI::ResultCode RayTracingClusterBlas::CreateBuffersInternal(
            [[maybe_unused]] RHI::Device& deviceBase,
            [[maybe_unused]] const RHI::DeviceRayTracingClusterBlasDescriptor* descriptor,
            [[maybe_unused]] const RHI::DeviceRayTracingBufferPools& bufferPools)
        {
            // TODO(CLAS): Implement this
            return RHI::ResultCode::Unimplemented;
        }
    } // namespace DX12
} // namespace AZ
