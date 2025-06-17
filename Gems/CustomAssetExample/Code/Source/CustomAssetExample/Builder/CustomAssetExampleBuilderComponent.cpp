/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <CustomAssetExample/Builder/CustomAssetExampleBuilderComponent.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>

namespace CustomAssetExample
{
    // AZ Components should only initialize their members to null and empty in constructor
    // Allocation of data should occur in Init(), once we can guarantee reflection and registration of types
    ExampleBuilderComponent::ExampleBuilderComponent()
    {
    }

    // Handle deallocation of your memory allocated in Init()
    ExampleBuilderComponent::~ExampleBuilderComponent()
    {
    }

    // Init is where you'll actually allocate memory or create objects
    // This ensures that any dependency components will have been been created and serialized
    void ExampleBuilderComponent::Init()
    {
    }

    // Activate is where you'd perform registration with other objects and systems.
    // All builder classes owned by this component should be registered here
    // Any EBuses for the builder classes should also be connected at this point
    void ExampleBuilderComponent::Activate()
    {
        AssetBuilderSDK::AssetBuilderDesc builderDescriptor;
        builderDescriptor.m_name = "Example Worker Builder";
        builderDescriptor.m_patterns.emplace_back(AssetBuilderSDK::AssetBuilderPattern("*.example", AssetBuilderSDK::AssetBuilderPattern::PatternType::Wildcard));
        builderDescriptor.m_patterns.emplace_back(AssetBuilderSDK::AssetBuilderPattern("*.exampleinclude", AssetBuilderSDK::AssetBuilderPattern::PatternType::Wildcard));
        builderDescriptor.m_patterns.emplace_back(AssetBuilderSDK::AssetBuilderPattern("*.examplesource", AssetBuilderSDK::AssetBuilderPattern::PatternType::Wildcard));
        builderDescriptor.m_patterns.emplace_back(AssetBuilderSDK::AssetBuilderPattern("*.examplejob", AssetBuilderSDK::AssetBuilderPattern::PatternType::Wildcard));
        builderDescriptor.m_busId = azrtti_typeid<ExampleBuilderWorker>();
        builderDescriptor.m_version = 1; // if you change this, all assets will automatically rebuild
        builderDescriptor.m_analysisFingerprint = ""; // if you change this, all assets will re-analyze but not necessarily rebuild.
        builderDescriptor.m_createJobFunction = AZStd::bind(&ExampleBuilderWorker::CreateJobs, &m_exampleBuilder, AZStd::placeholders::_1, AZStd::placeholders::_2);
        builderDescriptor.m_processJobFunction = AZStd::bind(&ExampleBuilderWorker::ProcessJob, &m_exampleBuilder, AZStd::placeholders::_1, AZStd::placeholders::_2);

        // note that this particualar builder does in fact emit various kinds of dependencies (as an example).
        // if your builder is simple and emits no dependencies (for example, it just processes a single file and that file
        // doesn't really depend on any other files or jobs), setting the BF_EmitsNoDependencies flag 
        // will improve "fast analysis" scan performance.
        builderDescriptor.m_flags = AssetBuilderSDK::AssetBuilderDesc::BF_None;

        m_exampleBuilder.BusConnect(builderDescriptor.m_busId);

        AssetBuilderSDK::AssetBuilderBus::Broadcast(&AssetBuilderSDK::AssetBuilderBusTraits::RegisterBuilderInformation, builderDescriptor);
    }

    // Disconnects from any EBuses we connected to in Activate()
    // Unregisters from objects and systems we register with in Activate()
    void ExampleBuilderComponent::Deactivate()
    {
        m_exampleBuilder.BusDisconnect();

        // We don't need to unregister the builder - the AP will handle this for us, because it is managing the lifecycle of this component
    }

    // This is your opportunity to perform static reflection or type registration of any types you need the serializer to know about
    void ExampleBuilderComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<ExampleBuilderComponent, AZ::Component>()
                ->Version(0)
                ->Attribute(AZ::Edit::Attributes::SystemComponentTags, AZStd::vector<AZ::Crc32>({ AssetBuilderSDK::ComponentTags::AssetBuilder }))
            ;
        }
    }

    void ExampleBuilderComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("ExampleBuilderPluginService"));
    }

    void ExampleBuilderComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("ExampleBuilderPluginService"));
    }

    void ExampleBuilderComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        AZ_UNUSED(required);
    }

    void ExampleBuilderComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        AZ_UNUSED(dependent);
    }
}
