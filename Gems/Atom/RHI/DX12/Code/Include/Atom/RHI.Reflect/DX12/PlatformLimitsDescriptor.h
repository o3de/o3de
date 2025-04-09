/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RHI.Reflect/PlatformLimitsDescriptor.h>
#include <AzCore/Preprocessor/Enum.h>
#include <AzCore/std/containers/unordered_map.h>

namespace AZ
{
    class ReflectContext;

    namespace DX12
    {
        //Should match D3D12_DESCRIPTOR_HEAP_TYPE
        AZ_ENUM(DESCRIPTOR_HEAP_TYPE,
            DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
            DESCRIPTOR_HEAP_TYPE_SAMPLER,
            DESCRIPTOR_HEAP_TYPE_RTV,
            DESCRIPTOR_HEAP_TYPE_DSV);

        struct FrameGraphExecuterData
        {
            AZ_TYPE_INFO(AZ::DX12::FrameGraphExecuterData, "{C21547F6-DE48-4F82-B812-1A187101AB4E}");
            static void Reflect(AZ::ReflectContext* context);

            //Cost per draw/dispatch item
            uint32_t m_itemCost = 1;

            // Cost per Attachment
            uint32_t m_attachmentCost = 8;

            // Maximum number of swapchains per commandlist
            uint32_t m_swapChainsPerCommandList = 8;

            // The maximum cost that can be associated with a single command list.
            uint32_t m_commandListCostThresholdMin = 250;

            // The maximum number of command lists per scope.
            uint32_t m_commandListsPerScopeMax = 16;
        };

        //! A descriptor used to configure limits for each backend
        class PlatformLimitsDescriptor final
            : public RHI::PlatformLimitsDescriptor
        {
            using Base = RHI::PlatformLimitsDescriptor;
        public:
            AZ_RTTI(AZ::DX12::PlatformLimitsDescriptor, "{ADCC8071-FCE4-4FA1-A048-DF8982951A0D}", Base);
            AZ_CLASS_ALLOCATOR(AZ::DX12::PlatformLimitsDescriptor, AZ::SystemAllocator);
            static void Reflect(AZ::ReflectContext* context);

            PlatformLimitsDescriptor() = default;

            static const uint32_t NumHeapFlags = 2;// D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE + 1;

            //! string key: string version of DESCRIPTOR_HEAP_TYPE.
            //! int array: Max count for descriptors 
            AZStd::unordered_map<AZStd::string, AZStd::array<uint32_t, NumHeapFlags>> m_descriptorHeapLimits;

            //! Denote portion of the shader-visible descriptor heap used to maintain static handles.
            //! NOTE: dynamic descriptors are needed to allocate per-frame descriptor tables for resources that are
            //! not bound via bindless, so this number should reflect that. If the majority of resources correctly
            //! leverage the bindless mechanism, this number can be higher (e.g. [0.8f, 0.9f]).
            float m_staticDescriptorRatio = 0.5f;

            FrameGraphExecuterData m_frameGraphExecuterData;

            void LoadPlatformLimitsDescriptor(const char* rhiName) override;
        };
    }
}
