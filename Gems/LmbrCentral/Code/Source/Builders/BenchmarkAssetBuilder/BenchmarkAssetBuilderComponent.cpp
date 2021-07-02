/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <LmbrCentral_precompiled.h>
#include <Builders/BenchmarkAssetBuilder/BenchmarkAssetBuilderComponent.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>
#include <AzFramework/Asset/Benchmark/BenchmarkAsset.h>
#include <AzFramework/Asset/Benchmark/BenchmarkSettingsAsset.h>

namespace BenchmarkAssetBuilder
{
    void BenchmarkAssetBuilderComponent::Init()
    {
    }

    void BenchmarkAssetBuilderComponent::Activate()
    {
        // Set up our asset builder for the BenchmarkAsset and BenchmarkSettingsAsset assets
        AssetBuilderSDK::AssetBuilderDesc builderDescriptor;
        builderDescriptor.m_name = "Benchmark Asset Worker Builder";
        builderDescriptor.m_patterns.emplace_back(AssetBuilderSDK::AssetBuilderPattern(
                                                    AZStd::string("*.") + AzFramework::s_benchmarkAssetExtension,
                                                    AssetBuilderSDK::AssetBuilderPattern::PatternType::Wildcard));
        builderDescriptor.m_patterns.emplace_back(AssetBuilderSDK::AssetBuilderPattern(
                                                    AZStd::string("*.") + AzFramework::s_benchmarkSettingsAssetExtension,
                                                    AssetBuilderSDK::AssetBuilderPattern::PatternType::Wildcard));
        builderDescriptor.m_busId = azrtti_typeid<BenchmarkAssetBuilderWorker>();
        builderDescriptor.m_version = 1; // if you change this, all assets will automatically rebuild
        builderDescriptor.m_analysisFingerprint = ""; // if you change this, all assets will re-analyze but not necessarily rebuild.
        builderDescriptor.m_createJobFunction = AZStd::bind(&BenchmarkAssetBuilderWorker::CreateJobs, &m_benchmarkAssetBuilder,
                                                            AZStd::placeholders::_1, AZStd::placeholders::_2);
        builderDescriptor.m_processJobFunction = AZStd::bind(&BenchmarkAssetBuilderWorker::ProcessJob, &m_benchmarkAssetBuilder,
                                                             AZStd::placeholders::_1, AZStd::placeholders::_2);

        // This builder specifically emits dependencies
        builderDescriptor.m_flags = AssetBuilderSDK::AssetBuilderDesc::BF_None;

        m_benchmarkAssetBuilder.BusConnect(builderDescriptor.m_busId);

        AssetBuilderSDK::AssetBuilderBus::Broadcast(&AssetBuilderSDK::AssetBuilderBusTraits::RegisterBuilderInformation,
                                                    builderDescriptor);
    }

    // Disconnects from any EBuses we connected to in Activate()
    // Unregisters from objects and systems we register with in Activate()
    void BenchmarkAssetBuilderComponent::Deactivate()
    {
        // We don't need to unregister the builder - the AP will handle this for us, because it is
        // managing the lifecycle of this component. All we need to do is disconnect from the bus.
        m_benchmarkAssetBuilder.BusDisconnect();
    }

    void BenchmarkAssetBuilderComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<BenchmarkAssetBuilderComponent, AZ::Component>()
                ->Version(0)
                ->Attribute(AZ::Edit::Attributes::SystemComponentTags,
                            AZStd::vector<AZ::Crc32>({ AssetBuilderSDK::ComponentTags::AssetBuilder }))
            ;
        }
    }

    void BenchmarkAssetBuilderComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("BenchmarkAssetBuilderPluginService"));
    }

    void BenchmarkAssetBuilderComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("BenchmarkAssetBuilderPluginService"));
    }

    void BenchmarkAssetBuilderComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        AZ_UNUSED(required);
    }

    void BenchmarkAssetBuilderComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        AZ_UNUSED(dependent);
    }
}
