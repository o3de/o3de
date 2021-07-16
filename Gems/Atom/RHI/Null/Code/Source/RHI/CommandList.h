/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <Atom/RHI/CommandList.h>
#include <Atom/RHI.Reflect/Base.h>
#include <Atom/RHI/DeviceObject.h>

namespace AZ
{
    namespace Null
    {
        class CommandList final
            : public RHI::CommandList
            , public RHI::DeviceObject 
        {
            using Base = RHI::CommandList;
        public:
            AZ_CLASS_ALLOCATOR(CommandList, AZ::SystemAllocator, 0);
            AZ_RTTI(CommandList, "{265B877D-21DC-48FF-9D82-AA30C3E16422}", Base);

            static RHI::Ptr<CommandList> Create();

            //////////////////////////////////////////////////////////////////////////
            // RHI::CommandList
            void SetViewports([[maybe_unused]] const RHI::Viewport* viewports, [[maybe_unused]] uint32_t count) override {}
            void SetScissors([[maybe_unused]] const RHI::Scissor* scissors, [[maybe_unused]] uint32_t count) override {}
            void SetShaderResourceGroupForDraw([[maybe_unused]] const RHI::ShaderResourceGroup& shaderResourceGroup) override {}
            void SetShaderResourceGroupForDispatch([[maybe_unused]] const RHI::ShaderResourceGroup& shaderResourceGroup) override {}
            void Submit([[maybe_unused]] const RHI::DrawItem& drawItem) override {}
            void Submit([[maybe_unused]] const RHI::CopyItem& copyItem) override {}
            void Submit([[maybe_unused]] const RHI::DispatchItem& dispatchItem) override {}
            void Submit([[maybe_unused]] const RHI::DispatchRaysItem& dispatchRaysItem) override {}
            void BeginPredication([[maybe_unused]] const RHI::Buffer& buffer, [[maybe_unused]] uint64_t offset, [[maybe_unused]] RHI::PredicationOp operation) override {}
            void EndPredication() override {}
            void BuildBottomLevelAccelerationStructure([[maybe_unused]] const RHI::RayTracingBlas& rayTracingBlas) override {}
            void BuildTopLevelAccelerationStructure([[maybe_unused]] const RHI::RayTracingTlas& rayTracingTlas) override {}
            /////////////////////////////////////////////////////////////////////////////////////////////////

            ///////////////////////////////////////////////////////////////////
            // RHI::DeviceObject
            void Shutdown() override;
            ///////////////////////////////////////////////////////////////////
        };
    }
}
