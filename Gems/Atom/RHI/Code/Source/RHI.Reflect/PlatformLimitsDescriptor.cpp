/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI.Reflect/PlatformLimitsDescriptor.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Settings/SettingsRegistryImpl.h>

namespace AZ::RHI
{
    void PlatformLimits::Reflect(ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
        {
            serializeContext->Class<PlatformLimits>()
                ->Version(1)
                ->Field("PlatformLimitsDescriptor", &PlatformLimits::m_platformLimitsDescriptor)
                ;
        }
    }

    void TransientAttachmentPoolBudgets::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<TransientAttachmentPoolBudgets>()
                ->Version(1)
                ->Field("BufferBudgetInBytes", &TransientAttachmentPoolBudgets::m_bufferBudgetInBytes)
                ->Field("ImageBudgetInBytes", &TransientAttachmentPoolBudgets::m_imageBudgetInBytes)
                ->Field("RenderTargetBudgetInBytes", &TransientAttachmentPoolBudgets::m_renderTargetBudgetInBytes)
                ;
        }
    }

    void PlatformDefaultValues::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<PlatformDefaultValues>()
                ->Version(1)
                ->Field("StagingBufferBudgetInBytes", &PlatformDefaultValues::m_stagingBufferBudgetInBytes)
                ->Field("AsyncQueueStagingBufferSizeInBytes", &PlatformDefaultValues::m_asyncQueueStagingBufferSizeInBytes)
                ->Field("MediumStagingBufferPageSizeInBytes", &PlatformDefaultValues::m_mediumStagingBufferPageSizeInBytes)
                ->Field("LargestStagingBufferPageSizeInBytes", &PlatformDefaultValues::m_largestStagingBufferPageSizeInBytes)
                ->Field("ImagePoolPageSizeInBytes", &PlatformDefaultValues::m_imagePoolPageSizeInBytes)
                ->Field("BufferPoolPageSizeInBytes", &PlatformDefaultValues::m_bufferPoolPageSizeInBytes)
                ;
        }
    }

    void PlatformLimitsDescriptor::Reflect(AZ::ReflectContext* context)
    {
        PlatformDefaultValues::Reflect(context);
        if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
        {
            serializeContext->Class<PlatformLimitsDescriptor>()
                ->Version(2)
                ->Field("TransientAttachmentPoolBudgets", &PlatformLimitsDescriptor::m_transientAttachmentPoolBudgets)
                ->Field("PlatformDefaultValues", &PlatformLimitsDescriptor::m_platformDefaultValues)
                ->Field("PagingParameters", &PlatformLimitsDescriptor::m_pagingParameters)
                ->Field("UsageHintParameters", &PlatformLimitsDescriptor::m_usageHintParameters)
                ->Field("HeapAllocationStrategy", &PlatformLimitsDescriptor::m_heapAllocationStrategy)
                ;
        }
    }

    RHI::Ptr<PlatformLimitsDescriptor> PlatformLimitsDescriptor::Create()
    {
        return aznew PlatformLimitsDescriptor;
    }

    void PlatformLimitsDescriptor::LoadPlatformLimitsDescriptor(const char* rhiName)
    {
        auto settingsRegistry = AZ::SettingsRegistry::Get();
        AZStd::string platformLimitsRegPath = AZStd::string::format("/O3DE/Atom/RHI/PlatformLimits/%s", rhiName);
        if (!(settingsRegistry &&
                settingsRegistry->GetObject(this, azrtti_typeid(this), platformLimitsRegPath.c_str())))
        {
            AZ_Warning(
                "Device", false, "Platform limits for %s %s is not loaded correctly. Will use default values.",
                AZ_TRAIT_OS_PLATFORM_NAME, rhiName);
        }
    }
}
