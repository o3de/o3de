/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Builder/WwiseBuilderComponent.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>

namespace WwiseBuilder
{
    void BuilderPluginComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<WwiseBuilder::BuilderPluginComponent, AZ::Component>()
                ->Version(1)
                ->Attribute(AZ::Edit::Attributes::SystemComponentTags, AZStd::vector<AZ::Crc32>({ AssetBuilderSDK::ComponentTags::AssetBuilder }))
                ;
        }
    }

    void BuilderPluginComponent::Activate()
    {
        // Register Wwise builder
        AssetBuilderSDK::AssetBuilderDesc builderDescriptor;
        builderDescriptor.m_name = "Wwise Builder";
        builderDescriptor.m_patterns.push_back(AssetBuilderSDK::AssetBuilderPattern("*.bnk", AssetBuilderSDK::AssetBuilderPattern::PatternType::Wildcard));
        builderDescriptor.m_patterns.push_back(AssetBuilderSDK::AssetBuilderPattern("*.wem", AssetBuilderSDK::AssetBuilderPattern::PatternType::Wildcard));
        builderDescriptor.m_busId = azrtti_typeid<WwiseBuilderWorker>();
        builderDescriptor.m_version = 2;
        builderDescriptor.m_createJobFunction = AZStd::bind(&WwiseBuilderWorker::CreateJobs, &m_wwiseBuilder, AZStd::placeholders::_1, AZStd::placeholders::_2);
        builderDescriptor.m_processJobFunction = AZStd::bind(&WwiseBuilderWorker::ProcessJob, &m_wwiseBuilder, AZStd::placeholders::_1, AZStd::placeholders::_2);

        // (optimization) this builder does not emit source dependencies:
        builderDescriptor.m_flags |= AssetBuilderSDK::AssetBuilderDesc::BF_EmitsNoDependencies;

        m_wwiseBuilder.BusConnect(builderDescriptor.m_busId);

        AssetBuilderSDK::AssetBuilderBus::Broadcast(&AssetBuilderSDK::AssetBuilderBus::Events::RegisterBuilderInformation, builderDescriptor);
    }

    void BuilderPluginComponent::Deactivate()
    {
        m_wwiseBuilder.BusDisconnect();
    }

} // namespace WwiseBuilder
