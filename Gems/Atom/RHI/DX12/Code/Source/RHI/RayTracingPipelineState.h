/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/DeviceRayTracingPipelineState.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <RHI/DX12.h>

namespace AZ
{
    namespace DX12
    {
        class RayTracingPipelineState final
            : public RHI::DeviceRayTracingPipelineState
        {
        public:
            AZ_CLASS_ALLOCATOR(RayTracingPipelineState, AZ::SystemAllocator);

            static RHI::Ptr<RayTracingPipelineState> Create();

            /// Returns the platform ray tracing pipeline state object
#ifdef AZ_DX12_DXR_SUPPORT
            ID3D12StateObject* Get() const;
#endif
            ID3D12RootSignature* GetGlobalRootSignature() const { return m_globalRootSignature.get(); }

        private:
            RayTracingPipelineState() = default;

            //////////////////////////////////////////////////////////////////////////
            // RHI::DevicePipelineState
            RHI::ResultCode InitInternal(RHI::Device& deviceBase, const RHI::DeviceRayTracingPipelineStateDescriptor* descriptor) override;
            void ShutdownInternal() override;
            //////////////////////////////////////////////////////////////////////////

            RHI::Ptr<ID3D12RootSignature> m_globalRootSignature;
#ifdef AZ_DX12_DXR_SUPPORT
            RHI::Ptr<ID3D12StateObject> m_rayTracingPipelineState;
#endif
        };
    }
}
