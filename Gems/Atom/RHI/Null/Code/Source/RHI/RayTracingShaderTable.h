/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/DeviceRayTracingShaderTable.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>


namespace AZ
{
    namespace Null
    {
        class Buffer;

        class RayTracingShaderTable final
            : public RHI::DeviceRayTracingShaderTable
        {
        public:
            AZ_CLASS_ALLOCATOR(RayTracingShaderTable, AZ::SystemAllocator);
            static RHI::Ptr<RayTracingShaderTable> Create();

        private:
            RayTracingShaderTable() = default;

            //////////////////////////////////////////////////////////////////////////
            // RHI::DeviceRayTracingShaderTable
            RHI::ResultCode BuildInternal() override {return RHI::ResultCode::Success;}
            //////////////////////////////////////////////////////////////////////////
        };
    }
}
