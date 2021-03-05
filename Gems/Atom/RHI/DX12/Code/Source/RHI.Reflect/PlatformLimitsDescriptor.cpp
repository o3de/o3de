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
#include <Atom/RHI.Reflect/DX12/PlatformLimitsDescriptor.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AZ
{
    namespace DX12
    {
        void PlatformLimitsDescriptor::Reflect(AZ::ReflectContext* context)
        {
            FrameGraphExecuterData::Reflect(context);
            if (SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<PlatformLimitsDescriptor, Base>()
                    ->Version(0)
                    ->Field("m_descriptorHeapLimits", &PlatformLimitsDescriptor::m_descriptorHeapLimits)
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
