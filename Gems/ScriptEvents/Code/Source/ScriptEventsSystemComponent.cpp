/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ScriptEventsSystemComponent.h"

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/EntityId.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/Math/Color.h>
#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/sort.h>

#include <AzFramework/Asset/GenericAssetHandler.h>

#include <ScriptEvents/ScriptEventsAssetRef.h>
#include <ScriptEvents/ScriptEventDefinition.h>
#include <ScriptEvents/ScriptEventFundamentalTypes.h>

AZ_DEFINE_BUDGET(ScriptCanvas);

namespace ScriptEvents
{
    void ScriptEventsSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        using namespace ScriptEvents;

        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<ScriptEventsSystemComponent, AZ::Component>()
                ->Version(1)
                // ScriptEvents avoids a use dependency on the AssetBuilderSDK. Therefore the Crc is used directly to register this component with the Gem builder
                ->Attribute(AZ::Edit::Attributes::SystemComponentTags, AZStd::vector<AZ::Crc32>({ AZ_CRC_CE("AssetBuilder") }));
                ;
        }

        ScriptEventData::VersionedProperty::Reflect(context);
        Parameter::Reflect(context);
        Method::Reflect(context);
        ScriptEvent::Reflect(context);

        ScriptEvents::ScriptEventsAsset::Reflect(context);
        ScriptEvents::ScriptEventsAssetRef::Reflect(context);
        ScriptEvents::ScriptEventsAssetPtr::Reflect(context);
    }

    void ScriptEventsSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("ScriptEventsService"));
    }

    void ScriptEventsSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("ScriptEventsService"));
    }

    void ScriptEventsSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("AssetDatabaseService"));
    }

    void ScriptEventsSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        AZ_UNUSED(dependent);
    }

    void ScriptEventsSystemComponent::Init()
    {
    }

    void ScriptEventsSystemComponent::Activate()
    {
        ScriptEventsSystemComponentImpl* moduleConfiguration = nullptr;
        ScriptEventModuleConfigurationRequestBus::BroadcastResult(moduleConfiguration, &ScriptEventModuleConfigurationRequests::GetSystemComponentImpl);
        if (moduleConfiguration)
        {
            moduleConfiguration->RegisterAssetHandler();
        }
    }

    void ScriptEventsSystemComponent::Deactivate()
    {
        for (auto& asset : m_scriptEvents)
        {
            asset.second.reset();
        }

        m_scriptEvents.clear();

        ScriptEventsSystemComponentImpl* moduleConfiguration = nullptr;
        ScriptEventModuleConfigurationRequestBus::BroadcastResult(moduleConfiguration, &ScriptEventModuleConfigurationRequests::GetSystemComponentImpl);
        if (moduleConfiguration)
        {
            moduleConfiguration->UnregisterAssetHandler();
        }

    }

    void ScriptEventsSystemComponentRuntimeImpl::RegisterAssetHandler()
    {
        AZ::Data::AssetType assetType(azrtti_typeid<ScriptEvents::ScriptEventsAsset>());
        if (AZ::Data::AssetManager::Instance().GetHandler(assetType))
        {
            return; // Asset Type already handled
        }

        m_assetHandler = AZStd::make_unique<ScriptEventAssetRuntimeHandler>(ScriptEvents::ScriptEventsAsset::GetDisplayName(),
            ScriptEvents::ScriptEventsAsset::GetGroup(), ScriptEvents::ScriptEventsAsset::GetFileFilter(),
            AZ::AzTypeInfo<ScriptEvents::ScriptEventsSystemComponent>::Uuid());

        AZ::Data::AssetManager::Instance().RegisterHandler(m_assetHandler.get(), assetType);

        // Use AssetCatalog service to register ScriptCanvas asset type and extension
        AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequests::AddAssetType, assetType);
        AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequests::EnableCatalogForAsset, assetType);
        AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequests::AddExtension,
            ScriptEvents::ScriptEventsAsset::GetFileFilter());
    }

    void ScriptEventsSystemComponentRuntimeImpl::UnregisterAssetHandler()
    {
        AZ::Data::AssetManager::Instance().UnregisterHandler(m_assetHandler.get());
        m_assetHandler.reset();
    }

}
