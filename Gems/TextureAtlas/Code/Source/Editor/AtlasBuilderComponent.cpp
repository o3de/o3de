/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "AtlasBuilderComponent.h"

#include <AzCore/Serialization/SerializeContext.h>

namespace TextureAtlasBuilder
{
    // AZ Components should only initialize their members to null and empty in constructor
    // Allocation of data should occur in Init(), once we can guarantee reflection and registration of types
    AtlasBuilderComponent::AtlasBuilderComponent()
    {
    }

    // Handle deallocation of your memory allocated in Init()
    AtlasBuilderComponent::~AtlasBuilderComponent()
    {
    }

    // Init is where you'll actually allocate memory or create objects
    // This ensures that any dependency components will have been been created and serialized
    void AtlasBuilderComponent::Init()
    {
    }

    // Activate is where you'd perform registration with other objects and systems.
    // All builder classes owned by this component should be registered here
    // Any EBuses for the builder classes should also be connected at this point
    void AtlasBuilderComponent::Activate()
    {
        AssetBuilderSDK::AssetBuilderDesc builderDescriptor;
        builderDescriptor.m_name = "Atlas Worker Builder";
        builderDescriptor.m_version = 2; // Add MipImageAsset allocator
        builderDescriptor.m_patterns.emplace_back(AssetBuilderSDK::AssetBuilderPattern("*.texatlas", AssetBuilderSDK::AssetBuilderPattern::PatternType::Wildcard));
        builderDescriptor.m_busId = azrtti_typeid<AtlasBuilderWorker>();
        builderDescriptor.m_createJobFunction = AZStd::bind(&AtlasBuilderWorker::CreateJobs, &m_atlasBuilder, AZStd::placeholders::_1, AZStd::placeholders::_2);
        builderDescriptor.m_processJobFunction = AZStd::bind(&AtlasBuilderWorker::ProcessJob, &m_atlasBuilder, AZStd::placeholders::_1, AZStd::placeholders::_2);

        m_atlasBuilder.BusConnect(builderDescriptor.m_busId);

        AssetBuilderSDK::AssetBuilderBus::Broadcast(&AssetBuilderSDK::AssetBuilderBusTraits::RegisterBuilderInformation, builderDescriptor);
    }

    // Disconnects from any EBuses we connected to in Activate()
    // Unregisters from objects and systems we register with in Activate()
    void AtlasBuilderComponent::Deactivate()
    {
        m_atlasBuilder.BusDisconnect();

        // We don't need to unregister the builder - the AP will handle this for us, because it is managing the lifecycle of this component
    }

    // Reflect the input and output formats for the serializer
    void AtlasBuilderComponent::Reflect(AZ::ReflectContext* context)
    {
        // components also get Reflect called automatically
        // this is your opportunity to perform static reflection or type registration of any types you want the serializer to know about
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<AtlasBuilderComponent, AZ::Component>()
                ->Version(0)
                ->Attribute(AZ::Edit::Attributes::SystemComponentTags, AZStd::vector<AZ::Crc32>({ AssetBuilderSDK::ComponentTags::AssetBuilder }))
                ;
        }

        AtlasBuilderInput::Reflect(context);
    }

    void AtlasBuilderComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("Atlas Builder Plugin Service"));
    }

    void AtlasBuilderComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("Atlas Builder Plugin Service"));
    }

    void AtlasBuilderComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        AZ_UNUSED(required);
    }

    void AtlasBuilderComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        AZ_UNUSED(dependent);
    }
}
