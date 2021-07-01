/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI.Reflect/PlatformLimitsDescriptor.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AZ
{
    namespace RHI
    {
        void PlatformLimits::Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<PlatformLimits>()
                    ->Version(0)
                    ->Field("m_platformLimitsDescriptor", &PlatformLimits::m_platformLimitsDescriptor)
                    ;
            }
        }

        void TransientAttachmentPoolBudgets::Reflect(AZ::ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<TransientAttachmentPoolBudgets>()
                    ->Version(0)
                    ->Field("m_bufferBudgetInBytes", &TransientAttachmentPoolBudgets::m_bufferBudgetInBytes)
                    ->Field("m_imageBudgetInBytes", &TransientAttachmentPoolBudgets::m_imageBudgetInBytes)
                    ->Field("m_renderTargetBudgetInBytes", &TransientAttachmentPoolBudgets::m_renderTargetBudgetInBytes)
                    ;
            }
        }

        void PlatformDefaultValues::Reflect(AZ::ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<PlatformDefaultValues>()
                    ->Version(0)
                    ->Field("m_stagingBufferBudgetInBytes", &PlatformDefaultValues::m_stagingBufferBudgetInBytes)
                    ->Field("m_asyncQueueStagingBufferSizeInBytes", &PlatformDefaultValues::m_asyncQueueStagingBufferSizeInBytes)
                    ->Field("m_mediumStagingBufferPageSizeInBytes", &PlatformDefaultValues::m_mediumStagingBufferPageSizeInBytes)
                    ->Field("m_largestStagingBufferPageSizeInBytes", &PlatformDefaultValues::m_largestStagingBufferPageSizeInBytes)
                    ->Field("m_imagePoolPageSizeInBytes", &PlatformDefaultValues::m_imagePoolPageSizeInBytes)
                    ->Field("m_bufferPoolPageSizeInBytes", &PlatformDefaultValues::m_bufferPoolPageSizeInBytes)
                    ;
            }
        }

        void PlatformLimitsDescriptor::Reflect(AZ::ReflectContext* context)
        {
            PlatformDefaultValues::Reflect(context);
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<PlatformLimitsDescriptor>()
                    ->Version(1)
                    ->Field("m_transientAttachmentPoolBudgets", &PlatformLimitsDescriptor::m_transientAttachmentPoolBudgets)
                    ->Field("m_platformDefaultValues", &PlatformLimitsDescriptor::m_platformDefaultValues)
                    ->Field("m_pagingParameters", &PlatformLimitsDescriptor::m_pagingParameters)
                    ->Field("m_usageHintParameters", &PlatformLimitsDescriptor::m_usageHintParameters)
                    ->Field("m_heapAllocationStrategy", &PlatformLimitsDescriptor::m_heapAllocationStrategy)
                    ;
            }
        }

        RHI::Ptr<PlatformLimitsDescriptor> PlatformLimitsDescriptor::Create()
        {
            return aznew PlatformLimitsDescriptor;
        }
    }
}
