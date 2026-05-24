/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Builders/GenericAssetBuilder/GenericAssetBuilderComponent.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzFramework/Asset/GenericAssetHandler.h>

AZ::ComponentDescriptor* GenericAssetBuilderComponent_CreateDescriptor()
{
    return LmbrCentral::GenericAssetBuilder::GenericAssetBuilderComponent::CreateDescriptor();
}

namespace LmbrCentral::GenericAssetBuilder
{
    void GenericAssetBuilderComponent::Init()
    {
    }

    void GenericAssetBuilderComponent::Activate()
    {
        AZStd::vector<GenericAssetBuilderRegistration> registrations;

        AssetBuilderSDK::AssetBuilderDesc builderDescriptor;
        builderDescriptor.m_name = "Generic Asset Builder";

        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
        AZ_Assert(serializeContext, "Failed to acquire serialize context.");
        if (!serializeContext)
        {
            return;
        }

        AZ::Data::AssetManager& manager = AZ::Data::AssetManager::Instance();

        // find all of the asset types that are handled by GenericAssetHandlers, and then check which ones
        // are opting into auto building.
        
        auto callback = 
        [&manager, &registrations](const AZ::SerializeContext::ClassData* classData, const AZ::Uuid&) -> bool
        {
            AZ::Data::AssetHandler* handler = manager.GetHandler(classData->m_typeId);
            if (!azrtti_istypeof<AzFramework::GenericAssetHandlerBase*>(handler))
            {
                // Not handled by the generic asset handler.
                return true; // return true to continue iterating!
            }

            AZ::Uuid assetTypeId = classData->m_typeId;

            AzFramework::GenericAssetHandlerBase* genericHandler = azrtti_cast<AzFramework::GenericAssetHandlerBase*>(handler);
            if (!genericHandler)
            {
                return true;
            }
            // check if we support copying this automatically
            if (genericHandler->AutoBuildAssetToCache())
            {
                AZStd::vector<AZStd::string> extensions;
                AZ::AssetTypeInfoBus::Event(assetTypeId, &AZ::AssetTypeInfoBus::Events::GetAssetTypeExtensions, extensions);
                for (const AZStd::string& extension : extensions)
                {
                    if (extension.empty())
                    {
                        AZ_Error("GenericAssetBuilder", false,
                            "GenericAsset with type [%s] wants to Auto Build using the Generic Asset Builder but has not "
                            "emitted exensions using AZ::AssetTypeInfoBus::Events::GetAssetTypeExtensions(), it will not process.",
                            classData->m_name);
                        continue;
                    }
                    registrations.emplace_back(
                        GenericAssetBuilderRegistration{ AZStd::string::format("*.%s", extension.c_str()), assetTypeId });
                }
            }
            return true;
        };
         
        serializeContext->EnumerateDerived<AZ::Data::AssetData>(callback);

        for (const GenericAssetBuilderRegistration& registration : registrations)
        {
                builderDescriptor.m_patterns.emplace_back(
                    AssetBuilderSDK::AssetBuilderPattern(
                     registration.m_pattern,
                     AssetBuilderSDK::AssetBuilderPattern::PatternType::Wildcard));
        }

        builderDescriptor.m_busId = azrtti_typeid<GenericAssetBuilderWorker>();
        builderDescriptor.m_version = 1; // if you change this, all assets will automatically rebuild
        builderDescriptor.m_analysisFingerprint = ""; // if you change this, all assets will re-analyze but not necessarily rebuild.
        builderDescriptor.m_createJobFunction = AZStd::bind(&GenericAssetBuilderWorker::CreateJobs, &m_genericAssetBuilder,
                                                            AZStd::placeholders::_1, AZStd::placeholders::_2);
        builderDescriptor.m_processJobFunction = AZStd::bind(&GenericAssetBuilderWorker::ProcessJob, &m_genericAssetBuilder,
                                                             AZStd::placeholders::_1, AZStd::placeholders::_2);

        // This builder specifically emits dependencies
        builderDescriptor.m_flags = AssetBuilderSDK::AssetBuilderDesc::BF_None;

        m_genericAssetBuilder.BusConnect(builderDescriptor.m_busId);
        m_genericAssetBuilder.SetRegistrations(AZStd::move(registrations));

        AssetBuilderSDK::AssetBuilderBus::Broadcast(&AssetBuilderSDK::AssetBuilderBusTraits::RegisterBuilderInformation,
                                                    builderDescriptor);
    }

    // Disconnects from any EBuses we connected to in Activate()
    // Unregisters from objects and systems we register with in Activate()
    void GenericAssetBuilderComponent::Deactivate()
    {
        // We don't need to unregister the builder - the AP will handle this for us, because it is
        // managing the lifecycle of this component. All we need to do is disconnect from the bus.
        m_genericAssetBuilder.BusDisconnect();
    }

    void GenericAssetBuilderComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<GenericAssetBuilderComponent, AZ::Component>()
                ->Version(0)
                ->Attribute(AZ::Edit::Attributes::SystemComponentTags,
                            AZStd::vector<AZ::Crc32>({ AssetBuilderSDK::ComponentTags::AssetBuilder }))
            ;
        }
    }

    void GenericAssetBuilderComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        // do not depend on this in your component, or it will create a loop
        provided.push_back(AZ_CRC_CE("GenericAssetBuilderPluginService"));
    }

    void GenericAssetBuilderComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("GenericAssetBuilderPluginService"));
    }

    void GenericAssetBuilderComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        AZ_UNUSED(required);
    }

    void GenericAssetBuilderComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        // causes all of the components that provide this registrar service to activate before we do,
        // so that by the time we activate, we know they are all there.
        dependent.push_back(AzFramework::s_GenericAssetRegistrar);
    }
} // namespace LmbrCentral::GenericAssetBuilder
