/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "SliceSystemComponent.h"
#include "SliceAsset.h"
#include <AzCore/Serialization/EditContext.h>

namespace AZ
{
    void SliceSystemComponent::Reflect(ReflectContext* context)
    {
        if (SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context))
        {
            serializeContext->Class<SliceSystemComponent, Component>();

            if (EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<SliceSystemComponent>(
                    "Slice System", "Manages the Slice system")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Engine")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System", 0xc94d118b))
                    ;
            }
        }
    }

    void SliceSystemComponent::GetProvidedServices(ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC("SliceSystemService", 0x1a5b7aad));
    }

    void SliceSystemComponent::GetIncompatibleServices(ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC("SliceSystemService", 0x1a5b7aad));
    }

    void SliceSystemComponent::GetDependentServices(ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC("AssetDatabaseService", 0x3abf5601));
        services.push_back(AZ_CRC("AssetCatalogService", 0xc68ffc57));
    }

    void SliceSystemComponent::Activate()
    {
        // Register with AssetDatabase
        AZ_Error("SliceSystemComponent", Data::AssetManager::IsReady(), "Can't register slice asset handler with AssetManager.");
        if (Data::AssetManager::IsReady())
        {
            Data::AssetManager::Instance().RegisterHandler(&m_assetHandler, AzTypeInfo<SliceAsset>::Uuid());
            Data::AssetManager::Instance().RegisterHandler(&m_assetHandler, AzTypeInfo<DynamicSliceAsset>::Uuid());
        }

        // Register with AssetCatalog
        EBUS_EVENT(Data::AssetCatalogRequestBus, EnableCatalogForAsset, AZ::AzTypeInfo<AZ::SliceAsset>::Uuid());
        EBUS_EVENT(Data::AssetCatalogRequestBus, AddExtension, "slice");
        EBUS_EVENT(Data::AssetCatalogRequestBus, EnableCatalogForAsset, AZ::AzTypeInfo<AZ::DynamicSliceAsset>::Uuid());
        EBUS_EVENT(Data::AssetCatalogRequestBus, AddExtension, "dynamicslice");
    }

    void SliceSystemComponent::Deactivate()
    {
        if (Data::AssetManager::IsReady())
        {
            Data::AssetManager::Instance().UnregisterHandler(&m_assetHandler);
        }
    }

} // namespace AZ
