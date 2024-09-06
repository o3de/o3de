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

namespace AZ::WebGPU
{
    class AZ::ReflectContext;

    struct FrameGraphExecuterData
    {
        AZ_TYPE_INFO(AZ::WebGPU::FrameGraphExecuterData, "{A811C1EF-8BE4-42CB-BFE4-DF8C9B36F980}");
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
        AZ_RTTI(AZ::WebGPU::PlatformLimitsDescriptor, "{AC5A875D-2707-443B-83CB-B0542837BE93}", Base);
        AZ_CLASS_ALLOCATOR(AZ::WebGPU::PlatformLimitsDescriptor, AZ::SystemAllocator);
        static void Reflect(AZ::ReflectContext* context);

        FrameGraphExecuterData m_frameGraphExecuterData;
    };
} // namespace AZ::WebGPU
