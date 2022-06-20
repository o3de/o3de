/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI.Reflect/Vulkan/PlatformLimitsDescriptor.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AZ
{
    namespace Vulkan
    {
        void PlatformLimitsDescriptor::Reflect(AZ::ReflectContext* context)
        {
            FrameGraphExecuterData::Reflect(context);
            if (SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<PlatformLimitsDescriptor, Base>()
                    ->Version(1)
                    ->Field("FrameGraphExecuterData", &PlatformLimitsDescriptor::m_frameGraphExecuterData)
                    ;
            }
        }

        void FrameGraphExecuterData::Reflect(AZ::ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<FrameGraphExecuterData>()
                    ->Version(0)
                    ->Field("ItemCost", &FrameGraphExecuterData::m_itemCost)
                    ->Field("AttachmentCost", &FrameGraphExecuterData::m_attachmentCost)
                    ->Field("SwapChainsPerCommandList", &FrameGraphExecuterData::m_swapChainsPerCommandList)
                    ->Field("CommandListCostThresholdMin", &FrameGraphExecuterData::m_commandListCostThresholdMin)
                    ->Field("CommandListsPerScopeMax", &FrameGraphExecuterData::m_commandListsPerScopeMax)
                    ;
            }
        }
    }
}
