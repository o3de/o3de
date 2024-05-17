/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <RHI/PipelineLayout.h>
#include <Atom/RHI/DevicePipelineState.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

namespace AZ
{
    namespace DX12
    {
        struct PipelineStateDrawData
        {
            RHI::MultisampleState m_multisampleState;
            RHI::PrimitiveTopology m_primitiveTopology = RHI::PrimitiveTopology::Undefined;
        };

        struct PipelineStateData
        {
            PipelineStateData()
                : m_type(RHI::PipelineStateType::Draw)
            {}

            RHI::PipelineStateType m_type;
            union
            {
                // Only draw data for now.
                PipelineStateDrawData m_drawData;
            };
        };        

        class PipelineState final
            : public RHI::DevicePipelineState
        {
            friend class PipelineStatePool;
        public:
            AZ_CLASS_ALLOCATOR(PipelineState, AZ::SystemAllocator);

            static RHI::Ptr<PipelineState> Create();

            /// Returns the pipeline layout associated with this PSO.
            const PipelineLayout* GetPipelineLayout() const;

            /// Returns the platform pipeline state object.
            ID3D12PipelineState* Get() const;

            const PipelineStateData& GetPipelineStateData() const;

        private:
            PipelineState() = default;

            //////////////////////////////////////////////////////////////////////////
            // RHI::DevicePipelineState
            RHI::ResultCode InitInternal(RHI::Device& device, const RHI::PipelineStateDescriptorForDraw& descriptor, RHI::DevicePipelineLibrary* pipelineLibrary) override;
            RHI::ResultCode InitInternal(RHI::Device& device, const RHI::PipelineStateDescriptorForDispatch& descriptor, RHI::DevicePipelineLibrary* pipelineLibrary) override;
            RHI::ResultCode InitInternal(RHI::Device& device, const RHI::PipelineStateDescriptorForRayTracing& descriptor, RHI::DevicePipelineLibrary* pipelineLibrary) override;
            void ShutdownInternal() override;
            //////////////////////////////////////////////////////////////////////////

            RHI::ConstPtr<PipelineLayout> m_pipelineLayout;
            RHI::Ptr<ID3D12PipelineState> m_pipelineState;
            PipelineStateData m_pipelineStateData;
        };
    }
}
