/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#pragma once

#include <Atom/RHI/RayTracingPipelineState.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

namespace AZ
{
    namespace DX12
    {
        class RayTracingPipelineState final
            : public RHI::RayTracingPipelineState
        {
        public:
            AZ_CLASS_ALLOCATOR(RayTracingPipelineState, AZ::SystemAllocator, 0);

            static RHI::Ptr<RayTracingPipelineState> Create();

            /// Returns the platform ray tracing pipeline state object
#ifdef AZ_DX12_DXR_SUPPORT
            ID3D12StateObject* Get() const;
#endif
            ID3D12RootSignature* GetGlobalRootSignature() const { return m_globalRootSignature.get(); }

        private:
            RayTracingPipelineState() = default;

            // creates a ID3D12RootSignature object from a RayTracingRootSignature
            void CreateRootSignature(RHI::Device& deviceBase,
                                     const RHI::RayTracingRootSignature& rayTracingRootSignature,
                                     bool isLocalRootSignature,
                                     Microsoft::WRL::ComPtr<ID3D12RootSignature>& rootSignatureComPtr);

            //////////////////////////////////////////////////////////////////////////
            // RHI::PipelineState
            RHI::ResultCode InitInternal(RHI::Device& deviceBase, const RHI::RayTracingPipelineStateDescriptor* descriptor) override;
            void ShutdownInternal() override;
            //////////////////////////////////////////////////////////////////////////

            RHI::Ptr<ID3D12RootSignature> m_globalRootSignature;
            AZStd::vector<RHI::Ptr<ID3D12RootSignature>> m_localRootSignatures;
#ifdef AZ_DX12_DXR_SUPPORT
            RHI::Ptr<ID3D12StateObject> m_rayTracingPipelineState;
#endif
        };
    }
}
