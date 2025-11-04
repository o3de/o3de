/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI.Reflect/DX12/PlatformLimitsDescriptor.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Settings/SettingsRegistryImpl.h>

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
                    ->Version(1)
                    ->Field("DescriptorHeapLimits", &PlatformLimitsDescriptor::m_descriptorHeapLimits)
                    ->Field("StaticDescriptorRatio", &PlatformLimitsDescriptor::m_staticDescriptorRatio)
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

        void PlatformLimitsDescriptor::LoadPlatformLimitsDescriptor(const char* rhiName)
        {
            auto settingsRegistry = AZ::SettingsRegistry::Get();
            AZStd::string platformLimitsRegPath = AZStd::string::format("/O3DE/Atom/RHI/PlatformLimits/%s", rhiName);
            if (!(settingsRegistry && settingsRegistry->GetObject(this, azrtti_typeid(this), platformLimitsRegPath.c_str())))
            {
                AZ_Warning(
                    "Device", false, "Platform limits for %s %s is not loaded correctly. Will use default values.",
                    AZ_TRAIT_OS_PLATFORM_NAME, rhiName);

                // Map default value must be initialized after attempting to serialize (and result in failure).
                // Otherwise, serialization won't overwrite the default values.
                m_descriptorHeapLimits = AZStd::unordered_map<AZStd::string, AZStd::array<uint32_t, NumHeapFlags>>({
                        { AZStd::string("DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV"), { 100000, 1000000 } },
                        { AZStd::string("DESCRIPTOR_HEAP_TYPE_SAMPLER"), { 2048, 2048 } },
                        { AZStd::string("DESCRIPTOR_HEAP_TYPE_RTV"), { 2048, 0 } },
                        { AZStd::string("DESCRIPTOR_HEAP_TYPE_DSV"), { 2048, 0 } }
                    });
            }

        }
    }
}
