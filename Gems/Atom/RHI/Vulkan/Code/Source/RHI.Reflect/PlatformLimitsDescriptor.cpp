/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
                    ->Version(0)
                    ->Field("m_frameGraphExecuterData", &PlatformLimitsDescriptor::m_frameGraphExecuterData)
                    ;
            }
        }

        void FrameGraphExecuterData::Reflect(AZ::ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<FrameGraphExecuterData>()
                    ->Version(0)
                    ->Field("m_itemCost", &FrameGraphExecuterData::m_itemCost)
                    ->Field("m_attachmentCost", &FrameGraphExecuterData::m_attachmentCost)
                    ->Field("m_swapChainsPerCommandList", &FrameGraphExecuterData::m_swapChainsPerCommandList)
                    ->Field("m_commandListCostThresholdMin", &FrameGraphExecuterData::m_commandListCostThresholdMin)
                    ->Field("m_commandListsPerScopeMax", &FrameGraphExecuterData::m_commandListsPerScopeMax)
                    ;
            }
        }
    }
}
