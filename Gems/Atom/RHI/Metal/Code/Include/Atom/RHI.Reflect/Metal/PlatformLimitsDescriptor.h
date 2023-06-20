/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RHI.Reflect/PlatformLimitsDescriptor.h>

namespace AZ
{
    class ReflectContext;

    namespace Metal
    {
        struct FrameGraphExecuterData
        {
            AZ_TYPE_INFO(AZ::Metal::FrameGraphExecuterData, "{BD831EFB-CC74-46F8-BE48-118B2E8F07D0}");
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
            AZ_RTTI(AZ::Metal::PlatformLimitsDescriptor, "{B89F116F-9FEF-4BCA-9EC7-9FF8F772B7FD}", Base);
            AZ_CLASS_ALLOCATOR(AZ::Metal::PlatformLimitsDescriptor, AZ::SystemAllocator);
            static void Reflect(AZ::ReflectContext* context);

            FrameGraphExecuterData m_frameGraphExecuterData;
        };

    }
}
